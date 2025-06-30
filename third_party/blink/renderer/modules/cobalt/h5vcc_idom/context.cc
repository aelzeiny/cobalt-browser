// Copyright 2025 The Cobalt Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/context.h"

#include "third_party/blink/renderer/bindings/modules/v8/v8_node_function.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_notification.h"
#include "v8/include/v8.h"

namespace cobalt {
namespace h5vcc {
namespace idom {

Context::Context(blink::Node* node) : node_(node), tracking_disabled_(false) {
  // Reserve modest capacity - larger patches will disable tracking if needed
  created_.reserve(1024);
  deleted_.reserve(512);
}

void Context::MarkCreated(blink::Node* node) {
  if (tracking_disabled_) {
    return;  // Skip tracking entirely
  }

  // If we're approaching vector limits, disable tracking to prevent crashes
  if (created_.size() >= 8192) {  // 8K limit - much more conservative
    if (!tracking_disabled_) {
      tracking_disabled_ = true;
      DLOG(WARNING) << "Context: Disabling node tracking after "
                    << created_.size() << " nodes to prevent memory issues";
      // Clear existing tracked nodes to free memory
      created_.clear();
      deleted_.clear();
    }
    return;
  }

  created_.push_back(node);
}

void Context::MarkDeleted(blink::Node* node) {
  if (tracking_disabled_) {
    return;  // Skip tracking entirely
  }

  // Apply same limit logic as MarkCreated
  if (deleted_.size() >= 2048) {  // 2K limit for deletions
    if (!tracking_disabled_) {
      tracking_disabled_ = true;
      DLOG(WARNING) << "Context: Disabling node tracking after "
                    << deleted_.size() << " deletions to prevent memory issues";
      created_.clear();
      deleted_.clear();
    }
    return;
  }

  deleted_.push_back(node);
}

void Context::NotifyChanges(blink::IDomNotification* notifications) {
  // If tracking was disabled, we have no nodes to notify about
  if (tracking_disabled_ || !notifications) {
    return;
  }

  if (notifications->nodesCreated() && !created_.empty()) {
    v8::Maybe<void> result =
        notifications->nodesCreated()->Invoke(nullptr, created_);
    (void)result;
  }
  if (notifications->nodesDeleted() && !deleted_.empty()) {
    v8::Maybe<void> result =
        notifications->nodesDeleted()->Invoke(nullptr, deleted_);
    (void)result;
  }
}

void Context::Trace(blink::Visitor* visitor) const {
  visitor->Trace(created_);
  visitor->Trace(deleted_);
  visitor->Trace(node_);
}

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

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

Context::Context(blink::Node* node) : node_(node) {}

void Context::MarkCreated(blink::Node* node) {
  created_.push_back(node);
}

void Context::MarkDeleted(blink::Node* node) {
  deleted_.push_back(node);
}

void Context::NotifyChanges(blink::IDomNotification* notifications) {
  if (notifications) {
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
}

void Context::Trace(blink::Visitor* visitor) const {
  visitor->Trace(created_);
  visitor->Trace(deleted_);
  visitor->Trace(node_);
}

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

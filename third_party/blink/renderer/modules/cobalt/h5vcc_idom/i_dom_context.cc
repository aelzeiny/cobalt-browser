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

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_context.h"

#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/context.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_notification.h"

namespace blink {

IDomContext::IDomContext() : context_(nullptr) {}

IDomContext::IDomContext(cobalt::h5vcc::idom::Context* context)
    : context_(context) {}

void IDomContext::markCreated(Node* node) {
  if (context_) {
    context_->MarkCreated(node);
  }
}

void IDomContext::markDeleted(Node* node) {
  if (context_) {
    context_->MarkDeleted(node);
  }
}

void IDomContext::notifyChanges(IDomNotification* notifications) {
  if (context_) {
    context_->NotifyChanges(notifications);
  }
}

void IDomContext::Trace(Visitor* visitor) const {
  ScriptWrappable::Trace(visitor);
  // Note: context_ is not traced as it's managed by the cobalt namespace
}

}  // namespace blink

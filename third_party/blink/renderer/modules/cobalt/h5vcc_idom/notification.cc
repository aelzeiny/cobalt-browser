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

#include "third_party/blink/renderer/bindings/modules/v8/v8_node_function.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_notification.h"

namespace blink {

IDomNotification::IDomNotification() = default;
IDomNotification::~IDomNotification() = default;

V8NodeFunction* IDomNotification::nodesCreated() const {
  return nodes_created_;
}

void IDomNotification::setNodesCreated(V8NodeFunction* value) {
  nodes_created_ = value;
}

V8NodeFunction* IDomNotification::nodesDeleted() const {
  return nodes_deleted_;
}

void IDomNotification::setNodesDeleted(V8NodeFunction* value) {
  nodes_deleted_ = value;
}

void IDomNotification::Trace(Visitor* visitor) const {
  visitor->Trace(nodes_created_);
  visitor->Trace(nodes_deleted_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink

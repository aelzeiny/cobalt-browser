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

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_node_data.h"

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/node_data.h"

namespace blink {

IDomNodeData::IDomNodeData(cobalt::h5vcc::idom::NodeData* node_data)
    : node_data_(node_data) {}

String IDomNodeData::nameOrCtor() const {
  return node_data_ ? String(node_data_->GetNameOrCtor()) : String();
}

String IDomNodeData::key() const {
  return node_data_ ? String(node_data_->GetKey()) : String();
}

String IDomNodeData::text() const {
  return node_data_ ? String(node_data_->GetText()) : String();
}

bool IDomNodeData::staticsApplied() const {
  return node_data_ ? node_data_->StaticsApplied() : false;
}

bool IDomNodeData::alwaysDiffAttributes() const {
  return node_data_ ? node_data_->AlwaysDiffAttributes() : false;
}

void IDomNodeData::Trace(Visitor* visitor) const {
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink

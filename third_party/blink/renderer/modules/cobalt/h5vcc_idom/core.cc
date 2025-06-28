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

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/core.h"

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/assertions.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/context.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/node_data.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/nodes.h"

namespace cobalt {
namespace h5vcc {
namespace idom {

namespace {

Context* context = nullptr;
blink::Node* current_node = nullptr;
blink::Node* current_parent = nullptr;

void EnterNode() {
  current_parent = current_node;
  current_node = nullptr;
}

void ExitNode() {
  // clearUnvisitedDOM(current_parent, getNextNode(), null);
  current_node = current_parent;
  current_parent = current_parent->parentNode();
}

}  // namespace

void AlignWithDOM(NodeDataMap& data_map,
                  const WTF::String& name_or_ctor,
                  const WTF::String& key,
                  const WTF::String& nonce) {
  // This is a simplified implementation.
  current_node = GetNextNode();
}

void AlwaysDiffAttributes(blink::Element* el) {
  // This is a simplified implementation.
}

blink::Element* Close() {
  ExitNode();
  return blink::To<blink::Element>(current_node);
}

Context* CurrentContext() {
  return context;
}

blink::Element* CurrentElement() {
  return blink::To<blink::Element>(current_parent);
}

blink::Node* CurrentPointer() {
  return GetNextNode();
}

blink::Node* GetNextNode() {
  if (current_node) {
    return current_node->nextSibling();
  } else {
    return current_parent->firstChild();
  }
}

blink::Element* Open(NodeDataMap& data_map,
                     const WTF::String& name_or_ctor,
                     const WTF::String& key,
                     const WTF::String& nonce) {
  AlignWithDOM(data_map, name_or_ctor, key, nonce);
  EnterNode();
  return blink::To<blink::Element>(current_parent);
}

void Skip() {
  AssertNoChildrenDeclaredYet("skip", current_node);
  SetInSkip(true);
  current_node = current_parent->lastChild();
}

blink::Node* SkipNode() {
  current_node = GetNextNode();
  return current_node;
}

blink::Element* TryGetCurrentElement() {
  return blink::To<blink::Element>(current_parent);
}

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

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

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/dom_util.h"

#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"

namespace cobalt {
namespace h5vcc {
namespace idom {

namespace {

blink::HeapVector<blink::Member<blink::Node>> GetAncestry(blink::Node* node,
                                                          blink::Node* root) {
  blink::HeapVector<blink::Member<blink::Node>> ancestry;
  blink::Node* cur = node;
  while (cur != root) {
    ancestry.push_back(cur);
    cur = cur->parentNode();
    if (!cur && root) {
      if (auto* shadow_root =
              DynamicTo<blink::ShadowRoot>(node->getRootNode(nullptr))) {
        cur = &shadow_root->host();
      }
    }
  }
  return ancestry;
}

}  // namespace

blink::HeapVector<blink::Member<blink::Node>> GetFocusedPath(
    blink::Node* node,
    blink::Node* root) {
  blink::Element* active_element = node->GetDocument().ActiveElement();
  if (!active_element || !node->contains(active_element)) {
    return blink::HeapVector<blink::Member<blink::Node>>();
  }
  return GetAncestry(active_element, root);
}

void MoveBefore(blink::Node* parentNode,
                blink::Node* node,
                blink::Node* referenceNode) {
  blink::Node* insert_reference_node = node->nextSibling();
  blink::Node* cur = referenceNode;
  while (cur != nullptr && cur != node) {
    blink::Node* next = cur->nextSibling();
    parentNode->insertBefore(cur, insert_reference_node, ASSERT_NO_EXCEPTION);
    cur = next;
  }
}

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

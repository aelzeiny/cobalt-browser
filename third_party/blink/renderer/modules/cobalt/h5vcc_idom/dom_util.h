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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_DOM_UTIL_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_DOM_UTIL_H_

#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_vector.h"

namespace blink {
class Node;
}

namespace cobalt {
namespace h5vcc {
namespace idom {

blink::HeapVector<blink::Member<blink::Node>> GetFocusedPath(blink::Node* node,
                                                             blink::Node* root);
void MoveBefore(blink::Node* parentNode,
                blink::Node* node,
                blink::Node* referenceNode);

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_DOM_UTIL_H_

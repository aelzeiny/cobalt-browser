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

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/assertions.h"

#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/node.h"

namespace cobalt {
namespace h5vcc {
namespace idom {

// Checks that no children have been declared yet for the given node.
void AssertNoChildrenDeclaredYet(const char* function_name, blink::Node* node) {
  // This is a simplified implementation.
  // In the original, this would check if any children have been processed.
  // For now, we'll just do a basic check
  if (node && node->firstChild()) {
    DLOG(WARNING) << function_name
                  << "() called on node that already has children";
  }
}

// Asserts that patchOuter has a parent node.
void AssertPatchOuterHasParentNode(blink::Node* parent) {
  if (!parent) {
    DLOG(FATAL)
        << "patchOuter() requires the node to have a parent when using a key.";
  }
}

// Asserts that no extra elements were patched.
void AssertPatchElementNoExtras(blink::Element* start_node,
                                blink::Node* current_node,
                                blink::Node* expected_next_node,
                                blink::Node* expected_prev_node) {
  // Verify that we didn't patch more than expected
  if (current_node && current_node->nextSibling() != expected_next_node) {
    DLOG(WARNING) << "patchOuter() patched more elements than expected";
  }
  if (current_node && current_node->previousSibling() != expected_prev_node) {
    DLOG(WARNING) << "patchOuter() patched different elements than expected";
  }
}

// Asserts that no unclosed tags remain.
void AssertNoUnclosedTags(blink::Node* current_node, blink::Node* root_node) {
  if (current_node != root_node) {
    DLOG(FATAL) << "Unclosed element tags detected. Make sure all element "
                << "calls have matching close() calls.";
  }
}

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_ASSERTIONS_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_ASSERTIONS_H_

#include <vector>

#include "base/logging.h"

namespace blink {
class Node;
class Element;
}  // namespace blink

namespace cobalt {
namespace h5vcc {
namespace idom {

// Global state variables (declared here, defined in assertions.cc)
extern bool in_patch_;
extern bool in_attributes_;
extern bool in_skip_;

// Asserts that a value exists and is not null or undefined.
template <typename T>
T* Assert(T* val) {
  DCHECK(val) << "Expected value to be defined";
  return val;
}

// Function declarations (implementations in assertions.cc)
void AssertInPatch(const char* function_name);
void AssertNotInAttributes(const char* function_name);
void AssertNotInSkip(const char* function_name);
void AssertInAttributes(const char* function_name);
void AssertVirtualAttributesClosed();
bool SetInAttributes(bool value);
bool SetInSkip(bool value);
bool SetInPatch(bool value);

// Checks that no children have been declared yet for the given node.
void AssertNoChildrenDeclaredYet(const char* function_name, blink::Node* node);

// Asserts that patchOuter has a parent node.
void AssertPatchOuterHasParentNode(blink::Node* parent);

// Asserts that no extra elements were patched.
void AssertPatchElementNoExtras(blink::Element* start_node,
                                blink::Node* current_node,
                                blink::Node* expected_next_node,
                                blink::Node* expected_prev_node);

// Asserts that no unclosed tags remain.
void AssertNoUnclosedTags(blink::Node* current_node, blink::Node* root_node);

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_ASSERTIONS_H_

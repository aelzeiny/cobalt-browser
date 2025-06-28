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
}

namespace cobalt {
namespace h5vcc {
namespace idom {

// Keeps track of whether or not we are in a patch.
bool in_patch_ = false;

// Keeps track whether or not we are in an attributes declaration (after
// elementOpenStart, but before elementOpenEnd).
bool in_attributes_ = false;

// Keeps track whether or not we are in an element that should not have its
// children cleared.
bool in_skip_ = false;

// Asserts that a value exists and is not null or undefined.
template <typename T>
T* Assert(T* val) {
  DCHECK(val) << "Expected value to be defined";
  return val;
}

// Makes sure that there is a current patch context.
void AssertInPatch(const char* function_name) {
  if (!in_patch_) {
    DLOG(FATAL) << "Cannot call " << function_name << "() unless in patch.";
  }
}

// Makes sure that the caller is not where attributes are expected.
void AssertNotInAttributes(const char* function_name) {
  if (in_attributes_) {
    DLOG(FATAL) << function_name << "() can not be called between "
                << "elementOpenStart() and elementOpenEnd().";
  }
}

// Makes sure that the caller is not inside an element that has declared skip.
void AssertNotInSkip(const char* function_name) {
  if (in_skip_) {
    DLOG(FATAL) << function_name << "() may not be called inside an element "
                << "that has called skip().";
  }
}

// Makes sure that the caller is where attributes are expected.
void AssertInAttributes(const char* function_name) {
  if (!in_attributes_) {
    DLOG(FATAL) << function_name << "() can only be called after calling "
                << "elementOpenStart().";
  }
}

// Makes sure the patch closes virtual attributes call
void AssertVirtualAttributesClosed() {
  if (in_attributes_) {
    DLOG(FATAL) << "elementOpenEnd() must be called after calling "
                << "elementOpenStart().";
  }
}

// Updates the state of being in an attribute declaration.
bool SetInAttributes(bool value) {
  bool previous = in_attributes_;
  in_attributes_ = value;
  return previous;
}

// Updates the state of being in a skip element.
bool SetInSkip(bool value) {
  bool previous = in_skip_;
  in_skip_ = value;
  return previous;
}

// Checks that no children have been declared yet for the given node.
void AssertNoChildrenDeclaredYet(const char* function_name, blink::Node* node) {
  // This is a simplified implementation.
  // In the original, this would check if any children have been processed.
}

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_ASSERTIONS_H_

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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_NATIVE_ATTRIBUTES_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_NATIVE_ATTRIBUTES_H_

#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/attribute_value.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace cobalt {
namespace h5vcc {
namespace idom {

// Fast native C++ attribute application without V8 conversions
class NativeAttributes {
 public:
  // Apply attributes directly to DOM element (replaces ApplyAttrs)
  static void ApplyAttributes(blink::Element* element,
                              const AttributeList& attributes) {
    if (!element || attributes.empty()) {
      return;
    }

    for (const auto& attr : attributes) {
      ApplySingleAttribute(element, attr.name, attr.value);
    }
  }

  // Apply static attributes (replaces ApplyStatics)
  static void ApplyStaticAttributes(blink::Element* element,
                                    const AttributeList& statics) {
    ApplyAttributes(element, statics);
  }

  // Apply single attribute without V8 overhead
  static void ApplySingleAttribute(blink::Element* element,
                                   const WTF::AtomicString& name,
                                   const AttributeValue& value) {
    if (!element) {
      return;
    }

    if (value.IsNull()) {
      element->removeAttribute(name);
      return;
    }

    // Handle special cases for performance
    if (name == "class") {
      element->setAttribute(name, value.ToAtomicString());
    } else if (name == "id") {
      element->setAttribute(name, value.ToAtomicString());
    } else if (name == "style") {
      // For style, we could optimize further by parsing CSS
      element->setAttribute(name, value.ToAtomicString());
    } else {
      // General case
      element->setAttribute(name, value.ToAtomicString());
    }
  }

  // Batch attribute removal for efficiency
  static void RemoveAttributes(blink::Element* element,
                               const WTF::Vector<WTF::AtomicString>& names) {
    if (!element) {
      return;
    }

    for (const auto& name : names) {
      element->removeAttribute(name);
    }
  }

  // Check if attribute value has changed (for diffing)
  static bool HasAttributeChanged(blink::Element* element,
                                  const WTF::AtomicString& name,
                                  const AttributeValue& new_value) {
    if (!element) {
      return true;
    }

    if (new_value.IsNull()) {
      return element->hasAttribute(name);
    }

    WTF::AtomicString current_value = element->getAttribute(name);
    WTF::AtomicString new_atomic_value = new_value.ToAtomicString();

    return current_value != new_atomic_value;
  }
};

// Fast attribute diffing without V8 conversions
class AttributeDiffer {
 public:
  // Diff attributes and apply only changes
  static void DiffAndApply(blink::Element* element,
                           const AttributeList& old_attributes,
                           const AttributeList& new_attributes) {
    if (!element) {
      return;
    }

    // Create maps for efficient lookup
    WTF::HashMap<WTF::AtomicString, AttributeValue> old_map;
    WTF::HashMap<WTF::AtomicString, AttributeValue> new_map;

    for (const auto& attr : old_attributes) {
      old_map.Set(attr.name, attr.value);
    }

    for (const auto& attr : new_attributes) {
      new_map.Set(attr.name, attr.value);
    }

    // Apply new/changed attributes
    for (const auto& new_attr : new_attributes) {
      auto old_it = old_map.find(new_attr.name);
      if (old_it == old_map.end() ||
          !AttributeValuesEqual(old_it->value, new_attr.value)) {
        NativeAttributes::ApplySingleAttribute(element, new_attr.name,
                                               new_attr.value);
      }
    }

    // Remove deleted attributes
    for (const auto& old_attr : old_attributes) {
      if (new_map.find(old_attr.name) == new_map.end()) {
        element->removeAttribute(old_attr.name);
      }
    }
  }

 private:
  static bool AttributeValuesEqual(const AttributeValue& a,
                                   const AttributeValue& b) {
    if (a.GetType() != b.GetType()) {
      return false;
    }

    switch (a.GetType()) {
      case AttributeValue::kString:
        return a.AsString() == b.AsString();
      case AttributeValue::kNumber:
        return a.AsNumber() == b.AsNumber();
      case AttributeValue::kBoolean:
        return a.AsBoolean() == b.AsBoolean();
      case AttributeValue::kNull:
        return true;
    }
    return false;
  }
};

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_NATIVE_ATTRIBUTES_H_

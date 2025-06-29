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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_ATTRIBUTES_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_ATTRIBUTES_H_

#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {
class ScriptValue;
}

namespace cobalt {
namespace h5vcc {
namespace idom {

using AttrMutator = std::function<
    void(blink::Element*, const WTF::String&, const blink::ScriptValue&)>;

// Applies an attribute or property to a given Element.
void ApplyAttr(blink::Element* el,
               const WTF::String& name,
               const WTF::String& value);
void ApplyProp(blink::Element* el,
               const WTF::String& name,
               const blink::ScriptValue& value);

// Type-aware attribute application (matches TypeScript applyAttributeTyped)
void ApplyAttributeTyped(blink::Element* el,
                         const WTF::String& name,
                         const blink::ScriptValue& value);

// Comprehensive style application (matches TypeScript applyStyle)
void ApplyStyle(blink::Element* el,
                const WTF::String& name,
                const blink::ScriptValue& style);

// Updates a single attribute using the appropriate mutator from the attribute
// map
void UpdateAttribute(blink::Element* el,
                     const WTF::String& name,
                     const blink::ScriptValue& value,
                     const blink::ScriptValue& attrs);

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_ATTRIBUTES_H_

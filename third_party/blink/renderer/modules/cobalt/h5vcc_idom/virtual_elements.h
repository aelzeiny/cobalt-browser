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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_VIRTUAL_ELEMENTS_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_VIRTUAL_ELEMENTS_H_

#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/text.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_vector.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class V8VoidCallback;
class IDomNodeData;

namespace virtual_elements {

/**
 * The offset in the virtual element declaration where the attributes are
 * specified.
 */
static const int ATTRIBUTES_OFFSET = 3;

/**
 * Buffers an attribute, which will get applied during the next call to
 * `elementOpen`, `elementOpenEnd` or `applyAttrs`.
 * @param attrs_builder The attributes builder to append to.
 * @param name The name of the attribute to buffer.
 * @param value The value of the attribute to buffer.
 */
void Attr(HeapVector<ScriptValue>& attrs_builder,
          const String& name,
          const ScriptValue& value);

/**
 * Allows you to define a key after an elementOpenStart. This is useful in
 * templates that define key after an element has been opened ie
 * `<div key('foo')></div>`.
 * @param args_builder The arguments builder to modify.
 * @param key The key to use for the next call.
 */
void Key(HeapVector<ScriptValue>& args_builder, const String& key);

/**
 * Applies the currently buffered attrs to the currently open element. This
 * clears the buffered attributes.
 * @param attrs_builder The attributes builder to apply and clear.
 * @param current_element The current element to apply attributes to.
 */
void ApplyAttrs(HeapVector<ScriptValue>& attrs_builder,
                Element* current_element);

/**
 * Applies the current static attributes to the currently open element. Note:
 * statics should be applied before calling `applyAttrs`.
 * @param statics The statics to apply to the current element.
 * @param current_element The current element to apply statics to.
 */
void ApplyStatics(const HeapVector<ScriptValue>& statics,
                  Element* current_element);

/**
 * Declares a virtual Element at the current location in the document. This
 * corresponds to an opening tag and a elementClose tag is required. This is
 * like elementOpen, but the attributes are defined using the attr function
 * rather than being passed as arguments. Must be followed by 0 or more calls
 * to attr, then a call to elementOpenEnd.
 * @param args_builder The arguments builder to initialize.
 * @param name_or_ctor The Element's tag or constructor.
 * @param key The key used to identify this element. This can be an
 *     empty string, but performance may be better if a unique value is used
 *     when iterating over an array of items.
 * @param statics An array of attribute name/value pairs of the static
 *     attributes for the Element. Attributes will only be set once when the
 *     Element is created.
 */
void ElementOpenStart(
    HeapVector<ScriptValue>& args_builder,
    const String& name_or_ctor,
    const String& key = String(),
    const absl::optional<HeapVector<ScriptValue>>& statics = absl::nullopt);

/**
 * Closes an open tag started with elementOpenStart.
 * @param args_builder The arguments builder with element info.
 * @param attrs_builder The attributes builder to apply.
 * @param open_func Function to call to open the element.
 * @return The corresponding Element.
 */
Element* ElementOpenEnd(
    HeapVector<ScriptValue>& args_builder,
    HeapVector<ScriptValue>& attrs_builder,
    std::function<Element*(const String&, const String&)> open_func);

/**
 * Declares a virtual Element at the current location in the document.
 * @param args_builder The arguments builder to use temporarily.
 * @param attrs_builder The attributes builder to use temporarily.
 * @param name_or_ctor The Element's tag or constructor.
 * @param key The key used to identify this element.
 * @param statics An array of attribute name/value pairs of the static
 *     attributes for the Element.
 * @param var_args Attribute name/value pairs of the dynamic attributes
 *     for the Element.
 * @param open_func Function to call to open the element.
 * @return The corresponding Element.
 */
Element* ElementOpen(
    HeapVector<ScriptValue>& args_builder,
    HeapVector<ScriptValue>& attrs_builder,
    const String& name_or_ctor,
    const String& key = String(),
    const absl::optional<HeapVector<ScriptValue>>& statics = absl::nullopt,
    const HeapVector<ScriptValue>& var_args = HeapVector<ScriptValue>(),
    std::function<Element*(const String&, const String&)> open_func = nullptr);

/**
 * Closes an open virtual Element.
 * @param name_or_ctor The Element's tag or constructor.
 * @param close_func Function to call to close the element.
 * @return The corresponding Element.
 */
Element* ElementClose(const String& name_or_ctor,
                      std::function<Element*()> close_func);

/**
 * Declares a virtual Element at the current location in the document that has
 * no children.
 * @param args_builder The arguments builder to use temporarily.
 * @param attrs_builder The attributes builder to use temporarily.
 * @param name_or_ctor The Element's tag or constructor.
 * @param key The key used to identify this element.
 * @param statics An array of attribute name/value pairs of the static
 *     attributes for the Element.
 * @param var_args Attribute name/value pairs of the dynamic attributes
 *     for the Element.
 * @param open_func Function to call to open the element.
 * @param close_func Function to call to close the element.
 * @return The corresponding Element.
 */
Element* ElementVoid(
    HeapVector<ScriptValue>& args_builder,
    HeapVector<ScriptValue>& attrs_builder,
    const String& name_or_ctor,
    const String& key = String(),
    const absl::optional<HeapVector<ScriptValue>>& statics = absl::nullopt,
    const HeapVector<ScriptValue>& var_args = HeapVector<ScriptValue>(),
    std::function<Element*(const String&, const String&)> open_func = nullptr,
    std::function<Element*()> close_func = nullptr);

/**
 * Declares a virtual Text at this point in the document with value change
 * detection.
 * @param value The value of the Text.
 * @param formatters Functions to format the value which are called only when
 *     the value has changed.
 * @param text_func Function to get the text node.
 * @param get_data_func Function to get node data.
 * @return The corresponding text node.
 */
Text* TextWithValue(const ScriptValue& value,
                    const HeapVector<Member<V8VoidCallback>>& formatters,
                    std::function<Text*()> text_func,
                    std::function<IDomNodeData*(Node*)> get_data_func);

}  // namespace virtual_elements
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_VIRTUAL_ELEMENTS_H_

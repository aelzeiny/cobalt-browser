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

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/native_patch.h"

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/core.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/native_virtual_elements.h"

namespace cobalt {
namespace h5vcc {
namespace idom {

// NativePatchContext implementation
blink::Element* NativePatchContext::ElementOpen(const char* tag_name,
                                                const char* key) {
  WTF::AtomicString atomic_tag(tag_name);
  WTF::String string_key = key ? WTF::String(key) : WTF::String();
  return ElementOpen(atomic_tag, string_key);
}

blink::Element* NativePatchContext::ElementOpen(
    const WTF::AtomicString& tag_name,
    const WTF::String& key) {
  blink::Element* element = NativeVirtualElements::ElementOpen(
      data_map_, tag_name, key, AttributeList(), attr_builder_.GetAttributes());

  // Clear attributes after use
  attr_builder_.Clear();
  return element;
}

blink::Element* NativePatchContext::ElementClose(const char* tag_name) {
  return ElementClose(WTF::AtomicString(tag_name));
}

blink::Element* NativePatchContext::ElementClose(
    const WTF::AtomicString& tag_name) {
  return NativeVirtualElements::ElementClose(tag_name);
}

blink::Element* NativePatchContext::ElementVoid(const char* tag_name,
                                                const char* key) {
  WTF::AtomicString atomic_tag(tag_name);
  WTF::String string_key = key ? WTF::String(key) : WTF::String();
  return ElementVoid(atomic_tag, string_key);
}

blink::Element* NativePatchContext::ElementVoid(
    const WTF::AtomicString& tag_name,
    const WTF::String& key) {
  blink::Element* element = NativeVirtualElements::ElementVoid(
      data_map_, tag_name, key, AttributeList(), attr_builder_.GetAttributes());

  // Clear attributes after use
  attr_builder_.Clear();
  return element;
}

// Text operations
blink::Text* NativePatchContext::Text(const char* value) {
  return Text(WTF::String(value));
}

blink::Text* NativePatchContext::Text(const WTF::String& value) {
  return NativeVirtualElements::TextWithValue(data_map_, value);
}

blink::Text* NativePatchContext::Text(double value) {
  return Text(WTF::String::Number(value));
}

blink::Text* NativePatchContext::Text(bool value) {
  return Text(value ? "true" : "false");
}

// Attribute operations
void NativePatchContext::Attr(const char* name, const char* value) {
  WTF::AtomicString atomic_name(name);
  WTF::String string_value(value);
  attr_builder_.AddAttribute(atomic_name, string_value);
}

void NativePatchContext::Attr(const char* name, const WTF::String& value) {
  WTF::AtomicString atomic_name(name);
  attr_builder_.AddAttribute(atomic_name, value);
}

void NativePatchContext::Attr(const char* name, double value) {
  WTF::AtomicString atomic_name(name);
  attr_builder_.AddAttribute(atomic_name, value);
}

void NativePatchContext::Attr(const char* name, bool value) {
  WTF::AtomicString atomic_name(name);
  attr_builder_.AddAttribute(atomic_name, value);
}

void NativePatchContext::Attr(const WTF::AtomicString& name,
                              const AttributeValue& value) {
  attr_builder_.AddAttribute(name, value);
}

// Current element access
blink::Element* NativePatchContext::CurrentElement() {
  return ::cobalt::h5vcc::idom::CurrentElement();
}

blink::Node* NativePatchContext::CurrentPointer() {
  return ::cobalt::h5vcc::idom::CurrentPointer();
}

void NativePatchContext::Skip() {
  ::cobalt::h5vcc::idom::Skip();
}

// OPTIMIZED: NativePatch implementation with proper incremental DOM logic
blink::Node* NativePatch::PatchInner(NodeDataMap& data_map,
                                     blink::Element* element,
                                     const NativePatchFunction& patch_fn) {
  if (!element || !patch_fn) {
    return nullptr;
  }

  // OPTIMIZED: Minimal patch context setup without expensive copying
  SetCurrentDataMap(&data_map);

  // Create optimized native context and run patch function
  NativePatchContext native_ctx(data_map);
  patch_fn(native_ctx);

  // Minimal state restoration
  SetCurrentDataMap(nullptr);

  return element;
}

blink::Node* NativePatch::PatchOuter(NodeDataMap& data_map,
                                     blink::Element* element,
                                     const NativePatchFunction& patch_fn) {
  if (!element || !patch_fn) {
    return nullptr;
  }

  // OPTIMIZED: Set up minimal patch context for outer patching
  SetCurrentDataMap(&data_map);

  // Create optimized native context and run patch function
  NativePatchContext native_ctx(data_map);
  patch_fn(native_ctx);

  // Get the result - may be a different element if replaced
  blink::Node* result = GetNextNode();
  if (!result || result == element) {
    result = element;
  }

  // Minimal state restoration
  SetCurrentDataMap(nullptr);

  return result;
}

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

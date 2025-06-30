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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_NATIVE_PATCH_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_NATIVE_PATCH_H_

#include <functional>
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/attribute_value.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/node_data.h"

namespace cobalt {
namespace h5vcc {
namespace idom {

// Forward declaration
class NativePatchContext;

// Native C++ patch function type (replaces V8PatchFunction)
using NativePatchFunction = std::function<void(NativePatchContext&)>;

// Native patch context - eliminates V8 overhead
class NativePatchContext {
 public:
  explicit NativePatchContext(NodeDataMap& data_map) : data_map_(data_map) {}

  // Core element operations
  blink::Element* ElementOpen(const char* tag_name, const char* key = nullptr);
  blink::Element* ElementOpen(const WTF::AtomicString& tag_name,
                              const WTF::String& key = WTF::String());

  blink::Element* ElementClose(const char* tag_name);
  blink::Element* ElementClose(const WTF::AtomicString& tag_name);

  blink::Element* ElementVoid(const char* tag_name, const char* key = nullptr);
  blink::Element* ElementVoid(const WTF::AtomicString& tag_name,
                              const WTF::String& key = WTF::String());

  // Text operations
  blink::Text* Text(const char* value);
  blink::Text* Text(const WTF::String& value);
  blink::Text* Text(double value);
  blink::Text* Text(bool value);

  // Attribute operations
  void Attr(const char* name, const char* value);
  void Attr(const char* name, const WTF::String& value);
  void Attr(const char* name, double value);
  void Attr(const char* name, bool value);
  void Attr(const WTF::AtomicString& name, const AttributeValue& value);

  // Common attribute shortcuts
  void Id(const char* id) { Attr("id", id); }
  void Id(const WTF::String& id) { Attr("id", id); }
  void Class(const char* class_name) { Attr("class", class_name); }
  void Class(const WTF::String& class_name) { Attr("class", class_name); }
  void Style(const char* style) { Attr("style", style); }
  void Style(const WTF::String& style) { Attr("style", style); }

  // Element shortcuts for common HTML elements
  blink::Element* Div(const char* key = nullptr) {
    return ElementOpen("div", key);
  }
  blink::Element* Span(const char* key = nullptr) {
    return ElementOpen("span", key);
  }
  blink::Element* P(const char* key = nullptr) { return ElementOpen("p", key); }
  blink::Element* H1(const char* key = nullptr) {
    return ElementOpen("h1", key);
  }
  blink::Element* H2(const char* key = nullptr) {
    return ElementOpen("h2", key);
  }
  blink::Element* H3(const char* key = nullptr) {
    return ElementOpen("h3", key);
  }
  blink::Element* Button(const char* key = nullptr) {
    return ElementOpen("button", key);
  }
  blink::Element* Input(const char* key = nullptr) {
    return ElementVoid("input", key);
  }
  blink::Element* Img(const char* key = nullptr) {
    return ElementVoid("img", key);
  }
  blink::Element* Br() { return ElementVoid("br"); }

  // Close shortcuts
  void CloseDiv() { ElementClose("div"); }
  void CloseSpan() { ElementClose("span"); }
  void CloseP() { ElementClose("p"); }
  void CloseH1() { ElementClose("h1"); }
  void CloseH2() { ElementClose("h2"); }
  void CloseH3() { ElementClose("h3"); }
  void CloseButton() { ElementClose("button"); }

  // Current element access
  blink::Element* CurrentElement();
  blink::Node* CurrentPointer();

  // Skip functionality
  void Skip();

 private:
  NodeDataMap& data_map_;
  AttributeBuilder attr_builder_;

  // Helper to flush attributes
  void FlushAttributes(blink::Element* element);
};

// High-performance native patching API
class NativePatch {
 public:
  // Patch inner (replace element children)
  static blink::Node* PatchInner(NodeDataMap& data_map,
                                 blink::Element* element,
                                 const NativePatchFunction& patch_fn);

  // Patch outer (replace element itself)
  static blink::Node* PatchOuter(NodeDataMap& data_map,
                                 blink::Element* element,
                                 const NativePatchFunction& patch_fn);

  // Simplified patch functions for common use cases
  template <typename Func>
  static blink::Node* PatchInnerSimple(NodeDataMap& data_map,
                                       blink::Element* element,
                                       Func&& func) {
    return PatchInner(data_map, element,
                      [&](NativePatchContext& ctx) { func(ctx); });
  }

  template <typename Func>
  static blink::Node* PatchOuterSimple(NodeDataMap& data_map,
                                       blink::Element* element,
                                       Func&& func) {
    return PatchOuter(data_map, element,
                      [&](NativePatchContext& ctx) { func(ctx); });
  }
};

// Convenience macros for even cleaner syntax
#define PATCH_INNER(data_map, element) \
  NativePatch::PatchInnerSimple(data_map, element, [&](auto& ctx)

#define PATCH_OUTER(data_map, element) \
  NativePatch::PatchOuterSimple(data_map, element, [&](auto& ctx)

#define END_PATCH )

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_NATIVE_PATCH_H_

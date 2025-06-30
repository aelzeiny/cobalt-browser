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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_NATIVE_VIRTUAL_ELEMENTS_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_NATIVE_VIRTUAL_ELEMENTS_H_

#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/text.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/attribute_value.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/native_attributes.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/node_data.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace cobalt {
namespace h5vcc {
namespace idom {

// Forward declarations for core functions
blink::Element* Open(NodeDataMap& data_map,
                     const WTF::String& name_or_ctor,
                     const WTF::String& key,
                     const WTF::String& nonce);
blink::Element* Close();
blink::Text* Text();

// Native C++ virtual elements implementation without V8 overhead
class NativeVirtualElements {
 public:
  // Native argument builder (replaces HeapVector<ScriptValue>)
  struct ElementArgs {
    WTF::AtomicString name_or_ctor;
    WTF::String key;
    AttributeList statics;
    WTF::String nonce;

    void Clear() {
      name_or_ctor = WTF::AtomicString();
      key = WTF::String();
      statics.clear();
      nonce = WTF::String();
    }
  };

  // Add attribute without V8 conversion (replaces Attr)
  static void AddAttribute(AttributeBuilder& builder,
                           const WTF::AtomicString& name,
                           const AttributeValue& value) {
    builder.AddAttribute(name, value);
  }

  // Convenience overloads for common types
  static void AddAttribute(AttributeBuilder& builder,
                           const WTF::AtomicString& name,
                           const WTF::String& value) {
    builder.AddAttribute(name, value);
  }

  static void AddAttribute(AttributeBuilder& builder,
                           const WTF::AtomicString& name,
                           const char* value) {
    builder.AddAttribute(name, WTF::String(value));
  }

  static void AddAttribute(AttributeBuilder& builder,
                           const WTF::AtomicString& name,
                           double value) {
    builder.AddAttribute(name, value);
  }

  static void AddAttribute(AttributeBuilder& builder,
                           const WTF::AtomicString& name,
                           bool value) {
    builder.AddAttribute(name, value);
  }

  // Set key for element (replaces Key function)
  static void SetKey(ElementArgs& args, const WTF::String& key) {
    args.key = key;
  }

  // Apply attributes directly (replaces ApplyAttrs)
  static void ApplyAttributes(blink::Element* element,
                              const AttributeBuilder& builder) {
    NativeAttributes::ApplyAttributes(element, builder.GetAttributes());
  }

  // Apply static attributes (replaces ApplyStatics)
  static void ApplyStaticAttributes(blink::Element* element,
                                    const AttributeList& statics) {
    NativeAttributes::ApplyStaticAttributes(element, statics);
  }

  // Element open start (replaces ElementOpenStart)
  static void ElementOpenStart(ElementArgs& args,
                               const WTF::AtomicString& name_or_ctor,
                               const WTF::String& key = WTF::String(),
                               const AttributeList& statics = AttributeList()) {
    args.name_or_ctor = name_or_ctor;
    args.key = key;
    args.statics = statics;
  }

  // Element open end (replaces ElementOpenEnd)
  static blink::Element* ElementOpenEnd(NodeDataMap& data_map,
                                        const ElementArgs& args,
                                        AttributeBuilder& attrs_builder) {
    blink::Element* element =
        Open(data_map, args.name_or_ctor.GetString(), args.key, args.nonce);
    if (!element) {
      return nullptr;
    }

    // Apply statics first
    ApplyStaticAttributes(element, args.statics);

    // Apply dynamic attributes
    ApplyAttributes(element, attrs_builder);

    // Clear builder for next use
    attrs_builder.Clear();

    return element;
  }

  // Full element open in one call (replaces ElementOpen)
  static blink::Element* ElementOpen(
      NodeDataMap& data_map,
      const WTF::AtomicString& name_or_ctor,
      const WTF::String& key = WTF::String(),
      const AttributeList& statics = AttributeList(),
      const AttributeList& dynamic_attrs = AttributeList()) {
    ElementArgs args;
    ElementOpenStart(args, name_or_ctor, key, statics);

    AttributeBuilder attrs_builder;
    for (const auto& attr : dynamic_attrs) {
      attrs_builder.AddAttribute(attr.name, attr.value);
    }

    return ElementOpenEnd(data_map, args, attrs_builder);
  }

  // Element close (replaces ElementClose)
  static blink::Element* ElementClose(const WTF::AtomicString& expected_name) {
    blink::Element* element = Close();

    // In debug mode, we could validate the tag matches
    // For now, just return the closed element
    return element;
  }

  // Element void (open + close in one call)
  static blink::Element* ElementVoid(
      NodeDataMap& data_map,
      const WTF::AtomicString& name_or_ctor,
      const WTF::String& key = WTF::String(),
      const AttributeList& statics = AttributeList(),
      const AttributeList& dynamic_attrs = AttributeList()) {
    blink::Element* element =
        ElementOpen(data_map, name_or_ctor, key, statics, dynamic_attrs);
    if (element) {
      ElementClose(name_or_ctor);
    }
    return element;
  }

  // Text node with native value handling (replaces TextWithValue)
  static blink::Text* TextWithValue(NodeDataMap& data_map,
                                    const WTF::String& value) {
    blink::Text* node = Text();
    if (!node) {
      return nullptr;
    }

    // Simple text comparison and update
    if (node->data() != value) {
      node->setData(value);
    }

    return node;
  }

  // Convenience overloads for text
  static blink::Text* TextWithValue(NodeDataMap& data_map, const char* value) {
    return TextWithValue(data_map, WTF::String(value));
  }

  static blink::Text* TextWithValue(NodeDataMap& data_map, double value) {
    return TextWithValue(data_map, WTF::String::Number(value));
  }

  static blink::Text* TextWithValue(NodeDataMap& data_map, bool value) {
    return TextWithValue(data_map, value ? "true" : "false");
  }
};

// Helper class for building elements fluently
class ElementBuilder {
 public:
  explicit ElementBuilder(const WTF::AtomicString& tag_name)
      : args_(), attrs_builder_() {
    args_.name_or_ctor = tag_name;
  }

  ElementBuilder& Key(const WTF::String& key) {
    args_.key = key;
    return *this;
  }

  ElementBuilder& Attr(const WTF::AtomicString& name,
                       const AttributeValue& value) {
    attrs_builder_.AddAttribute(name, value);
    return *this;
  }

  ElementBuilder& Attr(const WTF::AtomicString& name,
                       const WTF::String& value) {
    attrs_builder_.AddAttribute(name, value);
    return *this;
  }

  ElementBuilder& Attr(const WTF::AtomicString& name, const char* value) {
    attrs_builder_.AddAttribute(name, WTF::String(value));
    return *this;
  }

  ElementBuilder& Attr(const WTF::AtomicString& name, double value) {
    attrs_builder_.AddAttribute(name, value);
    return *this;
  }

  ElementBuilder& Attr(const WTF::AtomicString& name, bool value) {
    attrs_builder_.AddAttribute(name, value);
    return *this;
  }

  // Convenience overloads for const char*
  ElementBuilder& Attr(const char* name, const char* value) {
    WTF::AtomicString atomic_name(name);
    WTF::String string_value(value);
    attrs_builder_.AddAttribute(atomic_name, string_value);
    return *this;
  }

  ElementBuilder& Attr(const char* name, const WTF::String& value) {
    WTF::AtomicString atomic_name(name);
    attrs_builder_.AddAttribute(atomic_name, value);
    return *this;
  }

  ElementBuilder& StaticAttr(const WTF::AtomicString& name,
                             const AttributeValue& value) {
    args_.statics.emplace_back(name, value);
    return *this;
  }

  // Common attribute shortcuts
  ElementBuilder& Id(const WTF::String& id);
  ElementBuilder& Class(const WTF::String& class_name);
  ElementBuilder& Style(const WTF::String& style);
  ElementBuilder& Href(const WTF::String& href);
  ElementBuilder& Src(const WTF::String& src);
  ElementBuilder& Type(const WTF::String& type);
  ElementBuilder& Value(const WTF::String& value);
  ElementBuilder& Disabled(bool disabled);

  blink::Element* Open(NodeDataMap& data_map) {
    return NativeVirtualElements::ElementOpenEnd(data_map, args_,
                                                 attrs_builder_);
  }

  blink::Element* Void(NodeDataMap& data_map) {
    blink::Element* element = Open(data_map);
    if (element) {
      NativeVirtualElements::ElementClose(args_.name_or_ctor);
    }
    return element;
  }

 private:
  NativeVirtualElements::ElementArgs args_;
  AttributeBuilder attrs_builder_;
};

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_NATIVE_VIRTUAL_ELEMENTS_H_

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

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/attributes.h"

#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/bindings/core/v8/to_v8_traits.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_attr_mutator.h"
#include "third_party/blink/renderer/core/css/css_style_declaration.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/qualified_name.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/core/svg/svg_element.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace cobalt {
namespace h5vcc {
namespace idom {

namespace {

WTF::String GetNamespace(const WTF::String& name) {
  if (name.StartsWith("xml:")) {
    return "http://www.w3.org/XML/1998/namespace";
  }
  if (name.StartsWith("xlink:")) {
    return "http://www.w3.org/1999/xlink";
  }
  return WTF::String();
}

}  // namespace

void ApplyAttr(blink::Element* el,
               const WTF::String& name,
               const WTF::String& value) {
  if (value.IsNull()) {
    blink::QualifiedName qname(g_null_atom, AtomicString(name), g_null_atom);
    el->removeAttribute(qname);
  } else {
    const WTF::String& attr_ns = GetNamespace(name);
    if (!attr_ns.IsNull()) {
      el->setAttributeNS(AtomicString(attr_ns), AtomicString(name), value,
                         ASSERT_NO_EXCEPTION);
    } else {
      blink::QualifiedName qname(g_null_atom, AtomicString(name), g_null_atom);
      el->setAttribute(qname, AtomicString(value));
    }
  }
}

void ApplyProp(blink::Element* el,
               const WTF::String& name,
               const blink::ScriptValue& value) {
  blink::LocalFrame* frame = el->GetDocument().GetFrame();
  if (!frame) {
    return;
  }
  v8::Isolate* isolate = blink::ToIsolate(frame);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  blink::ScriptState* script_state = blink::ScriptState::From(context);
  if (!script_state) {
    return;
  }

  v8::MaybeLocal<v8::Value> maybe_v8_value =
      blink::ToV8Traits<blink::Element>::ToV8(script_state, el);
  v8::Local<v8::Value> v8_element_value;
  if (!maybe_v8_value.ToLocal(&v8_element_value) ||
      !v8_element_value->IsObject()) {
    return;
  }
  v8::Local<v8::Object> v8_object = v8_element_value.As<v8::Object>();

  // Use the ScriptValue directly - it already contains the V8 value
  v8::Local<v8::Value> v8_value = value.V8Value();
  v8::Local<v8::String> v8_name =
      v8::String::NewFromUtf8(isolate, name.Utf8().c_str()).ToLocalChecked();

  v8::Maybe<bool> result = v8_object->Set(context, v8_name, v8_value);
  (void)result;
}

namespace {

void SetStyleValue(blink::CSSStyleDeclaration* style,
                   const WTF::String& prop,
                   const WTF::String& value) {
  if (prop.Contains("-")) {
    style->setProperty(prop, value, WTF::String(), ASSERT_NO_EXCEPTION);
  } else {
    style->SetPropertyInternal(
        blink::CSSPropertyID::kInvalid, prop, value, false,
        blink::SecureContextMode::kInsecureContext, nullptr);
  }
}

}  // namespace

void ApplyAttributeTyped(blink::Element* el,
                         const WTF::String& name,
                         const blink::ScriptValue& value) {
  v8::Local<v8::Value> v8_value = value.V8Value();

  // Check the type of the value
  if (v8_value->IsObject() || v8_value->IsFunction()) {
    // For objects and functions, apply as property
    ApplyProp(el, name, value);
  } else {
    // For primitives, apply as attribute
    v8::String::Utf8Value value_utf8(value.GetIsolate(), v8_value);
    WTF::String string_value(*value_utf8);
    ApplyAttr(el, name, string_value);
  }
}

void ApplyStyle(blink::Element* el,
                const WTF::String& name,
                const blink::ScriptValue& style) {
  // Check if element has a style property (HTMLElement or SVGElement)
  blink::HTMLElement* html_element = blink::DynamicTo<blink::HTMLElement>(el);
  blink::SVGElement* svg_element = blink::DynamicTo<blink::SVGElement>(el);

  blink::CSSStyleDeclaration* el_style = nullptr;
  if (html_element) {
    el_style = &html_element->style();
  } else if (svg_element) {
    el_style = &svg_element->style();
  }

  if (!el_style) {
    return;  // Element doesn't support style
  }

  v8::Local<v8::Value> v8_style = style.V8Value();

  if (v8_style->IsString()) {
    // Handle string CSS
    v8::String::Utf8Value css_text_utf8(style.GetIsolate(), v8_style);
    WTF::String css_text(*css_text_utf8);
    el_style->setCSSText(css_text);
  } else if (v8_style->IsObject()) {
    // Handle object with property-value pairs
    el_style->setCSSText("");  // Clear existing styles

    v8::Local<v8::Object> style_obj = v8_style.As<v8::Object>();
    v8::Local<v8::Context> context = style.GetIsolate()->GetCurrentContext();

    v8::Local<v8::Array> property_names;
    if (style_obj->GetOwnPropertyNames(context).ToLocal(&property_names)) {
      for (uint32_t i = 0; i < property_names->Length(); i++) {
        v8::Local<v8::Value> key;
        v8::Local<v8::Value> value;

        if (property_names->Get(context, i).ToLocal(&key) &&
            style_obj->Get(context, key).ToLocal(&value)) {
          v8::String::Utf8Value prop_utf8(style.GetIsolate(), key);
          v8::String::Utf8Value value_utf8(style.GetIsolate(), value);

          WTF::String prop(*prop_utf8);
          WTF::String val(*value_utf8);

          SetStyleValue(el_style, prop, val);
        }
      }
    }
  }
}

void UpdateAttribute(blink::Element* el,
                     const WTF::String& name,
                     const blink::ScriptValue& value,
                     const blink::ScriptValue& attrs) {
  v8::Isolate* isolate = value.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Value> attrs_v8 = attrs.V8Value();

  // Try to get specific mutator first
  v8::Local<v8::Value> mutator_value;
  if (attrs_v8->IsObject()) {
    v8::Local<v8::Object> attrs_obj = attrs_v8.As<v8::Object>();

    // First try the specific attribute name
    v8::Local<v8::String> v8_name =
        v8::String::NewFromUtf8(isolate, name.Utf8().c_str()).ToLocalChecked();

    if (!attrs_obj->Get(context, v8_name).ToLocal(&mutator_value) ||
        mutator_value->IsUndefined() || mutator_value->IsNull()) {
      // Fall back to __default mutator
      v8::Local<v8::String> default_key =
          v8::String::NewFromUtf8(isolate, "__default").ToLocalChecked();
      attrs_obj->Get(context, default_key).ToLocal(&mutator_value);
    }
  }

  // If we have a function mutator, call it
  if (!mutator_value.IsEmpty() && mutator_value->IsFunction()) {
    blink::ScriptState* script_state = blink::ScriptState::From(context);
    v8::Local<v8::Value> v8_element;
    if (blink::ToV8Traits<blink::Element>::ToV8(script_state, el)
            .ToLocal(&v8_element)) {
      v8::Local<v8::String> v8_name =
          v8::String::NewFromUtf8(isolate, name.Utf8().c_str())
              .ToLocalChecked();

      // Create arguments array
      v8::Local<v8::Value> args[] = {v8_element, v8_name, value.V8Value()};

      // Call the mutator function
      v8::Local<v8::Function> mutator_func = mutator_value.As<v8::Function>();
      mutator_func->Call(context, v8::Undefined(isolate), 3, args);
    }
  } else {
    // Fall back to default behavior (type-aware attribute application)
    ApplyAttributeTyped(el, name, value);
  }
}

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

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
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/core/svg/svg_element.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

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

bool IsValidAttributeName(const WTF::String& name) {
  // Empty names are invalid
  if (name.empty()) {
    return false;
  }

  // Check for invalid characters according to HTML spec
  // Attribute names cannot contain: space, tab, newline, form feed, /, >, =, "
  // and certain other special characters like !@#$%^&*()
  for (unsigned i = 0; i < name.length(); ++i) {
    UChar c = name[i];
    if (c <= 0x20 || c == '/' || c == '>' || c == '=' || c == '"' ||
        c == '\'' || c == '!' || c == '@' || c == '#' || c == '$' || c == '%' ||
        c == '^' || c == '&' || c == '*' || c == '(' || c == ')' || c == '+' ||
        c == '[' || c == ']' || c == '{' || c == '}' || c == '|' || c == '\\' ||
        c == '?' || c == '<' || c == '`' || c == '~') {
      return false;
    }
  }
  return true;
}

void ApplyAttr(blink::Element* el,
               const WTF::String& name,
               const WTF::String& value) {
  // Note: Attribute name validation is handled at the API level in
  // H5vccIdom::applyAttr

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

WTF::String CamelCaseToKebabCase(const WTF::String& camel_case) {
  WTF::StringBuilder kebab_case;
  for (unsigned i = 0; i < camel_case.length(); ++i) {
    UChar c = camel_case[i];
    if (c >= 'A' && c <= 'Z') {
      if (i > 0) {
        kebab_case.Append('-');
      }
      kebab_case.Append(c + ('a' - 'A'));  // Convert to lowercase
    } else {
      kebab_case.Append(c);
    }
  }
  return kebab_case.ToString();
}

void SetStyleValue(blink::CSSStyleDeclaration* style,
                   const WTF::String& prop,
                   const WTF::String& value,
                   blink::ExecutionContext* execution_context) {
  WTF::String css_prop;
  if (prop.Contains("-")) {
    // Already kebab-case
    css_prop = prop;
  } else {
    // Convert camelCase to kebab-case (backgroundColor -> background-color)
    css_prop = CamelCaseToKebabCase(prop);
  }
  style->setProperty(execution_context, css_prop, value, WTF::String(),
                     ASSERT_NO_EXCEPTION);
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
    el_style = html_element->style();
  } else if (svg_element) {
    el_style = svg_element->style();
  }

  if (!el_style) {
    return;  // Element doesn't support style
  }

  blink::ExecutionContext* execution_context = el->GetExecutionContext();
  if (!execution_context) {
    return;
  }

  v8::Local<v8::Value> v8_style = style.V8Value();

  if (v8_style->IsString()) {
    // Handle string CSS
    v8::String::Utf8Value css_text_utf8(style.GetIsolate(), v8_style);
    WTF::String css_text(*css_text_utf8);
    el_style->setCSSText(execution_context, css_text, ASSERT_NO_EXCEPTION);
  } else if (v8_style->IsObject()) {
    // Handle object with property-value pairs
    el_style->setCSSText(execution_context, "",
                         ASSERT_NO_EXCEPTION);  // Clear existing styles

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

          SetStyleValue(el_style, prop, val, execution_context);
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
      bool success =
          attrs_obj->Get(context, default_key).ToLocal(&mutator_value);
      (void)success;  // Suppress unused variable warning
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
      v8::MaybeLocal<v8::Value> result =
          mutator_func->Call(context, v8::Undefined(isolate), 3, args);
      (void)result;  // Suppress unused variable warning
    }
  } else {
    // Fall back to default behavior (type-aware attribute application)
    ApplyAttributeTyped(el, name, value);
  }
}

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

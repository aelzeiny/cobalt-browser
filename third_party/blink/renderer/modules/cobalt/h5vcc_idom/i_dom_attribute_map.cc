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

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_attribute_map.h"

#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_element.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_attr_mutator.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/attributes.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

namespace blink {

IDomAttributeMap::IDomAttributeMap() = default;

// static
ScriptValue IDomAttributeMap::CreateJavaScriptAttributeMap(
    ScriptState* script_state) {
  ScriptState::Scope scope(script_state);
  v8::Isolate* isolate = script_state->GetIsolate();
  v8::Local<v8::Context> context = script_state->GetContext();

  // Create a plain JavaScript object
  v8::Local<v8::Object> map = v8::Object::New(isolate);

  // Create the default mutator function
  auto default_callback = [](const v8::FunctionCallbackInfo<v8::Value>& info) {
    if (info.Length() < 3) {
      return;
    }

    v8::HandleScope handle_scope(info.GetIsolate());

    // Extract arguments: element, name, value
    if (!info[0]->IsObject()) {
      return;
    }
    v8::Local<v8::Object> element_obj = info[0].As<v8::Object>();

    // Get the native Element from the V8 object
    Element* element = V8Element::ToWrappable(info.GetIsolate(), element_obj);
    if (!element) {
      return;
    }

    v8::String::Utf8Value name_utf8(info.GetIsolate(), info[1]);
    v8::String::Utf8Value value_utf8(info.GetIsolate(), info[2]);

    String name(*name_utf8);
    String value(*value_utf8);

    cobalt::h5vcc::idom::ApplyAttr(element, name, value);
  };

  v8::Local<v8::Function> default_function =
      v8::Function::New(context, default_callback).ToLocalChecked();

  // Set the __default property to the function
  map->Set(context,
           v8::String::NewFromUtf8(isolate, "__default").ToLocalChecked(),
           default_function)
      .ToChecked();

  // Create the style mutator function
  auto style_callback = [](const v8::FunctionCallbackInfo<v8::Value>& info) {
    if (info.Length() < 3) {
      return;
    }

    v8::HandleScope handle_scope(info.GetIsolate());

    // Extract arguments: element, name, value
    if (!info[0]->IsObject()) {
      return;
    }
    v8::Local<v8::Object> element_obj = info[0].As<v8::Object>();

    // Get the native Element from the V8 object
    Element* element = V8Element::ToWrappable(info.GetIsolate(), element_obj);
    if (!element) {
      return;
    }

    v8::String::Utf8Value name_utf8(info.GetIsolate(), info[1]);
    String name(*name_utf8);

    // Create ScriptValue from the V8 value
    ScriptValue value(info.GetIsolate(), info[2]);

    // For style, we should apply it as a property rather than attribute
    cobalt::h5vcc::idom::ApplyProp(element, name, value);
  };

  v8::Local<v8::Function> style_function =
      v8::Function::New(context, style_callback).ToLocalChecked();

  // Set the style property to the function
  map->Set(context, v8::String::NewFromUtf8(isolate, "style").ToLocalChecked(),
           style_function)
      .ToChecked();

  return ScriptValue(isolate, map);
}

V8AttrMutator* IDomAttributeMap::defaultValue() const {
  return default_value_;
}

void IDomAttributeMap::setDefaultValue(V8AttrMutator* value) {
  default_value_ = value;
}

V8AttrMutator* IDomAttributeMap::style() const {
  return style_;
}

void IDomAttributeMap::setStyle(V8AttrMutator* value) {
  style_ = value;
}

void IDomAttributeMap::Trace(Visitor* visitor) const {
  visitor->Trace(default_value_);
  visitor->Trace(style_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink

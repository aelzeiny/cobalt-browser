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

IDomAttributeMap::IDomAttributeMap() {
  // Default mutators will be set when accessed from JavaScript context
  // We can't create V8AttrMutator here without a ScriptState
}

// static
V8AttrMutator* IDomAttributeMap::CreateDefaultMutator(
    ScriptState* script_state) {
  ScriptState::Scope scope(script_state);
  v8::Isolate* isolate = script_state->GetIsolate();
  v8::Local<v8::Context> context = script_state->GetContext();

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
    String name(*name_utf8);

    // Create ScriptValue from the V8 value for type-aware handling
    ScriptValue value(info.GetIsolate(), info[2]);

    cobalt::h5vcc::idom::ApplyAttributeTyped(element, name, value);
  };

  v8::Local<v8::Function> default_function =
      v8::Function::New(context, default_callback).ToLocalChecked();

  return V8AttrMutator::Create(default_function, script_state);
}

// static
V8AttrMutator* IDomAttributeMap::CreateStyleMutator(ScriptState* script_state) {
  ScriptState::Scope scope(script_state);
  v8::Isolate* isolate = script_state->GetIsolate();
  v8::Local<v8::Context> context = script_state->GetContext();

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

    // For style, use the comprehensive style handler
    cobalt::h5vcc::idom::ApplyStyle(element, name, value);
  };

  v8::Local<v8::Function> style_function =
      v8::Function::New(context, style_callback).ToLocalChecked();

  return V8AttrMutator::Create(style_function, script_state);
}

// static
ScriptValue IDomAttributeMap::CreateJavaScriptAttributeMap(
    ScriptState* script_state) {
  ScriptState::Scope scope(script_state);
  v8::Isolate* isolate = script_state->GetIsolate();
  v8::Local<v8::Context> context = script_state->GetContext();

  // Create a plain JavaScript object
  v8::Local<v8::Object> map = v8::Object::New(isolate);

  // Create the default and style mutators
  V8AttrMutator* default_mutator = CreateDefaultMutator(script_state);
  V8AttrMutator* style_mutator = CreateStyleMutator(script_state);

  // Set the __default property to the function
  if (default_mutator && default_mutator->IsCallbackObject()) {
    v8::Local<v8::Value> default_function = default_mutator->CallbackObject();
    map->Set(context,
             v8::String::NewFromUtf8(isolate, "__default").ToLocalChecked(),
             default_function)
        .ToChecked();
  }

  // Set the style property to the function
  if (style_mutator && style_mutator->IsCallbackObject()) {
    v8::Local<v8::Value> style_function = style_mutator->CallbackObject();
    map->Set(context,
             v8::String::NewFromUtf8(isolate, "style").ToLocalChecked(),
             style_function)
        .ToChecked();
  }

  return ScriptValue(isolate, map);
}

V8AttrMutator* IDomAttributeMap::defaultValue(ScriptState* script_state) const {
  if (!default_value_) {
    // Lazy-initialize the default mutator
    const_cast<IDomAttributeMap*>(this)->default_value_ =
        CreateDefaultMutator(script_state);
  }
  return default_value_;
}

void IDomAttributeMap::setDefaultValue(ScriptState* script_state,
                                       V8AttrMutator* value) {
  default_value_ = value;
}

V8AttrMutator* IDomAttributeMap::style(ScriptState* script_state) const {
  if (!style_) {
    // Lazy-initialize the style mutator
    const_cast<IDomAttributeMap*>(this)->style_ =
        CreateStyleMutator(script_state);
  }
  return style_;
}

void IDomAttributeMap::setStyle(ScriptState* script_state,
                                V8AttrMutator* value) {
  style_ = value;
}

V8AttrMutator* IDomAttributeMap::AnonymousNamedGetter(ScriptState* script_state,
                                                      const String& name) {
  // Handle special cases first
  if (name == "__default") {
    return defaultValue(script_state);
  }
  if (name == "style") {
    return style(script_state);
  }

  // Look up in custom mutators
  auto it = custom_mutators_.find(name);
  if (it != custom_mutators_.end()) {
    return it->value;
  }

  return nullptr;
}

void IDomAttributeMap::AnonymousNamedSetter(const String& name,
                                            V8AttrMutator* mutator) {
  // Handle special cases first
  if (name == "__default") {
    default_value_ = mutator;
    return;
  }
  if (name == "style") {
    style_ = mutator;
    return;
  }

  // Store in custom mutators
  if (mutator) {
    custom_mutators_.Set(name, mutator);
  } else {
    custom_mutators_.erase(name);
  }
}

bool IDomAttributeMap::AnonymousNamedDeleter(const String& name) {
  // Cannot delete special cases
  if (name == "__default" || name == "style") {
    return false;
  }

  // Remove from custom mutators
  return custom_mutators_.erase(name) > 0;
}

V8AttrMutator* IDomAttributeMap::GetMutator(ScriptState* script_state,
                                            const String& name) const {
  // First check for specific mutator
  if (name == "style") {
    return style(script_state);
  }

  auto it = custom_mutators_.find(name);
  if (it != custom_mutators_.end()) {
    return it->value;
  }

  // Fall back to default mutator
  return defaultValue(script_state);
}

void IDomAttributeMap::Trace(Visitor* visitor) const {
  visitor->Trace(default_value_);
  visitor->Trace(style_);
  visitor->Trace(custom_mutators_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink

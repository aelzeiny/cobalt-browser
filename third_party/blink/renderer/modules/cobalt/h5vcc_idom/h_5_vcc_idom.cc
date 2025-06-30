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

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/h_5_vcc_idom.h"

#include "base/logging.h"
#include "third_party/blink/renderer/bindings/core/v8/to_v8_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/to_v8_traits.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_element.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_void_callback.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_patch_function.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/attributes.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/core.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/global.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_node_data.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_notification.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_patcher.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_symbols.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/node_data.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

namespace blink {

H5vccIdom::H5vccIdom(LocalDOMWindow& window)
    : ExecutionContextLifecycleObserver(window.GetExecutionContext()),
      notifications_(MakeGarbageCollected<IDomNotification>()),
      symbols_(MakeGarbageCollected<IDomSymbols>()) {
  // attributes_ will be initialized lazily when first accessed
}

void H5vccIdom::ContextDestroyed() {}

void H5vccIdom::patch(Element* element, V8VoidCallback* function) {
  cobalt::h5vcc::idom::PatchInner(node_data_map_, element, function,
                                  ScriptValue());
}

IDomNotification* H5vccIdom::notifications() {
  return notifications_;
}

void H5vccIdom::setKeyAttributeName(const String& name) {
  cobalt::h5vcc::idom::SetKeyAttributeName(name);
}

String H5vccIdom::getKeyAttributeName() {
  return cobalt::h5vcc::idom::KeyAttributeName();
}

IDomSymbols* H5vccIdom::symbols() {
  return symbols_;
}

void H5vccIdom::clearCache(Node* node) {
  cobalt::h5vcc::idom::ClearCache(node_data_map_, node);
}

String H5vccIdom::getKey(Node* node, ExceptionState& exception_state) {
  if (!cobalt::h5vcc::idom::IsDataInitialized(node_data_map_, node)) {
    exception_state.ThrowDOMException(DOMExceptionCode::kInvalidStateError,
                                      "Expected value to be defined");
    return String();
  }
  return cobalt::h5vcc::idom::GetKey(node_data_map_, node);
}

IDomNodeData* H5vccIdom::getData(Node* node, const String& fallback_key) {
  cobalt::h5vcc::idom::NodeData* internal_data =
      cobalt::h5vcc::idom::GetData(node_data_map_, node, fallback_key);
  if (!internal_data) {
    return nullptr;
  }
  return MakeGarbageCollected<IDomNodeData>(internal_data);
}

void H5vccIdom::importNode(Node* node) {
  cobalt::h5vcc::idom::ImportNode(node_data_map_, node);
}

bool H5vccIdom::isDataInitialized(Node* node) {
  return cobalt::h5vcc::idom::IsDataInitialized(node_data_map_, node);
}

void H5vccIdom::applyAttr(Element* el,
                          const String& name,
                          const String& value,
                          ExceptionState& exception_state) {
  if (!el) {
    exception_state.ThrowDOMException(DOMExceptionCode::kInvalidStateError,
                                      "Element cannot be null");
    return;
  }

  // Check for empty attribute names
  if (name.empty()) {
    exception_state.ThrowDOMException(DOMExceptionCode::kInvalidCharacterError,
                                      "Attribute name cannot be empty");
    return;
  }

  // Check for invalid characters in attribute names
  if (!cobalt::h5vcc::idom::IsValidAttributeName(name)) {
    exception_state.ThrowDOMException(DOMExceptionCode::kInvalidCharacterError,
                                      "Invalid character in attribute name");
    return;
  }

  cobalt::h5vcc::idom::ApplyAttr(el, name, value);
}

void H5vccIdom::applyProp(Element* el,
                          const ScriptValue& name,
                          const ScriptValue& value,
                          ExceptionState& exception_state) {
  if (!el) {
    exception_state.ThrowDOMException(DOMExceptionCode::kInvalidStateError,
                                      "Element cannot be null");
    return;
  }

  v8::Local<v8::Value> name_v8 = name.V8Value();

  if (name_v8->IsSymbol()) {
    // Handle symbol property names directly
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

    v8::MaybeLocal<v8::Value> maybe_v8_element =
        blink::ToV8Traits<blink::Element>::ToV8(script_state, el);
    v8::Local<v8::Value> v8_element_value;
    if (!maybe_v8_element.ToLocal(&v8_element_value) ||
        !v8_element_value->IsObject()) {
      return;
    }
    v8::Local<v8::Object> v8_object = v8_element_value.As<v8::Object>();

    // Set property using symbol key directly
    v8::Maybe<bool> result = v8_object->Set(context, name_v8, value.V8Value());
    (void)result;
  } else {
    // Handle string property names
    v8::String::Utf8Value name_utf8(name.GetIsolate(), name_v8);
    String string_name(*name_utf8);
    cobalt::h5vcc::idom::ApplyProp(el, string_name, value);
  }
}

ScriptValue H5vccIdom::attributes(ScriptState* script_state) {
  // Lazy-initialize attributes if not set
  if (attributes_.IsEmpty()) {
    attributes_ = createAttributeMap(script_state);
  }
  return attributes_;
}

void H5vccIdom::setAttributes(ScriptState* script_state,
                              const ScriptValue& attributes) {
  attributes_ = attributes;
}

ScriptValue H5vccIdom::createAttributeMap(ScriptState* script_state) {
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

    v8::Isolate* isolate = info.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    // Extract arguments: element, name, value
    if (!info[0]->IsObject()) {
      return;
    }
    v8::Local<v8::Object> element_obj = info[0].As<v8::Object>();

    // Get the native Element from the V8 object
    Element* element = V8Element::ToWrappable(isolate, element_obj);
    if (!element) {
      return;
    }

    // Handle symbol property names
    if (info[1]->IsSymbol()) {
      // For symbols, we need to use ApplyProp directly with the V8 value
      ScriptValue value(isolate, info[2]);

      blink::LocalFrame* frame = element->GetDocument().GetFrame();
      if (!frame) {
        return;
      }
      v8::Local<v8::Context> context = isolate->GetCurrentContext();
      blink::ScriptState* script_state = blink::ScriptState::From(context);
      if (!script_state) {
        return;
      }

      v8::MaybeLocal<v8::Value> maybe_v8_element =
          blink::ToV8Traits<blink::Element>::ToV8(script_state, element);
      v8::Local<v8::Value> v8_element_value;
      if (!maybe_v8_element.ToLocal(&v8_element_value) ||
          !v8_element_value->IsObject()) {
        return;
      }
      v8::Local<v8::Object> v8_object = v8_element_value.As<v8::Object>();

      // Set property using symbol key directly
      v8::Maybe<bool> result = v8_object->Set(context, info[1], info[2]);
      (void)result;
      return;
    }

    v8::String::Utf8Value name_utf8(isolate, info[1]);
    String name(*name_utf8);

    // Create ScriptValue from the V8 value for type-aware handling
    ScriptValue value(isolate, info[2]);

    cobalt::h5vcc::idom::ApplyAttributeTyped(element, name, value);
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

    // For style, use the comprehensive style handler
    cobalt::h5vcc::idom::ApplyStyle(element, name, value);
  };

  v8::Local<v8::Function> style_function =
      v8::Function::New(context, style_callback).ToLocalChecked();

  // Set the style property to the function
  map->Set(context, v8::String::NewFromUtf8(isolate, "style").ToLocalChecked(),
           style_function)
      .ToChecked();

  return ScriptValue(isolate, map);
}

void H5vccIdom::alignWithDOM(const String& name_or_ctor,
                             const String& key,
                             const String& nonce) {
  cobalt::h5vcc::idom::AlignWithDOM(node_data_map_, name_or_ctor, key, nonce);
}

void H5vccIdom::alwaysDiffAttributes(Element* el) {
  cobalt::h5vcc::idom::AlwaysDiffAttributes(el);
}

Element* H5vccIdom::close() {
  return cobalt::h5vcc::idom::Close();
}

IDomPatcher* H5vccIdom::createPatchInner(const PatchConfig* config) {
  (void)config;  // Suppress unused parameter warning
  return MakeGarbageCollected<IDomPatcher>(false);
}

IDomPatcher* H5vccIdom::createPatchOuter(const PatchConfig* config) {
  (void)config;  // Suppress unused parameter warning
  return MakeGarbageCollected<IDomPatcher>(true);
}

IDomContext* H5vccIdom::currentContext() {
  cobalt::h5vcc::idom::Context* ctx = cobalt::h5vcc::idom::CurrentContext();
  if (!ctx) {
    return nullptr;
  }
  // Create IDomContext wrapper for the cobalt context
  return MakeGarbageCollected<IDomContext>(ctx);
}

Element* H5vccIdom::currentElement() {
  return cobalt::h5vcc::idom::CurrentElement();
}

Node* H5vccIdom::currentPointer() {
  return cobalt::h5vcc::idom::CurrentPointer();
}

Node* H5vccIdom::getNextNode() {
  return cobalt::h5vcc::idom::GetNextNode();
}

Element* H5vccIdom::open(const String& name_or_ctor,
                         const String& key,
                         const String& nonce) {
  Element* result =
      cobalt::h5vcc::idom::Open(node_data_map_, name_or_ctor, key, nonce);
  if (!result && !name_or_ctor.empty()) {
    // If Open() returned null but name was not empty, it means invalid name
    v8::Isolate* isolate = GetExecutionContext()->GetIsolate();
    isolate->ThrowException(v8::Exception::Error(
        v8::String::NewFromUtf8Literal(isolate, "Invalid element name")));
    return nullptr;
  }
  return result;
}

Node* H5vccIdom::patchInner(Element* el,
                            V8PatchFunction* template_function,
                            ScriptValue data) {
  if (!el) {
    // Throw exception for null element
    v8::Isolate* isolate = GetExecutionContext()->GetIsolate();
    isolate->ThrowException(v8::Exception::TypeError(
        v8::String::NewFromUtf8Literal(isolate, "Element cannot be null")));
    return nullptr;
  }
  return cobalt::h5vcc::idom::PatchInner(node_data_map_, el, template_function,
                                         data);
}

Node* H5vccIdom::patchOuter(Element* el,
                            V8PatchFunction* template_function,
                            ScriptValue data) {
  if (!el) {
    // Throw exception for null element
    v8::Isolate* isolate = GetExecutionContext()->GetIsolate();
    isolate->ThrowException(v8::Exception::TypeError(
        v8::String::NewFromUtf8Literal(isolate, "Element cannot be null")));
    return nullptr;
  }
  return cobalt::h5vcc::idom::PatchOuter(node_data_map_, el, template_function,
                                         data);
}

Text* H5vccIdom::text() {
  cobalt::h5vcc::idom::SetCurrentDataMap(&node_data_map_);
  Text* result = cobalt::h5vcc::idom::Text();
  cobalt::h5vcc::idom::SetCurrentDataMap(nullptr);
  return result;
}

void H5vccIdom::skip() {
  cobalt::h5vcc::idom::Skip();
}

Node* H5vccIdom::skipNode() {
  return cobalt::h5vcc::idom::SkipNode();
}

Element* H5vccIdom::tryGetCurrentElement() {
  return cobalt::h5vcc::idom::TryGetCurrentElement();
}

void H5vccIdom::Trace(Visitor* visitor) const {
  visitor->Trace(notifications_);
  visitor->Trace(symbols_);
  visitor->Trace(node_data_map_);
  // attributes_ is a ScriptValue which handles its own V8 references
  ExecutionContextLifecycleObserver::Trace(visitor);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink

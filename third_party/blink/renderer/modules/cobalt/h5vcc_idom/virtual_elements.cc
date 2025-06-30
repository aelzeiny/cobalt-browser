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

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/virtual_elements.h"

#include "third_party/blink/renderer/bindings/core/v8/to_v8_traits.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_void_callback.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/attributes.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_node_data.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

namespace blink {
namespace virtual_elements {

void Attr(HeapVector<ScriptValue>& attrs_builder,
          const String& name,
          const ScriptValue& value) {
  v8::Isolate* isolate = value.GetIsolate();
  attrs_builder.push_back(ScriptValue(isolate, V8String(isolate, name)));
  attrs_builder.push_back(value);
}

void Key(HeapVector<ScriptValue>& args_builder, const String& key) {
  if (args_builder.size() >= 2) {
    // We need an isolate, but we don't have a ScriptValue here
    // This function should be called from a context where we have access to an
    // isolate For now, we'll modify the existing ScriptValue if it exists
    if (!args_builder[1].IsEmpty()) {
      v8::Isolate* isolate = args_builder[1].GetIsolate();
      args_builder[1] = ScriptValue(isolate, V8String(isolate, key));
    }
  }
}

void ApplyAttrs(HeapVector<ScriptValue>& attrs_builder,
                Element* current_element,
                const ScriptValue& attrs) {
  if (!current_element) {
    return;
  }

  // If custom attrs are provided, use them to apply the buffered attributes
  if (!attrs.IsEmpty()) {
    v8::Isolate* isolate = attrs.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> attrs_v8 = attrs.V8Value();

    if (attrs_v8->IsObject()) {
      v8::Local<v8::Object> attrs_obj = attrs_v8.As<v8::Object>();

      // Apply buffered attributes using custom attribute map
      for (wtf_size_t i = 0; i < attrs_builder.size(); i += 2) {
        if (i + 1 < attrs_builder.size()) {
          v8::Local<v8::Value> name_v8 = attrs_builder[i].V8Value();
          v8::String::Utf8Value name_utf8(attrs_builder[i].GetIsolate(),
                                          name_v8);
          String name(*name_utf8);

          // Look for a specific handler for this attribute name
          v8::Local<v8::String> key_string = V8String(isolate, name);
          v8::Local<v8::Value> handler_value;
          bool has_handler = false;

          if (attrs_obj->Get(context, key_string).ToLocal(&handler_value) &&
              handler_value->IsFunction()) {
            has_handler = true;
          } else {
            // Fall back to __default handler
            v8::Local<v8::String> default_key = V8String(isolate, "__default");
            if (attrs_obj->Get(context, default_key).ToLocal(&handler_value) &&
                handler_value->IsFunction()) {
              has_handler = true;
            }
          }

          if (has_handler) {
            v8::Local<v8::Function> handler = handler_value.As<v8::Function>();

            // Convert current_element to V8 object
            ScriptState* script_state = ScriptState::From(context);
            v8::MaybeLocal<v8::Value> maybe_element =
                ToV8Traits<Element>::ToV8(script_state, current_element);
            v8::Local<v8::Value> element_v8;
            if (maybe_element.ToLocal(&element_v8)) {
              // Call the handler: handler(element, name, value)
              v8::Local<v8::Value> args[] = {element_v8, name_v8,
                                             attrs_builder[i + 1].V8Value()};
              handler->Call(context, v8::Undefined(isolate), 3, args)
                  .ToLocalChecked();
            }
          } else {
            // Fall back to default behavior if no handler found
            cobalt::h5vcc::idom::ApplyAttributeTyped(current_element, name,
                                                     attrs_builder[i + 1]);
          }
        }
      }
    } else {
      // attrs is not an object, fall back to default behavior
      for (wtf_size_t i = 0; i < attrs_builder.size(); i += 2) {
        if (i + 1 < attrs_builder.size()) {
          v8::Local<v8::Value> name_v8 = attrs_builder[i].V8Value();
          v8::String::Utf8Value name_utf8(attrs_builder[i].GetIsolate(),
                                          name_v8);
          String name(*name_utf8);
          cobalt::h5vcc::idom::ApplyAttributeTyped(current_element, name,
                                                   attrs_builder[i + 1]);
        }
      }
    }
  } else {
    // No custom attrs provided, use default behavior
    for (wtf_size_t i = 0; i < attrs_builder.size(); i += 2) {
      if (i + 1 < attrs_builder.size()) {
        v8::Local<v8::Value> name_v8 = attrs_builder[i].V8Value();
        v8::String::Utf8Value name_utf8(attrs_builder[i].GetIsolate(), name_v8);
        String name(*name_utf8);
        cobalt::h5vcc::idom::ApplyAttributeTyped(current_element, name,
                                                 attrs_builder[i + 1]);
      }
    }
  }

  // Clear the builder
  attrs_builder.clear();
}

void ApplyStatics(const HeapVector<ScriptValue>& statics,
                  Element* current_element,
                  const ScriptValue& attrs) {
  if (!current_element || statics.empty()) {
    return;
  }

  // If custom attrs are provided, use them to apply the static attributes
  if (!attrs.IsEmpty()) {
    v8::Isolate* isolate = attrs.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Value> attrs_v8 = attrs.V8Value();

    if (attrs_v8->IsObject()) {
      v8::Local<v8::Object> attrs_obj = attrs_v8.As<v8::Object>();

      // Apply static attributes using custom attribute map
      for (wtf_size_t i = 0; i < statics.size(); i += 2) {
        if (i + 1 < statics.size()) {
          v8::Local<v8::Value> name_v8 = statics[i].V8Value();
          v8::String::Utf8Value name_utf8(statics[i].GetIsolate(), name_v8);
          String name(*name_utf8);

          // Look for a specific handler for this attribute name
          v8::Local<v8::String> key_string = V8String(isolate, name);
          v8::Local<v8::Value> handler_value;
          bool has_handler = false;

          if (attrs_obj->Get(context, key_string).ToLocal(&handler_value) &&
              handler_value->IsFunction()) {
            has_handler = true;
          } else {
            // Fall back to __default handler
            v8::Local<v8::String> default_key = V8String(isolate, "__default");
            if (attrs_obj->Get(context, default_key).ToLocal(&handler_value) &&
                handler_value->IsFunction()) {
              has_handler = true;
            }
          }

          if (has_handler) {
            v8::Local<v8::Function> handler = handler_value.As<v8::Function>();

            // Convert current_element to V8 object
            ScriptState* script_state = ScriptState::From(context);
            v8::MaybeLocal<v8::Value> maybe_element =
                ToV8Traits<Element>::ToV8(script_state, current_element);
            v8::Local<v8::Value> element_v8;
            if (maybe_element.ToLocal(&element_v8)) {
              // Call the handler: handler(element, name, value)
              v8::Local<v8::Value> args[] = {element_v8, name_v8,
                                             statics[i + 1].V8Value()};
              handler->Call(context, v8::Undefined(isolate), 3, args)
                  .ToLocalChecked();
            }
          } else {
            // Fall back to default behavior if no handler found
            cobalt::h5vcc::idom::ApplyAttributeTyped(current_element, name,
                                                     statics[i + 1]);
          }
        }
      }
    } else {
      // attrs is not an object, fall back to default behavior
      for (wtf_size_t i = 0; i < statics.size(); i += 2) {
        if (i + 1 < statics.size()) {
          v8::Local<v8::Value> name_v8 = statics[i].V8Value();
          v8::String::Utf8Value name_utf8(statics[i].GetIsolate(), name_v8);
          String name(*name_utf8);
          cobalt::h5vcc::idom::ApplyAttributeTyped(current_element, name,
                                                   statics[i + 1]);
        }
      }
    }
  } else {
    // No custom attrs provided, use default behavior
    for (wtf_size_t i = 0; i < statics.size(); i += 2) {
      if (i + 1 < statics.size()) {
        v8::Local<v8::Value> name_v8 = statics[i].V8Value();
        v8::String::Utf8Value name_utf8(statics[i].GetIsolate(), name_v8);
        String name(*name_utf8);
        cobalt::h5vcc::idom::ApplyAttributeTyped(current_element, name,
                                                 statics[i + 1]);
      }
    }
  }
}

void ElementOpenStart(HeapVector<ScriptValue>& args_builder,
                      const String& name_or_ctor,
                      const String& key,
                      const absl::optional<HeapVector<ScriptValue>>& statics) {
  // This function is just a placeholder - the actual isolate handling
  // should be done in the caller (H5vccIdom::elementOpenStart)
  // since we need access to the execution context to get an isolate

  // Clear and initialize args builder
  args_builder.clear();
  args_builder.resize(3);

  // The actual V8 work will be done by the caller
}

Element* ElementOpenEnd(
    HeapVector<ScriptValue>& args_builder,
    HeapVector<ScriptValue>& attrs_builder,
    std::function<Element*(const String&, const String&)> open_func) {
  if (args_builder.empty() || !open_func) {
    return nullptr;
  }

  // Check if first argument exists and is valid
  if (args_builder[0].IsEmpty()) {
    return nullptr;
  }

  v8::Local<v8::Value> name_v8 = args_builder[0].V8Value();
  if (name_v8.IsEmpty()) {
    return nullptr;
  }

  v8::String::Utf8Value name_utf8(args_builder[0].GetIsolate(), name_v8);
  String name_or_ctor(*name_utf8);

  String key;
  if (args_builder.size() > 1 && !args_builder[1].IsEmpty()) {
    v8::Local<v8::Value> key_v8 = args_builder[1].V8Value();
    if (!key_v8.IsEmpty()) {
      v8::String::Utf8Value key_utf8(args_builder[1].GetIsolate(), key_v8);
      key = String(*key_utf8);
    }
  }

  Element* element = open_func(name_or_ctor, key);

  // Apply statics if present
  if (args_builder.size() > 2 && !args_builder[2].IsEmpty()) {
    v8::Local<v8::Value> statics_value = args_builder[2].V8Value();
    if (statics_value->IsArray()) {
      v8::Local<v8::Array> statics_array = statics_value.As<v8::Array>();
      HeapVector<ScriptValue> statics;
      v8::Local<v8::Context> context =
          args_builder[2].GetIsolate()->GetCurrentContext();
      for (uint32_t i = 0; i < statics_array->Length(); ++i) {
        v8::Local<v8::Value> item;
        if (statics_array->Get(context, i).ToLocal(&item)) {
          statics.push_back(ScriptValue(args_builder[2].GetIsolate(), item));
        }
      }
      ApplyStatics(statics, element);
    }
  }

  // Apply buffered attributes
  ApplyAttrs(attrs_builder, element);

  // Clear args builder
  args_builder.clear();

  return element;
}

Element* ElementOpen(
    HeapVector<ScriptValue>& args_builder,
    HeapVector<ScriptValue>& attrs_builder,
    const String& name_or_ctor,
    const String& key,
    const absl::optional<HeapVector<ScriptValue>>& statics,
    const HeapVector<ScriptValue>& var_args,
    std::function<Element*(const String&, const String&)> open_func) {
  if (!open_func) {
    return nullptr;
  }

  // Don't call ElementOpenStart here since it needs isolate access
  // The caller should have already set up the args_builder properly

  // Add varArgs as attributes (in name, value pairs)
  for (wtf_size_t i = 0; i < var_args.size(); i += 2) {
    if (i + 1 < var_args.size()) {
      v8::Local<v8::Value> name_v8 = var_args[i].V8Value();
      v8::String::Utf8Value name_utf8(var_args[i].GetIsolate(), name_v8);
      String attr_name(*name_utf8);
      Attr(attrs_builder, attr_name, var_args[i + 1]);
    }
  }

  return ElementOpenEnd(args_builder, attrs_builder, open_func);
}

Element* ElementClose(const String& name_or_ctor,
                      std::function<Element*()> close_func) {
  if (!close_func) {
    return nullptr;
  }

  Element* element = close_func();

  // In DEBUG mode, we could validate that the closing tag matches the opening
  // tag Similar to assertCloseMatchesOpenTag in the TypeScript version For now,
  // we'll just return the closed element

  return element;
}

Element* ElementVoid(
    HeapVector<ScriptValue>& args_builder,
    HeapVector<ScriptValue>& attrs_builder,
    const String& name_or_ctor,
    const String& key,
    const absl::optional<HeapVector<ScriptValue>>& statics,
    const HeapVector<ScriptValue>& var_args,
    std::function<Element*(const String&, const String&)> open_func,
    std::function<Element*()> close_func) {
  Element* element = ElementOpen(args_builder, attrs_builder, name_or_ctor, key,
                                 statics, var_args, open_func);
  if (element && close_func) {
    close_func();  // Close the element but don't use the return value
  }
  return element;  // Return the opened element
}

Text* TextWithValue(const ScriptValue& value,
                    const HeapVector<Member<V8VoidCallback>>& formatters,
                    std::function<Text*()> text_func,
                    std::function<IDomNodeData*(Node*)> get_data_func) {
  if (!text_func || !get_data_func) {
    return nullptr;
  }

  Text* node = text_func();
  if (!node) {
    return nullptr;
  }

  IDomNodeData* data = get_data_func(node);
  if (!data) {
    return node;
  }

  String current_text = data->text();
  String new_value;

  // Convert ScriptValue to string
  v8::Local<v8::Value> v8_value = value.V8Value();
  if (v8_value->IsString()) {
    v8::String::Utf8Value utf8_value(value.GetIsolate(), v8_value);
    new_value = String(*utf8_value);
  } else if (v8_value->IsNumber()) {
    v8::Local<v8::Context> context = value.GetIsolate()->GetCurrentContext();
    new_value = String::Number(v8_value->NumberValue(context).FromMaybe(0));
  } else if (v8_value->IsBoolean()) {
    new_value = v8_value->BooleanValue(value.GetIsolate()) ? "true" : "false";
  } else {
    new_value = "";  // Default to empty string for other types
  }

  // Only update if the text has changed
  if (current_text != new_value) {
    String formatted = new_value;

    // Apply formatters in sequence
    for (const auto& formatter : formatters) {
      if (formatter) {
        // For formatters, we'd need to call the callback with the current value
        // This is complex in the V8 binding context, so for now we'll skip
        // formatters In a full implementation, we'd need to:
        // 1. Convert the formatted string to a ScriptValue
        // 2. Call formatter->InvokeAndReportException with that value
        // 3. Get the result and convert back to string
      }
    }

    // Update the text content if it differs from what's currently in the DOM
    if (node->data() != formatted) {
      node->setData(formatted);
    }
  }

  return node;
}

}  // namespace virtual_elements
}  // namespace blink

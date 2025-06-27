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

#include "third_party/blink/renderer/bindings/core/v8/to_v8_traits.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/core/css/css_style_declaration.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/qualified_name.h"
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
               const WTF::String& value) {
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

  v8::Local<v8::Value> v8_value;
  if (value.IsNull()) {
    v8_value = v8::Null(isolate);
  } else {
    v8_value =
        v8::String::NewFromUtf8(isolate, value.Utf8().c_str()).ToLocalChecked();
  }
  v8::Local<v8::String> v8_name =
      v8::String::NewFromUtf8(isolate, name.Utf8().c_str()).ToLocalChecked();

  v8::Maybe<bool> result = v8_object->Set(context, v8_name, v8_value);
  (void)result;
}

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

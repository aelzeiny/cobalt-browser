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

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/core.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_void_callback.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_patch_function.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/assertions.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/context.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/dom_util.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/node_data.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/nodes.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

namespace cobalt {
namespace h5vcc {
namespace idom {

namespace {

Context* context = nullptr;
blink::Node* current_node = nullptr;
blink::Node* current_parent = nullptr;
blink::Document* doc = nullptr;
blink::HeapVector<blink::Member<blink::Node>>* focus_path = nullptr;

void EnterNode() {
  current_parent = current_node;
  current_node = nullptr;
}

void ExitNode() {
  // clearUnvisitedDOM(current_parent, getNextNode(), null);
  current_node = current_parent;
  current_parent = current_parent->parentNode();
}

void updatePatchContext(Context* ctx) {
  // Update the global context reference
  context = ctx;
}

}  // namespace

void AlignWithDOM(NodeDataMap& data_map,
                  const WTF::String& name_or_ctor,
                  const WTF::String& key,
                  const WTF::String& nonce) {
  // This is a simplified implementation.
  current_node = GetNextNode();
}

void AlwaysDiffAttributes(blink::Element* el) {
  // This is a simplified implementation.
}

blink::Element* Close() {
  ExitNode();
  return blink::To<blink::Element>(current_node);
}

Context* CurrentContext() {
  return context;
}

blink::Element* CurrentElement() {
  return blink::To<blink::Element>(current_parent);
}

blink::Node* CurrentPointer() {
  return GetNextNode();
}

blink::Node* GetNextNode() {
  if (current_node) {
    return current_node->nextSibling();
  } else {
    return current_parent->firstChild();
  }
}

blink::Element* Open(NodeDataMap& data_map,
                     const WTF::String& name_or_ctor,
                     const WTF::String& key,
                     const WTF::String& nonce) {
  AlignWithDOM(data_map, name_or_ctor, key, nonce);
  EnterNode();
  return blink::To<blink::Element>(current_parent);
}

void Skip() {
  AssertNoChildrenDeclaredYet("skip", current_node);
  SetInSkip(true);
  current_node = current_parent->lastChild();
}

blink::Node* SkipNode() {
  current_node = GetNextNode();
  return current_node;
}

blink::Element* TryGetCurrentElement() {
  return blink::To<blink::Element>(current_parent);
}

void Patch(blink::Element* node,
           blink::V8PatchFunction* template_function,
           blink::ScriptValue data,
           bool is_outer) {
  Context* prev_context = context;
  blink::Document* prev_doc = doc;
  blink::HeapVector<blink::Member<blink::Node>>* prev_focus_path = focus_path;
  blink::Node* prev_current_node = current_node;
  blink::Node* prev_current_parent = current_parent;

  doc = &node->GetDocument();
  context = MakeGarbageCollected<Context>(node);
  current_node = nullptr;
  current_parent = node->parentNode();
  // Store the focused path - allocate a new HeapVector for this patch
  auto current_focus_path = GetFocusedPath(node, current_parent);
  focus_path = &current_focus_path;

  bool previous_in_attributes = SetInAttributes(false);
  bool previous_in_skip = SetInSkip(false);
  updatePatchContext(context);

  // Get V8 isolate from ExecutionContext
  v8::Isolate* isolate = node->GetExecutionContext()->GetIsolate();
  v8::TryCatch try_catch(isolate);
  v8::Local<v8::Value> v8_data = data.V8Value();
  if (is_outer) {
    // TODO: Implement patchOuter
  } else {
    current_node = node;
    EnterNode();
    blink::ScriptValue script_data(isolate, v8_data);
    v8::Maybe<void> result = template_function->Invoke(nullptr, script_data);
    (void)result;  // Suppress unused variable warning
    ExitNode();
  }

  context->NotifyChanges(nullptr);

  doc = prev_doc;
  context = prev_context;
  focus_path = prev_focus_path;
  current_node = prev_current_node;
  current_parent = prev_current_parent;

  SetInAttributes(previous_in_attributes);
  SetInSkip(previous_in_skip);
  updatePatchContext(context);
}

void PatchInner(blink::Element* node,
                blink::V8PatchFunction* template_function,
                blink::ScriptValue data) {
  Patch(node, template_function, data, false);
}

void PatchOuter(blink::Element* node,
                blink::V8PatchFunction* template_function,
                blink::ScriptValue data) {
  Patch(node, template_function, data, true);
}

void PatchInner(blink::Element* node,
                blink::V8VoidCallback* template_function,
                blink::ScriptValue data) {
  // Convert V8VoidCallback to a simple patch operation
  // For now, just call the callback directly as this matches the original
  // intent
  if (node && template_function) {
    template_function->InvokeAndReportException(
        node->GetExecutionContext()->ToScriptWrappable());
  }
}

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

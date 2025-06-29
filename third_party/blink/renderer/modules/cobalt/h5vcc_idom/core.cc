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

#include "base/no_destructor.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_void_callback.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_patch_function.h"
#include "third_party/blink/renderer/core/dom/text.h"
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
blink::TreeWalker* comment_tree_walker = nullptr;

// Used to build up call arguments. Each patch call gets a separate copy.
// Using base::NoDestructor to avoid global constructor warnings
std::vector<blink::ScriptValue>& GetArgsBuilderInternal() {
  static base::NoDestructor<std::vector<blink::ScriptValue>> args_builder;
  return *args_builder;
}

// Used to build up attrs for an element.
std::vector<blink::ScriptValue>& GetAttrsBuilderInternal() {
  static base::NoDestructor<std::vector<blink::ScriptValue>> attrs_builder;
  return *attrs_builder;
}

// Need a reference to the node data map (will be passed from H5vccIdom)
NodeDataMap* current_data_map = nullptr;

// Default match function to determine if nodes match
bool DefaultMatchFn(blink::Node* match_node,
                    const WTF::String& name_or_ctor,
                    const WTF::String& expected_name_or_ctor,
                    const WTF::String& key,
                    const WTF::String& expected_key) {
  // Key check treats null and undefined as equivalent
  return name_or_ctor == expected_name_or_ctor && key == expected_key;
}

// Checks whether the current node matches the specified nameOrCtor and key
bool Matches(NodeDataMap& data_map,
             blink::Node* match_node,
             const WTF::String& name_or_ctor,
             const WTF::String& key) {
  NodeData* data = GetData(data_map, match_node, WTF::String());
  if (!data) {
    return false;
  }
  return DefaultMatchFn(match_node, name_or_ctor, data->GetNameOrCtor(), key,
                        data->GetKey());
}

// Finds the matching node, starting at `node` and looking at subsequent
// siblings
blink::Node* GetMatchingNode(NodeDataMap& data_map,
                             blink::Node* match_node,
                             const WTF::String& name_or_ctor,
                             const WTF::String& key) {
  if (!match_node) {
    return nullptr;
  }

  blink::Node* cur = match_node;
  do {
    if (Matches(data_map, cur, name_or_ctor, key)) {
      return cur;
    }
  } while (!key.IsNull() && (cur = cur->nextSibling()));

  return nullptr;
}

// Creates a Node and marks it as created
blink::Node* CreateNode(NodeDataMap& data_map,
                        const WTF::String& name_or_ctor,
                        const WTF::String& key,
                        const WTF::String& nonce) {
  blink::Node* node = nullptr;

  if (name_or_ctor == "#text") {
    node = CreateText(data_map, doc);
  } else {
    node = CreateElement(data_map, doc, current_parent,
                         WTF::AtomicString(name_or_ctor), key);
    if (!nonce.IsNull() && node->IsElementNode()) {
      blink::To<blink::Element>(node)->setAttribute(WTF::AtomicString("nonce"),
                                                    WTF::AtomicString(nonce));
    }
  }

  if (context) {
    context->MarkCreated(node);
  }

  return node;
}

// Changes to the next sibling of the current node
void NextNode() {
  current_node = GetNextNode();
}

void EnterNode() {
  current_parent = current_node;
  current_node = nullptr;
}

// Clears out any unvisited nodes in a given range
void ClearUnvisitedDOM(blink::Node* maybe_parent_node,
                       blink::Node* start_node,
                       blink::Node* end_node) {
  if (!maybe_parent_node) {
    return;
  }

  blink::Node* parent_node = maybe_parent_node;
  blink::Node* child = start_node;

  while (child != end_node) {
    blink::Node* next = child ? child->nextSibling() : nullptr;
    if (child) {
      parent_node->removeChild(child, ASSERT_NO_EXCEPTION);
      if (context) {
        context->MarkDeleted(child);
      }
    }
    child = next;
  }
}

void ExitNode() {
  ClearUnvisitedDOM(current_parent, GetNextNode(), nullptr);
  current_node = current_parent;
  current_parent = current_parent->parentNode();
}

void updatePatchContext(Context* ctx) {
  // Update the global context reference
  context = ctx;
}

// Get the args builder (currently unused)
// std::vector<blink::ScriptValue>& GetArgsBuilder() {
//   return GetArgsBuilderInternal();
// }

// Get the attrs builder (currently unused)
// std::vector<blink::ScriptValue>& GetAttrsBuilder() {
//   return GetAttrsBuilderInternal();
// }

}  // namespace

void AlignWithDOM(NodeDataMap& data_map,
                  const WTF::String& name_or_ctor,
                  const WTF::String& key,
                  const WTF::String& nonce) {
  NextNode();
  blink::Node* existing_node =
      GetMatchingNode(data_map, current_node, name_or_ctor, key);
  blink::Node* node = existing_node
                          ? existing_node
                          : CreateNode(data_map, name_or_ctor, key, nonce);

  // If we are at the matching node, then we are done.
  if (node == current_node) {
    return;
  }

  // Re-order the node into the right position, preserving focus if either
  // node or current_node are focused by making sure that they are not detached
  // from the DOM.
  bool node_in_focus_path = false;
  if (focus_path) {
    for (const auto& focus_node : *focus_path) {
      if (focus_node == node) {
        node_in_focus_path = true;
        break;
      }
    }
  }

  if (node_in_focus_path) {
    // Move everything else before the node.
    MoveBefore(current_parent, node, current_node);
  } else {
    current_parent->insertBefore(node, current_node, ASSERT_NO_EXCEPTION);
  }

  current_node = node;
}

void AlwaysDiffAttributes(blink::Element* el) {
  if (!el || !current_data_map) {
    return;
  }
  NodeData* data = GetData(*current_data_map, el, WTF::String());
  if (data) {
    data->SetAlwaysDiffAttributes(true);
  }
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
  return current_parent ? blink::To<blink::Element>(current_parent) : nullptr;
}

// Makes sure the current node is a Text node and creates one if it's not
blink::Text* Text() {
  if (!current_data_map) {
    return nullptr;
  }
  AlignWithDOM(*current_data_map, "#text", WTF::String(), WTF::String());
  return blink::To<blink::Text>(current_node);
}

// Sets the current data map for this patch session
void SetCurrentDataMap(NodeDataMap* data_map) {
  current_data_map = data_map;
}

// Shared patch setup and cleanup
blink::Node* RunPatchWithContext(blink::Node* node,
                                 blink::V8PatchFunction* fn,
                                 blink::ScriptValue data,
                                 bool is_outer) {
  Context* prev_context = context;
  blink::Document* prev_doc = doc;
  blink::HeapVector<blink::Member<blink::Node>>* prev_focus_path = focus_path;
  std::vector<blink::ScriptValue> prev_args_builder = GetArgsBuilderInternal();
  std::vector<blink::ScriptValue> prev_attrs_builder =
      GetAttrsBuilderInternal();
  blink::Node* prev_current_node = current_node;
  blink::Node* prev_current_parent = current_parent;
  blink::TreeWalker* prev_tree_walker = comment_tree_walker;
  bool previous_in_attributes = false;
  bool previous_in_skip = false;

  doc = &node->GetDocument();
  context = MakeGarbageCollected<Context>(node);
  GetArgsBuilderInternal().clear();
  GetAttrsBuilderInternal().clear();
  current_node = nullptr;
  current_parent = node->parentNode();
  auto current_focus_path = GetFocusedPath(node, current_parent);
  focus_path = &current_focus_path;

  previous_in_attributes = SetInAttributes(false);
  previous_in_skip = SetInSkip(false);
  updatePatchContext(context);

  blink::Node* ret_val = nullptr;
  if (is_outer) {
    // PatchOuter implementation inline
    // Create a fake start node that points to our target node
    blink::Element* start_node = blink::To<blink::Element>(node);
    blink::Node* expected_next_node = node->nextSibling();
    blink::Node* expected_prev_node = node->previousSibling();

    // Set up the fake start node by temporarily manipulating current_node
    current_node = expected_prev_node;

    // Get V8 isolate from ExecutionContext
    v8::Isolate* isolate = node->GetExecutionContext()->GetIsolate();
    // Handle case where data might be undefined/empty
    blink::ScriptValue script_data =
        data.IsEmpty() ? blink::ScriptValue(isolate, v8::Undefined(isolate))
                       : data;
    fn->InvokeAndReportException(
        node->GetExecutionContext()->ToScriptWrappable(), script_data);

    // Verify that we have the right node structure
    if (current_data_map) {
      NodeData* data_obj = GetData(*current_data_map, node, WTF::String());
      if (data_obj && !data_obj->GetKey().IsNull()) {
        AssertPatchOuterHasParentNode(current_parent);
      }
    }
    AssertPatchElementNoExtras(start_node, current_node, expected_next_node,
                               expected_prev_node);

    if (current_parent) {
      ClearUnvisitedDOM(current_parent, GetNextNode(), node->nextSibling());
    }

    // Return null if no change, otherwise return the current node
    ret_val = (current_node == expected_prev_node) ? nullptr : current_node;
  } else {
    // PatchInner implementation inline
    current_node = node;
    EnterNode();

    // Get V8 isolate from ExecutionContext
    v8::Isolate* isolate = node->GetExecutionContext()->GetIsolate();
    // Handle case where data might be undefined/empty
    blink::ScriptValue script_data =
        data.IsEmpty() ? blink::ScriptValue(isolate, v8::Undefined(isolate))
                       : data;
    fn->InvokeAndReportException(
        node->GetExecutionContext()->ToScriptWrappable(), script_data);

    ExitNode();
    AssertNoUnclosedTags(current_node, node);
    ret_val = node;
  }
  AssertVirtualAttributesClosed();

  context->NotifyChanges(nullptr);

  doc = prev_doc;
  context = prev_context;
  GetArgsBuilderInternal() = prev_args_builder;
  GetAttrsBuilderInternal() = prev_attrs_builder;
  current_node = prev_current_node;
  current_parent = prev_current_parent;
  focus_path = prev_focus_path;
  comment_tree_walker = prev_tree_walker;

  SetInAttributes(previous_in_attributes);
  SetInSkip(previous_in_skip);
  updatePatchContext(context);

  return ret_val;
}

blink::Node* PatchInner(NodeDataMap& data_map,
                        blink::Element* node,
                        blink::V8PatchFunction* template_function,
                        blink::ScriptValue data) {
  SetCurrentDataMap(&data_map);
  blink::Node* result =
      RunPatchWithContext(node, template_function, data, false);
  SetCurrentDataMap(nullptr);
  return result;
}

blink::Node* PatchOuter(NodeDataMap& data_map,
                        blink::Element* node,
                        blink::V8PatchFunction* template_function,
                        blink::ScriptValue data) {
  SetCurrentDataMap(&data_map);
  blink::Node* result =
      RunPatchWithContext(node, template_function, data, true);
  SetCurrentDataMap(nullptr);
  return result;
}

blink::Node* PatchInner(NodeDataMap& data_map,
                        blink::Element* node,
                        blink::V8VoidCallback* template_function,
                        blink::ScriptValue data) {
  if (!node || !template_function) {
    return nullptr;
  }

  // Simple implementation: just call the callback
  // This should be enough to test if the basic callback mechanism works
  template_function->InvokeAndReportException(
      node->GetExecutionContext()->ToScriptWrappable());

  return node;
}

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

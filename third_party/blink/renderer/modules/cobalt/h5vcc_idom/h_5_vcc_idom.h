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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_H_5_VCC_IDOM_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_H_5_VCC_IDOM_H_

#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/core/execution_context/execution_context_lifecycle_observer.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/attribute_value.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/attributes.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/core.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_attribute_map.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_context.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_node_data.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_notification.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_patcher.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_symbols.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/native_patch.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/native_virtual_elements.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/node_data.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/virtual_elements.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace blink {

class ExecutionContext;
class LocalDOMWindow;
class Element;
class V8VoidCallback;
class V8PatchFunction;
class PatchConfig;

class H5vccIdom final : public ScriptWrappable,
                        public ExecutionContextLifecycleObserver {
  DEFINE_WRAPPERTYPEINFO();

 public:
  explicit H5vccIdom(LocalDOMWindow&);

  void ContextDestroyed() override;

  // Web-exposed interface:
  void patch(Element* element, V8VoidCallback* function);
  IDomNotification* notifications();
  void setKeyAttributeName(const String& name);
  String getKeyAttributeName();
  IDomSymbols* symbols();
  void clearCache(Node* node);
  String getKey(Node* node, ExceptionState&);
  IDomNodeData* getData(Node* node, const String& fallback_key = String());
  void importNode(Node* node);
  bool isDataInitialized(Node* node);
  void applyAttr(Element* el,
                 const String& name,
                 const String& value,
                 ExceptionState&);
  void applyProp(Element* el,
                 const ScriptValue& name,
                 const ScriptValue& value,
                 ExceptionState&);
  ScriptValue attributes(ScriptState* script_state);
  void setAttributes(ScriptState* script_state, const ScriptValue& attributes);
  ScriptValue createAttributeMap(ScriptState*);
  void alignWithDOM(const String& name_or_ctor,
                    const String& key,
                    const String& nonce = String());
  void alwaysDiffAttributes(Element* el);
  Element* close();
  IDomPatcher* createPatchInner(const PatchConfig* config = nullptr);
  IDomPatcher* createPatchOuter(const PatchConfig* config = nullptr);
  IDomContext* currentContext();
  Element* currentElement();
  Node* currentPointer();
  Node* getNextNode();
  Element* open(const String& name_or_ctor,
                const String& key = String(),
                const String& nonce = String());
  Node* patchInner(Element* el,
                   V8PatchFunction* template_function,
                   ScriptValue data = ScriptValue());
  Node* patchOuter(Element* el,
                   V8PatchFunction* template_function,
                   ScriptValue data = ScriptValue());
  Text* text();
  Text* text(const ScriptValue& value,
             const HeapVector<Member<V8VoidCallback>>& formatters =
                 HeapVector<Member<V8VoidCallback>>());
  Text* textWithValue(const ScriptValue& value,
                      const HeapVector<Member<V8VoidCallback>>& formatters =
                          HeapVector<Member<V8VoidCallback>>());
  void skip();
  Node* skipNode();
  Element* tryGetCurrentElement();

  // Virtual element functions
  void attr(const String& name, const ScriptValue& value);
  void key(const String& key);
  void applyAttrs(const ScriptValue& attrs = ScriptValue());
  void applyStatics(
      const absl::optional<HeapVector<ScriptValue>>& statics = absl::nullopt,
      const ScriptValue& attrs = ScriptValue());
  void elementOpenStart(
      const String& name_or_ctor,
      const String& key = String(),
      const absl::optional<HeapVector<ScriptValue>>& statics = absl::nullopt);
  Element* elementOpenEnd();
  Element* elementOpen(
      const String& name_or_ctor,
      const String& key = String(),
      const absl::optional<HeapVector<ScriptValue>>& statics = absl::nullopt,
      const HeapVector<ScriptValue>& var_args = HeapVector<ScriptValue>());
  Element* elementClose(const String& name_or_ctor);
  Element* elementVoid(
      const String& name_or_ctor,
      const String& key = String(),
      const absl::optional<HeapVector<ScriptValue>>& statics = absl::nullopt,
      const HeapVector<ScriptValue>& var_args = HeapVector<ScriptValue>());

  void Trace(Visitor*) const override;

 private:
  // OPTIMIZED NATIVE IMPLEMENTATIONS
  // These methods use the high-performance native C++ APIs
  void setAttributeOptimized(const String& name, const ScriptValue& value);
  Element* elementOpenOptimized(const String& name_or_ctor, const String& key);
  Text* textOptimized(const String& value);

  // Fast path for patch operations using native APIs
  Node* patchInnerOptimized(Element* el,
                            V8PatchFunction* template_function,
                            ScriptValue data);
  Node* patchOuterOptimized(Element* el,
                            V8PatchFunction* template_function,
                            ScriptValue data);
  Member<IDomNotification> notifications_;
  Member<IDomSymbols> symbols_;
  cobalt::h5vcc::idom::NodeDataMap node_data_map_;
  ScriptValue attributes_;

  // Virtual element builders (optimized native versions)
  cobalt::h5vcc::idom::NativeVirtualElements::ElementArgs native_args_builder_;
  cobalt::h5vcc::idom::AttributeBuilder native_attrs_builder_;

  // Legacy V8 builders for compatibility
  HeapVector<ScriptValue> args_builder_;
  HeapVector<ScriptValue> attrs_builder_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_H_5_VCC_IDOM_H_

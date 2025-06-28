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
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/attributes.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/core.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_attribute_map.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_context.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_notification.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_patcher.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_symbols.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/node_data.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"

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
  IDomSymbols* symbols();
  void clearCache(Node* node);
  String getKey(Node* node);
  void importNode(Node* node);
  bool isDataInitialized(Node* node);
  void applyAttr(Element* el, const String& name, const String& value);
  void applyProp(Element* el, const String& name, const String& value);
  IDomAttributeMap* attributes();
  IDomAttributeMap* createAttributeMap();
  void alignWithDOM(const String& name_or_ctor,
                    const String& key,
                    const String& nonce);
  void alwaysDiffAttributes(Element* el);
  Element* close();
  IDomPatcher* createPatchInner(const PatchConfig* config);
  IDomPatcher* createPatchOuter(const PatchConfig* config);
  IDomContext* currentContext();
  Element* currentElement();
  Node* currentPointer();
  Node* getNextNode();
  Element* open(const String& name_or_ctor,
                const String& key,
                const String& nonce);
  IDomPatcher* patchInner(Element* el,
                          V8PatchFunction* template_function,
                          ScriptValue data);
  IDomPatcher* patchOuter(Element* el,
                          V8PatchFunction* template_function,
                          ScriptValue data);
  void skip();
  Node* skipNode();
  Element* tryGetCurrentElement();

  void Trace(Visitor*) const override;

 private:
  Member<IDomNotification> notifications_;
  Member<IDomSymbols> symbols_;
  cobalt::h5vcc::idom::NodeDataMap node_data_map_;
  Member<IDomAttributeMap> attributes_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_H_5_VCC_IDOM_H_

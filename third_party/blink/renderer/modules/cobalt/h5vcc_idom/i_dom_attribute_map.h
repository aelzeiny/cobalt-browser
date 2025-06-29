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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_I_DOM_ATTRIBUTE_MAP_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_I_DOM_ATTRIBUTE_MAP_H_

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/attributes.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class ScriptState;
class ScriptValue;
class V8AttrMutator;

class IDomAttributeMap final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  IDomAttributeMap();

  // Static factory method to create a JavaScript object with attribute mutators
  static ScriptValue CreateJavaScriptAttributeMap(ScriptState* script_state);

  // Static methods to create the default mutator functions
  static V8AttrMutator* CreateDefaultMutator(ScriptState* script_state);
  static V8AttrMutator* CreateStyleMutator(ScriptState* script_state);

  V8AttrMutator* defaultValue(ScriptState* script_state) const;
  void setDefaultValue(ScriptState* script_state, V8AttrMutator* value);

  V8AttrMutator* style(ScriptState* script_state) const;
  void setStyle(ScriptState* script_state, V8AttrMutator* value);

  // Map-like operations for custom attribute mutators
  V8AttrMutator* AnonymousNamedGetter(ScriptState* script_state,
                                      const String& name);
  void AnonymousNamedSetter(const String& name, V8AttrMutator* mutator);
  bool AnonymousNamedDeleter(const String& name);

  // Internal method to get mutator with fallback to default
  V8AttrMutator* GetMutator(ScriptState* script_state,
                            const String& name) const;

  void Trace(Visitor* visitor) const override;

 private:
  Member<V8AttrMutator> default_value_;
  Member<V8AttrMutator> style_;
  HeapHashMap<String, Member<V8AttrMutator>> custom_mutators_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_I_DOM_ATTRIBUTE_MAP_H_

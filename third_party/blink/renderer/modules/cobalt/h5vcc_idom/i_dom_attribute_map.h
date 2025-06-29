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

  V8AttrMutator* defaultValue() const;
  void setDefaultValue(V8AttrMutator* value);

  V8AttrMutator* style() const;
  void setStyle(V8AttrMutator* value);

  void Trace(Visitor* visitor) const override;

 private:
  Member<V8AttrMutator> default_value_;
  Member<V8AttrMutator> style_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_I_DOM_ATTRIBUTE_MAP_H_

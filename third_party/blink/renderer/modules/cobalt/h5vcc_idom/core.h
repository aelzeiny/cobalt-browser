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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_CORE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_CORE_H_

#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_patch_function.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/context.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/node_data.h"

namespace blink {
class V8VoidCallback;
}

namespace cobalt {
namespace h5vcc {
namespace idom {

void AlignWithDOM(NodeDataMap& data_map,
                  const WTF::String& name_or_ctor,
                  const WTF::String& key,
                  const WTF::String& nonce);
void AlwaysDiffAttributes(blink::Element* el);
blink::Element* Close();
Context* CurrentContext();
blink::Element* CurrentElement();
blink::Node* CurrentPointer();
blink::Node* GetNextNode();
blink::Element* Open(NodeDataMap& data_map,
                     const WTF::String& name_or_ctor,
                     const WTF::String& key,
                     const WTF::String& nonce);
void Skip();
blink::Node* SkipNode();
blink::Element* TryGetCurrentElement();

void PatchInner(blink::Element* node,
                blink::V8PatchFunction* template_function,
                blink::ScriptValue data);
void PatchOuter(blink::Element* node,
                blink::V8PatchFunction* template_function,
                blink::ScriptValue data);

// Overload for V8VoidCallback
void PatchInner(blink::Element* node,
                blink::V8VoidCallback* template_function,
                blink::ScriptValue data);

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_CORE_H_

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

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_patcher.h"

#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_patch_function.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/core.h"

namespace blink {

IDomPatcher::IDomPatcher(bool is_outer) : is_outer_(is_outer) {}

void IDomPatcher::patch(Element* el,
                        V8PatchFunction* template_function,
                        ScriptValue data) {
  if (is_outer_) {
    cobalt::h5vcc::idom::PatchOuter(el, template_function, data);
  } else {
    cobalt::h5vcc::idom::PatchInner(el, template_function, data);
  }
}

void IDomPatcher::Trace(Visitor* visitor) const {
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink

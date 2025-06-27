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

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/h_5_vcc_idom.h"

#include "base/logging.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_void_callback.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"

namespace blink {

H5vccIdom::H5vccIdom(LocalDOMWindow& window)
    : ExecutionContextLifecycleObserver(window.GetExecutionContext()) {}

void H5vccIdom::ContextDestroyed() {}

void H5vccIdom::patch(Element* element, V8VoidCallback* function) {
  DLOG(INFO) << "H5vccIdom::patch called";

  if (!function) {
    DLOG(WARNING) << "H5vccIdom::patch - No function provided";
    return;
  }

  DLOG(INFO) << "H5vccIdom::patch - About to invoke callback";
  // Immediately call back the JavaScript function
  function->InvokeAndReportException(
      GetExecutionContext()->ToScriptWrappable());
  DLOG(INFO) << "H5vccIdom::patch - Callback invoked";
}

void H5vccIdom::Trace(Visitor* visitor) const {
  ExecutionContextLifecycleObserver::Trace(visitor);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink

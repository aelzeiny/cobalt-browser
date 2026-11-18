// Copyright 2026 The Cobalt Authors. All Rights Reserved.
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

#include "third_party/blink/renderer/modules/cobalt/wasm/h_5_vcc_wasm_imports.h"

#include "cobalt/wasm/starboard/event_queue.h"
#include "cobalt/wasm/starboard/guest_memory.h"
#include "cobalt/wasm/starboard/wasm_player_host.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/workers/dedicated_worker_global_scope.h"
#include "third_party/blink/renderer/modules/cobalt/wasm/h_5_vcc_wasm_net.h"
#include "third_party/blink/renderer/modules/cobalt/wasm/h_5_vcc_wasm_player.h"
#include "third_party/blink/renderer/modules/cobalt/wasm/h_5_vcc_wasm_surface.h"
#include "third_party/blink/renderer/modules/event_target_modules_names.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/persistent.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "v8/include/v8-array-buffer.h"
#include "v8/include/v8-value.h"
#include "v8/include/v8-wasm.h"

namespace blink {

namespace {

// Event names are per-thread AtomicStrings because this runs on workers as
// well as the main window thread.
AtomicString WakeEventType() {
  return AtomicString("wake");
}

bool IsAllowedOrigin(const SecurityOrigin* origin) {
  if (!origin || origin->IsOpaque()) {
    return false;
  }
  const String& protocol = origin->Protocol();
  if (origin->IsLocalhost()) {
    return protocol == "https" || protocol == "http";
  }
  if (protocol != "https") {
    return false;
  }
  const String& host = origin->Host();
  return host == "youtube.com" || host.EndsWith(".youtube.com") ||
         host == "youtube-nocookie.com" ||
         host.EndsWith(".youtube-nocookie.com") || host == "youtubekids.com" ||
         host.EndsWith(".youtubekids.com");
}

}  // namespace

// static
const char H5vccWasmImports::kSupplementName[] = "H5vccWasmImports";

// static
H5vccWasmImports* H5vccWasmImports::h5vccWasmImports(LocalDOMWindow& window) {
  // Only a top-level YouTube or localhost document controls the platform;
  // third-party sites and embedded content (ads, widgets) must not.
  LocalFrame* frame = window.GetFrame();
  if (!frame || !frame->IsOutermostMainFrame() ||
      !IsAllowedOrigin(window.GetSecurityOrigin())) {
    return nullptr;
  }
  return FromContext(window.GetExecutionContext());
}

// static
H5vccWasmImports* H5vccWasmImports::h5vccWasmImports(
    DedicatedWorkerGlobalScope& worker) {
  // A worker can't tell which frame created it, so require an allowed
  // top-level frame origin that matches the worker's own origin: that admits
  // YouTube/localhost main-frame workers and excludes third-party or
  // cross-origin iframe workers.
  const SecurityOrigin* top_origin = worker.top_level_frame_security_origin();
  if (!IsAllowedOrigin(top_origin) ||
      !top_origin->IsSameOriginWith(worker.GetSecurityOrigin())) {
    return nullptr;
  }
  return FromContext(worker.GetExecutionContext());
}

// static
H5vccWasmImports* H5vccWasmImports::FromContext(ExecutionContext* context) {
  if (!context || context->IsContextDestroyed()) {
    return nullptr;
  }
  H5vccWasmImports* provider =
      Supplement<ExecutionContext>::From<H5vccWasmImports>(*context);
  if (!provider) {
    provider = MakeGarbageCollected<H5vccWasmImports>(*context);
    ProvideTo(*context, provider);
  }
  return provider;
}

H5vccWasmImports::H5vccWasmImports(ExecutionContext& context)
    : Supplement<ExecutionContext>(context),
      ExecutionContextLifecycleObserver(&context),
      events_(base::MakeRefCounted<cobalt::EventQueue>(
          context.GetTaskRunner(TaskType::kInternalMedia),
          WTF::BindRepeating(&H5vccWasmImports::DispatchWake,
                             WrapWeakPersistent(this)))),
      host_(std::make_unique<cobalt::WasmPlayerHost>(
          events_,
          cobalt::WasmPlayerHost::DefaultSbPlayerInterface())),
      surface_(MakeGarbageCollected<H5vccWasmSurface>(*this)),
      player_(MakeGarbageCollected<H5vccWasmPlayer>(*this)),
      net_(MakeGarbageCollected<H5vccWasmNet>(*this)) {}

H5vccWasmImports::~H5vccWasmImports() = default;

const AtomicString& H5vccWasmImports::InterfaceName() const {
  return event_target_names::kH5VccWasmImports;
}

ExecutionContext* H5vccWasmImports::GetExecutionContext() const {
  return ExecutionContextLifecycleObserver::GetExecutionContext();
}

void H5vccWasmImports::bindMemory(ScriptState* script_state,
                                  const ScriptValue& memory,
                                  ExceptionState& exception_state) {
  if (!host_) {
    exception_state.ThrowDOMException(DOMExceptionCode::kInvalidStateError,
                                      "The context is gone.");
    return;
  }
  v8::Local<v8::Value> value = memory.V8Value();
  if (value.IsEmpty() || value->IsNull()) {
    memory_.Reset();
    return;
  }
  if (!value->IsWasmMemoryObject()) {
    exception_state.ThrowTypeError("bindMemory() expects a WebAssembly.Memory");
    return;
  }
  v8::Local<v8::WasmMemoryObject> wasm_memory =
      value.As<v8::WasmMemoryObject>();
  std::shared_ptr<v8::BackingStore> backing_store =
      wasm_memory->Buffer()->GetBackingStore();
  if (!backing_store || backing_store->IsShared()) {
    exception_state.ThrowTypeError(
        "bindMemory() does not support shared WebAssembly.Memory");
    return;
  }
  memory_.Reset(script_state->GetIsolate(), wasm_memory);
}

bool H5vccWasmImports::discardPages(v8::Isolate* isolate,
                                    uint32_t offset,
                                    uint32_t size) {
  if (!host_) {
    return false;
  }
  cobalt::GuestMemory memory = Memory(isolate);
  return memory.DiscardPages(offset, size);
}

cobalt::GuestMemory H5vccWasmImports::Memory(v8::Isolate* isolate) const {
  return cobalt::GuestMemory(isolate, memory_);
}

EventListener* H5vccWasmImports::onwake() {
  return GetAttributeEventListener(WakeEventType());
}

void H5vccWasmImports::setOnwake(EventListener* listener) {
  SetAttributeEventListener(WakeEventType(), listener);
}

void H5vccWasmImports::ContextDestroyed() {
  net_->Shutdown();
  host_.reset();
  cobalt::WasmPlayerHost::ResetForCurrentThread();
  // Players being destroyed keep the queue alive; stop it calling back here.
  events_->Shutdown();
  events_.reset();
  memory_.Reset();
}

void H5vccWasmImports::Trace(Visitor* visitor) const {
  Supplement<ExecutionContext>::Trace(visitor);
  ExecutionContextLifecycleObserver::Trace(visitor);
  EventTarget::Trace(visitor);
  visitor->Trace(surface_);
  visitor->Trace(player_);
  visitor->Trace(net_);
}

void H5vccWasmImports::DispatchWake() {
  if (!events_ || !GetExecutionContext()) {
    return;
  }
  DispatchEvent(*Event::Create(WakeEventType()));
}

}  // namespace blink

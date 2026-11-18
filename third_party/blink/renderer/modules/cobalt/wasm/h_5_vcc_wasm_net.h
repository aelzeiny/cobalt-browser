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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_WASM_H_5_VCC_WASM_NET_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_WASM_H_5_VCC_WASM_NET_H_

#include <stddef.h>
#include <stdint.h>

#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "v8/include/v8-forward.h"

namespace cobalt {
class EventQueue;
}  // namespace cobalt

namespace blink {

class ExceptionState;
class H5vccWasmImports;
class Response;
class ScriptState;

// Implements h_5_vcc_wasm_net.idl. Owned by H5vccWasmImports and used only on
// its ExecutionContext's thread.
class MODULES_EXPORT H5vccWasmNet final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  explicit H5vccWasmNet(H5vccWasmImports& imports);
  ~H5vccWasmNet() override;

  int32_t adoptResponse(ScriptState* script_state,
                        Response* response,
                        ExceptionState& exception_state);
  int32_t read(v8::Isolate* isolate,
               int32_t stream_id,
               uint32_t dst_offset,
               uint32_t capacity);
  void close(int32_t stream_id);

  void Shutdown();
  size_t OpenStreamsForTesting() const;

  void Trace(Visitor* visitor) const override;

 private:
  class StreamLoader;
  class NetLoaderClient;
  struct State;

  cobalt::EventQueue* events() const;

  Member<H5vccWasmImports> imports_;
  Member<State> state_;
  int32_t next_stream_id_ = 1;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_WASM_H_5_VCC_WASM_NET_H_

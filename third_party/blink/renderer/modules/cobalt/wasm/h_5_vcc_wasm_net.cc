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

#include "third_party/blink/renderer/modules/cobalt/wasm/h_5_vcc_wasm_net.h"

#include <algorithm>
#include <limits>

#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "cobalt/wasm/starboard/event_queue.h"
#include "cobalt/wasm/starboard/guest_memory.h"
#include "cobalt/wasm/starboard/wasm_starboard_abi.h"
#include "services/network/public/mojom/fetch_api.mojom-blink.h"
#include "third_party/blink/renderer/core/fetch/body_stream_buffer.h"
#include "third_party/blink/renderer/core/fetch/fetch_data_loader.h"
#include "third_party/blink/renderer/core/fetch/fetch_response_data.h"
#include "third_party/blink/renderer/core/fetch/response.h"
#include "third_party/blink/renderer/modules/cobalt/wasm/h_5_vcc_wasm_imports.h"
#include "third_party/blink/renderer/platform/bindings/exception_code.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_hash_map.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/loader/fetch/bytes_consumer.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class H5vccWasmNet::StreamLoader final : public FetchDataLoader,
                                         public BytesConsumer::Client {
 public:
  enum class StreamState {
    kOpen,
    kEof,
    kError,
    kAborted,
    kClosed,
  };

  StreamLoader(H5vccWasmNet& owner,
               int32_t stream_id,
               StreamState initial_state = StreamState::kOpen)
      : owner_(&owner), stream_id_(stream_id), state_(initial_state) {}

  void Start(BytesConsumer* consumer, FetchDataLoader::Client* client) override {
    DCHECK(!consumer_);
    DCHECK(!loader_client_);
    if (state_ != StreamState::kOpen) {
      consumer->Cancel();
      client->DidFetchDataLoadedCustomFormat();
      return;
    }
    consumer_ = consumer;
    loader_client_ = client;
    consumer_->SetClient(this);
    switch (consumer_->GetPublicState()) {
      case BytesConsumer::PublicState::kReadableOrWaiting:
        break;
      case BytesConsumer::PublicState::kClosed:
        FinishBackend(StreamState::kEof, /*notify_loader_client=*/true);
        break;
      case BytesConsumer::PublicState::kErrored:
        FinishBackend(StreamState::kError, /*notify_loader_client=*/true);
        break;
    }
  }

  void OnStateChange() override {
    if (!consumer_ || state_ != StreamState::kOpen) {
      return;
    }
    switch (consumer_->GetPublicState()) {
      case BytesConsumer::PublicState::kReadableOrWaiting:
        break;
      case BytesConsumer::PublicState::kClosed:
        FinishBackend(StreamState::kEof, /*notify_loader_client=*/true);
        break;
      case BytesConsumer::PublicState::kErrored:
        FinishBackend(StreamState::kError, /*notify_loader_client=*/true);
        break;
    }
    NotifyReadyIfWaiting();
  }

  String DebugName() const override { return "H5vccWasmNet::StreamLoader"; }

  // Called by BodyStreamBuffer::StopLoading() on context destruction; per
  // FetchDataLoader contract, `loader_client_` methods must not be invoked.
  void Cancel() override {
    waiting_for_readable_ = false;
    FinishBackend(StreamState::kClosed, /*notify_loader_client=*/false);
  }

  void OnSignalAbort() {
    FinishBackend(StreamState::kAborted, /*notify_loader_client=*/true);
    NotifyReadyIfWaiting();
  }

  void CloseFromHost(bool notify_loader_client) {
    waiting_for_readable_ = false;
    FinishBackend(StreamState::kClosed, notify_loader_client);
  }

  int32_t Read(cobalt::GuestMemory& memory,
               uint32_t dst_offset,
               uint32_t capacity) {
    if (capacity == 0 ||
        capacity >
            static_cast<uint32_t>(std::numeric_limits<int32_t>::max())) {
      return cobalt::kWasmNetError;
    }
    uint8_t* raw_dst = memory.Bytes(dst_offset, capacity);
    if (!raw_dst) {
      return cobalt::kWasmNetError;
    }
    // SAFETY: GuestMemory::Bytes() verified that [dst_offset, dst_offset +
    // capacity) lies inside the bound non-shared WebAssembly.Memory.
    base::span<uint8_t> dst = UNSAFE_BUFFERS(base::span(raw_dst, capacity));

    switch (state_) {
      case StreamState::kEof:
        return cobalt::kWasmNetEof;
      case StreamState::kError:
      case StreamState::kClosed:
        return cobalt::kWasmNetError;
      case StreamState::kAborted:
        return cobalt::kWasmNetAborted;
      case StreamState::kOpen:
        break;
    }
    if (!consumer_) {
      FinishBackend(StreamState::kEof, /*notify_loader_client=*/true);
      return cobalt::kWasmNetEof;
    }

    size_t total_copied = 0;
    while (total_copied < dst.size()) {
      base::span<const char> chunk;
      BytesConsumer::Result res = consumer_->BeginRead(chunk);
      if (res == BytesConsumer::Result::kShouldWait) {
        if (total_copied > 0) {
          return static_cast<int32_t>(total_copied);
        }
        waiting_for_readable_ = true;
        return cobalt::kWasmNetPending;
      }
      if (res == BytesConsumer::Result::kDone) {
        FinishBackend(StreamState::kEof, /*notify_loader_client=*/true);
        return total_copied > 0 ? static_cast<int32_t>(total_copied)
                                : cobalt::kWasmNetEof;
      }
      if (res == BytesConsumer::Result::kError) {
        FinishBackend(StreamState::kError, /*notify_loader_client=*/true);
        return total_copied > 0 ? static_cast<int32_t>(total_copied)
                                : cobalt::kWasmNetError;
      }
      DCHECK_EQ(res, BytesConsumer::Result::kOk);
      const size_t to_copy = std::min(chunk.size(), dst.size() - total_copied);
      if (to_copy > 0) {
        dst.subspan(total_copied, to_copy)
            .copy_from(base::as_bytes(chunk.first(to_copy)));
      }
      res = consumer_->EndRead(to_copy);
      total_copied += to_copy;
      if (res == BytesConsumer::Result::kDone) {
        FinishBackend(StreamState::kEof, /*notify_loader_client=*/true);
        return total_copied > 0 ? static_cast<int32_t>(total_copied)
                                : cobalt::kWasmNetEof;
      }
      if (res == BytesConsumer::Result::kError) {
        FinishBackend(StreamState::kError, /*notify_loader_client=*/true);
        return total_copied > 0 ? static_cast<int32_t>(total_copied)
                                : cobalt::kWasmNetError;
      }
    }
    return static_cast<int32_t>(total_copied);
  }

  void Trace(Visitor* visitor) const override {
    visitor->Trace(owner_);
    visitor->Trace(consumer_);
    visitor->Trace(loader_client_);
    FetchDataLoader::Trace(visitor);
    BytesConsumer::Client::Trace(visitor);
  }

 private:
  void FinishBackend(StreamState terminal_state, bool notify_loader_client) {
    if (state_ == StreamState::kOpen) {
      state_ = terminal_state;
    }
    if (consumer_) {
      BytesConsumer* consumer = consumer_.Release();
      consumer->ClearClient();
      if (terminal_state != StreamState::kEof) {
        consumer->Cancel();
      }
    }
    if (loader_client_) {
      FetchDataLoader::Client* client = loader_client_.Release();
      if (notify_loader_client) {
        client->DidFetchDataLoadedCustomFormat();
      }
    }
  }

  void NotifyReadyIfWaiting() {
    if (!waiting_for_readable_) {
      return;
    }
    waiting_for_readable_ = false;
    if (cobalt::EventQueue* queue = owner_->events()) {
      queue->PushAndWake({cobalt::kWasmSbEventNetReady, 0, stream_id_, 0, 0});
    }
  }

  Member<H5vccWasmNet> owner_;
  const int32_t stream_id_;
  Member<BytesConsumer> consumer_;
  Member<FetchDataLoader::Client> loader_client_;
  StreamState state_;
  bool waiting_for_readable_ = false;
};

class H5vccWasmNet::NetLoaderClient final
    : public GarbageCollected<NetLoaderClient>,
      public FetchDataLoader::Client {
 public:
  explicit NetLoaderClient(StreamLoader& stream) : stream_(&stream) {}

  void DidFetchDataLoadedCustomFormat() override {}
  void DidFetchDataLoadFailed() override { stream_->OnSignalAbort(); }
  void Abort() override { stream_->OnSignalAbort(); }

  void Trace(Visitor* visitor) const override {
    visitor->Trace(stream_);
    FetchDataLoader::Client::Trace(visitor);
  }

 private:
  Member<StreamLoader> stream_;
};

struct H5vccWasmNet::State final : public GarbageCollected<State> {
  HeapHashMap<int32_t, Member<StreamLoader>> streams;
  size_t live_count = 0;
  size_t closed_tombstones = 0;

  void Trace(Visitor* visitor) const { visitor->Trace(streams); }
};

H5vccWasmNet::H5vccWasmNet(H5vccWasmImports& imports)
    : imports_(&imports), state_(MakeGarbageCollected<State>()) {}

H5vccWasmNet::~H5vccWasmNet() = default;

cobalt::EventQueue* H5vccWasmNet::events() const {
  return imports_->events();
}

int32_t H5vccWasmNet::adoptResponse(ScriptState* script_state,
                                    Response* response,
                                    ExceptionState& exception_state) {
  if (!imports_->host() || !imports_->events() ||
      !script_state->ContextIsValid()) {
    exception_state.ThrowDOMException(DOMExceptionCode::kInvalidStateError,
                                      "The context is gone.");
    return 0;
  }
  if (!response) {
    exception_state.ThrowTypeError("adoptResponse() expects a Response");
    return 0;
  }
  const network::mojom::FetchResponseType response_type =
      response->GetResponse()->GetType();
  if (response_type == network::mojom::FetchResponseType::kOpaque ||
      response_type == network::mojom::FetchResponseType::kOpaqueRedirect ||
      response_type == network::mojom::FetchResponseType::kError) {
    exception_state.ThrowTypeError(
        "Cannot adopt an opaque or network-error Response");
    return 0;
  }
  if (response->IsBodyLocked() || response->IsBodyUsed()) {
    exception_state.ThrowTypeError("Response body is already locked or used");
    return 0;
  }
  if (next_stream_id_ <= 0 ||
      next_stream_id_ == std::numeric_limits<int32_t>::max()) {
    exception_state.ThrowDOMException(DOMExceptionCode::kInvalidStateError,
                                      "Stream handle space exhausted.");
    return 0;
  }

  // Sweep nulled entries here on the JS slow path rather than erasing inside
  // [NoAllocDirectCall] close(), where HeapHashMap::erase() could trigger an
  // Oilpan table-shrink allocation under --wasm-fast-api.
  if (state_->closed_tombstones >= 8) {
    Vector<int32_t, 16> dead_keys;
    for (const auto& entry : state_->streams) {
      if (!entry.value) {
        dead_keys.push_back(entry.key);
      }
    }
    state_->streams.RemoveAll(dead_keys);
    state_->closed_tombstones = 0;
  }

  const int32_t stream_id = next_stream_id_++;
  BodyStreamBuffer* body_buffer = response->BodyBuffer();
  if (!body_buffer) {
    state_->streams.Set(
        stream_id, MakeGarbageCollected<StreamLoader>(
                       *this, stream_id, StreamLoader::StreamState::kEof));
    ++state_->live_count;
    return stream_id;
  }

  auto* stream = MakeGarbageCollected<StreamLoader>(*this, stream_id);
  auto* client = MakeGarbageCollected<NetLoaderClient>(*stream);
  state_->streams.Set(stream_id, stream);
  ++state_->live_count;
  body_buffer->StartLoading(stream, client, exception_state);
  if (exception_state.HadException()) {
    state_->streams.erase(stream_id);
    --state_->live_count;
    return 0;
  }
  return stream_id;
}

int32_t H5vccWasmNet::read(v8::Isolate* isolate,
                           int32_t stream_id,
                           uint32_t dst_offset,
                           uint32_t capacity) {
  if (!imports_->host() || stream_id <= 0) {
    return cobalt::kWasmNetError;
  }
  auto it = state_->streams.find(stream_id);
  if (it == state_->streams.end() || !it->value) {
    return cobalt::kWasmNetError;
  }
  cobalt::GuestMemory memory = imports_->Memory(isolate);
  return it->value->Read(memory, dst_offset, capacity);
}

void H5vccWasmNet::close(int32_t stream_id) {
  if (stream_id <= 0) {
    return;
  }
  auto it = state_->streams.find(stream_id);
  if (it == state_->streams.end() || !it->value) {
    return;
  }
  StreamLoader* stream = it->value.Release();
  DCHECK_GT(state_->live_count, 0u);
  --state_->live_count;
  ++state_->closed_tombstones;
  stream->CloseFromHost(/*notify_loader_client=*/true);
  if (cobalt::EventQueue* queue = imports_->events()) {
    queue->DropNetStream(stream_id);
  }
}

void H5vccWasmNet::Shutdown() {
  for (auto& entry : state_->streams) {
    if (entry.value) {
      entry.value->CloseFromHost(/*notify_loader_client=*/false);
    }
  }
  state_->streams.clear();
  state_->live_count = 0;
  state_->closed_tombstones = 0;
}

size_t H5vccWasmNet::OpenStreamsForTesting() const {
  return state_->live_count;
}

void H5vccWasmNet::Trace(Visitor* visitor) const {
  visitor->Trace(imports_);
  visitor->Trace(state_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink

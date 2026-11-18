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

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "cobalt/wasm/starboard/wasm_player_host.h"
#include "cobalt/wasm/starboard/wasm_starboard_abi.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "starboard/extension/experimental/experimental_features.h"
#include "starboard/extension/player_settings.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_testing.h"
#include "third_party/blink/renderer/core/dom/abort_controller.h"
#include "third_party/blink/renderer/core/fetch/body_stream_buffer.h"
#include "third_party/blink/renderer/core/fetch/fetch_response_data.h"
#include "third_party/blink/renderer/core/fetch/response.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"
#include "third_party/blink/renderer/modules/cobalt/wasm/h_5_vcc_wasm_net.h"
#include "third_party/blink/renderer/modules/cobalt/wasm/h_5_vcc_wasm_player.h"
#include "third_party/blink/renderer/modules/cobalt/wasm/h_5_vcc_wasm_surface.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/loader/fetch/data_pipe_bytes_consumer.h"
#include "third_party/blink/renderer/platform/testing/task_environment.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-script.h"
#include "v8/include/v8-wasm.h"

namespace blink {
namespace {

using cobalt::WasmPlayerHost;

// The extension calls made on this thread, or nullopt for none.
std::optional<size_t>& FeatureCount() {
  static std::optional<size_t> count;
  return count;
}
std::optional<int>& MaxVideoInputSize() {
  static std::optional<int> size;
  return size;
}
std::optional<std::string>& MaxVideoResolution() {
  static std::optional<std::string> resolution;
  return resolution;
}

constexpr StarboardExtensionExperimentalFeaturesConfigurationApi kFeaturesApi =
    {kStarboardExtensionExperimentalFeaturesConfigurationName, 1,
     [](const StarboardExtensionExperimentalFeatures* features) {
       FeatureCount() = features->entry_count;
     }};

constexpr StarboardExtensionPlayerSettingsApi kPlayerSettingsApi = {
    kStarboardExtensionPlayerSettingsName, 1,
    [](int size) { MaxVideoInputSize() = size; },
    [](const char* resolution) {
      MaxVideoResolution() = resolution ? resolution : "<null>";
    },
    nullptr};

class H5vccWasmImportsTest : public testing::Test {
 protected:
  H5vccWasmImportsTest() {
    WasmPlayerHost::SetExperimentalFeaturesApiForTesting(&kFeaturesApi);
    WasmPlayerHost::SetPlayerSettingsApiForTesting(&kPlayerSettingsApi);
    FeatureCount().reset();
    MaxVideoInputSize().reset();
    MaxVideoResolution().reset();
  }
  ~H5vccWasmImportsTest() override {
    WasmPlayerHost::SetPlayerSettingsApiForTesting(nullptr);
    WasmPlayerHost::SetExperimentalFeaturesApiForTesting(nullptr);
  }

  test::TaskEnvironment task_environment_;
  std::unique_ptr<DummyPageHolder> page_holder_ =
      std::make_unique<DummyPageHolder>();
};

TEST_F(H5vccWasmImportsTest, ContextDestroyedResetsThreadSettings) {
  auto* imports = MakeGarbageCollected<H5vccWasmImports>(
      *page_holder_->GetFrame().DomWindow());
  H5vccWasmPlayer* player = imports->player();
  ASSERT_TRUE(player->setMaxVideoInputSizeForCurrentThread(1024));
  EXPECT_EQ(MaxVideoInputSize(), 1024);
  EXPECT_FALSE(FeatureCount());

  page_holder_.reset();
  EXPECT_FALSE(imports->host());
  EXPECT_EQ(FeatureCount(), 0u);
  EXPECT_EQ(MaxVideoInputSize(), 0);
  EXPECT_EQ(MaxVideoResolution(), "<null>");
}

TEST_F(H5vccWasmImportsTest, OperationsValidateHandles) {
  auto* imports = MakeGarbageCollected<H5vccWasmImports>(
      *page_holder_->GetFrame().DomWindow());
  H5vccWasmPlayer* player = imports->player();
  H5vccWasmSurface* surface = imports->surface();
  EXPECT_FALSE(player->seek(/*player=*/7, 0, 0));
  EXPECT_FALSE(player->writeEndOfStream(/*player=*/7, /*stream_type=*/0));
  EXPECT_FALSE(player->setPlaybackRate(/*player=*/7, 1));
  EXPECT_EQ(player->getMaximumNumberOfSamplesPerWrite(/*player=*/7, 0), 0);
  const uint32_t id = surface->create();
  EXPECT_NE(id, 0u);
  EXPECT_EQ(surface->token(id).length(), 32u);
  EXPECT_TRUE(surface->token(id + 1).empty());
  surface->destroy(id);
  EXPECT_TRUE(surface->token(id).empty());

  // Once the context is gone, operations do nothing.
  page_holder_.reset();
  EXPECT_EQ(surface->create(), 0u);
  EXPECT_FALSE(player->setMaxVideoInputSizeForCurrentThread(1));
}

TEST_F(H5vccWasmImportsTest, BindNullUnbindsAndDiscardPagesZerosSlabs) {
  V8TestingScope scope;
  auto* imports =
      MakeGarbageCollected<H5vccWasmImports>(*scope.GetExecutionContext());
  v8::Isolate* isolate = scope.GetIsolate();
  v8::Local<v8::Context> context = scope.GetContext();
  v8::Local<v8::Value> mem_val =
      v8::Script::Compile(
          context,
          v8::String::NewFromUtf8Literal(
              isolate, "new WebAssembly.Memory({initial: 2, maximum: 4})"))
          .ToLocalChecked()
          ->Run(context)
          .ToLocalChecked();
  DummyExceptionStateForTesting exception_state;
  imports->bindMemory(scope.GetScriptState(), ScriptValue(isolate, mem_val),
                      exception_state);
  ASSERT_FALSE(exception_state.HadException());

  cobalt::GuestMemory gm = imports->Memory(isolate);
  uint8_t* ptr = gm.Bytes(65536, 65536);
  ASSERT_TRUE(ptr);
  // SAFETY: GuestMemory::Bytes() checked that [65536, 131072) fits in memory.
  UNSAFE_BUFFERS(base::span(ptr, 65536)).fill(0x5A);
  EXPECT_FALSE(imports->discardPages(isolate, 65537, 4096));
  ASSERT_TRUE(imports->discardPages(isolate, 65536, 65536));
  std::vector<uint8_t> readback;
  ASSERT_TRUE(gm.ReadBytes(65536, 65536, &readback));
  EXPECT_EQ(readback.front(), 0u);
  EXPECT_EQ(readback.back(), 0u);

  imports->bindMemory(scope.GetScriptState(),
                      ScriptValue(isolate, v8::Null(isolate)), exception_state);
  ASSERT_FALSE(exception_state.HadException());
  EXPECT_FALSE(imports->discardPages(isolate, 65536, 65536));
}

TEST_F(H5vccWasmImportsTest, NetReadsDataPipeWithoutJsBuffersAndEmitsNetReady) {
  V8TestingScope scope;
  auto* imports =
      MakeGarbageCollected<H5vccWasmImports>(*scope.GetExecutionContext());
  v8::Isolate* isolate = scope.GetIsolate();
  v8::Local<v8::Context> context = scope.GetContext();
  v8::Local<v8::Value> mem_val =
      v8::Script::Compile(
          context,
          v8::String::NewFromUtf8Literal(
              isolate, "new WebAssembly.Memory({initial: 2, maximum: 4})"))
          .ToLocalChecked()
          ->Run(context)
          .ToLocalChecked();
  DummyExceptionStateForTesting exception_state;
  imports->bindMemory(scope.GetScriptState(), ScriptValue(isolate, mem_val),
                      exception_state);
  ASSERT_FALSE(exception_state.HadException());

  mojo::ScopedDataPipeProducerHandle producer;
  mojo::ScopedDataPipeConsumerHandle consumer_pipe;
  ASSERT_EQ(mojo::CreateDataPipe(65536, producer, consumer_pipe),
            MOJO_RESULT_OK);
  DataPipeBytesConsumer::CompletionNotifier* notifier = nullptr;
  auto* bytes_consumer = MakeGarbageCollected<DataPipeBytesConsumer>(
      scope.GetExecutionContext()->GetTaskRunner(TaskType::kNetworking),
      std::move(consumer_pipe), &notifier);
  auto* body_buffer = BodyStreamBuffer::Create(
      scope.GetScriptState(), bytes_consumer, /*signal=*/nullptr,
      /*cached_metadata_handler=*/nullptr);
  FetchResponseData* response_data =
      FetchResponseData::CreateWithBuffer(body_buffer);
  Response* response =
      Response::Create(scope.GetExecutionContext(), response_data);

  H5vccWasmNet* net = imports->net();
  const int32_t stream_id =
      net->adoptResponse(scope.GetScriptState(), response, exception_state);
  ASSERT_FALSE(exception_state.HadException());
  ASSERT_GT(stream_id, 0);
  EXPECT_TRUE(response->IsBodyUsed());

  // Second adopt on the same Response fails because the body is locked/used.
  DummyExceptionStateForTesting second_exception;
  EXPECT_EQ(
      net->adoptResponse(scope.GetScriptState(), response, second_exception),
      0);
  EXPECT_TRUE(second_exception.HadException());

  // Opaque Response is rejected.
  DummyExceptionStateForTesting opaque_exception;
  Response* opaque_response = Response::Create(
      scope.GetExecutionContext(),
      FetchResponseData::Create()->CreateOpaqueFilteredResponse());
  EXPECT_EQ(net->adoptResponse(scope.GetScriptState(), opaque_response,
                               opaque_exception),
            0);
  EXPECT_TRUE(opaque_exception.HadException());

  // Pipe is initially empty -> returns kWasmNetPending (0) and arms wake.
  EXPECT_EQ(net->read(isolate, stream_id, 0, 1024), cobalt::kWasmNetPending);

  const uint8_t payload[] = {'u', 'm', 'p', '!', 1, 2, 3, 4};
  ASSERT_EQ(producer->WriteAllData(payload), MOJO_RESULT_OK);
  task_environment_.RunUntilIdle();

  // kWasmSbEventNetReady is queued in pollEvents().
  cobalt::WasmSbEvent ev = {};
  ASSERT_EQ(imports->player()->pollEvents(isolate, 4096, 4), 1u);
  ASSERT_TRUE(imports->Memory(isolate).Read(4096, &ev));
  EXPECT_EQ(ev.kind, cobalt::kWasmSbEventNetReady);
  EXPECT_EQ(ev.player, 0u);
  EXPECT_EQ(ev.a, stream_id);

  EXPECT_EQ(net->read(isolate, stream_id, 0, 1024),
            static_cast<int32_t>(sizeof(payload)));
  std::vector<uint8_t> guest_bytes;
  ASSERT_TRUE(
      imports->Memory(isolate).ReadBytes(0, sizeof(payload), &guest_bytes));
  EXPECT_EQ(base::span(guest_bytes), base::span(payload));

  // Close producer + signal completion -> clean EOF (-1).
  producer.reset();
  notifier->SignalComplete();
  task_environment_.RunUntilIdle();
  EXPECT_EQ(net->read(isolate, stream_id, 0, 1024), cobalt::kWasmNetEof);

  net->close(stream_id);
  EXPECT_EQ(net->OpenStreamsForTesting(), 0u);
  EXPECT_EQ(net->read(isolate, stream_id, 0, 1024), cobalt::kWasmNetError);
  EXPECT_EQ(net->read(isolate, 0, 0, 1024), cobalt::kWasmNetError);
  EXPECT_EQ(net->read(isolate, -1, 0, 1024), cobalt::kWasmNetError);
}

TEST_F(H5vccWasmImportsTest, NetReportsAbortSignal) {
  V8TestingScope scope;
  auto* imports =
      MakeGarbageCollected<H5vccWasmImports>(*scope.GetExecutionContext());
  v8::Isolate* isolate = scope.GetIsolate();
  v8::Local<v8::Context> context = scope.GetContext();
  v8::Local<v8::Value> mem_val =
      v8::Script::Compile(
          context,
          v8::String::NewFromUtf8Literal(
              isolate, "new WebAssembly.Memory({initial: 1, maximum: 2})"))
          .ToLocalChecked()
          ->Run(context)
          .ToLocalChecked();
  DummyExceptionStateForTesting exception_state;
  imports->bindMemory(scope.GetScriptState(), ScriptValue(isolate, mem_val),
                      exception_state);
  ASSERT_FALSE(exception_state.HadException());

  mojo::ScopedDataPipeProducerHandle producer;
  mojo::ScopedDataPipeConsumerHandle consumer_pipe;
  ASSERT_EQ(mojo::CreateDataPipe(65536, producer, consumer_pipe),
            MOJO_RESULT_OK);
  DataPipeBytesConsumer::CompletionNotifier* notifier = nullptr;
  auto* bytes_consumer = MakeGarbageCollected<DataPipeBytesConsumer>(
      scope.GetExecutionContext()->GetTaskRunner(TaskType::kNetworking),
      std::move(consumer_pipe), &notifier);
  AbortController* abort_controller =
      AbortController::Create(scope.GetScriptState());
  auto* body_buffer = BodyStreamBuffer::Create(
      scope.GetScriptState(), bytes_consumer, abort_controller->signal(),
      /*cached_metadata_handler=*/nullptr);
  Response* response =
      Response::Create(scope.GetExecutionContext(),
                       FetchResponseData::CreateWithBuffer(body_buffer));

  H5vccWasmNet* net = imports->net();
  const int32_t stream_id =
      net->adoptResponse(scope.GetScriptState(), response, exception_state);
  ASSERT_GT(stream_id, 0);
  EXPECT_EQ(net->read(isolate, stream_id, 0, 256), cobalt::kWasmNetPending);

  abort_controller->abort(scope.GetScriptState());
  task_environment_.RunUntilIdle();

  cobalt::WasmSbEvent ev = {};
  ASSERT_EQ(imports->player()->pollEvents(isolate, 4096, 4), 1u);
  ASSERT_TRUE(imports->Memory(isolate).Read(4096, &ev));
  EXPECT_EQ(ev.kind, cobalt::kWasmSbEventNetReady);
  EXPECT_EQ(ev.a, stream_id);
  EXPECT_EQ(net->read(isolate, stream_id, 0, 256), cobalt::kWasmNetAborted);
  net->close(stream_id);
}

TEST_F(H5vccWasmImportsTest, AllowsOnlyYouTubeAndLocalhostOrigins) {
  {
    V8TestingScope scope(KURL("https://www.youtube.com/tv"));
    EXPECT_NE(H5vccWasmImports::h5vccWasmImports(scope.GetWindow()), nullptr);
  }
  {
    V8TestingScope scope(KURL("https://web-release-qa.youtube.com/tv"));
    EXPECT_NE(H5vccWasmImports::h5vccWasmImports(scope.GetWindow()), nullptr);
  }
  {
    V8TestingScope scope(KURL("https://www.youtube-nocookie.com/embed/abc"));
    EXPECT_NE(H5vccWasmImports::h5vccWasmImports(scope.GetWindow()), nullptr);
  }
  {
    V8TestingScope scope(KURL("https://www.youtubekids.com/"));
    EXPECT_NE(H5vccWasmImports::h5vccWasmImports(scope.GetWindow()), nullptr);
  }
  {
    V8TestingScope scope(KURL("http://localhost:8080/"));
    EXPECT_NE(H5vccWasmImports::h5vccWasmImports(scope.GetWindow()), nullptr);
  }
  {
    V8TestingScope scope(KURL("http://127.0.0.1:9222/"));
    EXPECT_NE(H5vccWasmImports::h5vccWasmImports(scope.GetWindow()), nullptr);
  }
  {
    V8TestingScope scope(KURL("https://evil.example/"));
    EXPECT_EQ(H5vccWasmImports::h5vccWasmImports(scope.GetWindow()), nullptr);
  }
  {
    V8TestingScope scope(KURL("https://notyoutube.com/"));
    EXPECT_EQ(H5vccWasmImports::h5vccWasmImports(scope.GetWindow()), nullptr);
  }
  {
    V8TestingScope scope(KURL("http://www.youtube.com/tv"));
    EXPECT_EQ(H5vccWasmImports::h5vccWasmImports(scope.GetWindow()), nullptr);
  }
  {
    V8TestingScope scope(KURL("data:text/html,hello"));
    EXPECT_EQ(H5vccWasmImports::h5vccWasmImports(scope.GetWindow()), nullptr);
  }
}

}  // namespace
}  // namespace blink

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

#include "cobalt/wasm/starboard/guest_memory.h"

#include <algorithm>
#include <limits>
#include <memory>

#include "base/memory/page_size.h"
#include "build/build_config.h"
#include "v8/include/v8-array-buffer.h"
#include "v8/include/v8-local-handle.h"
#include "v8/include/v8-wasm.h"

#if BUILDFLAG(IS_POSIX)
#include <sys/mman.h>
#endif

namespace cobalt {

GuestMemory::GuestMemory(v8::Isolate* isolate,
                         const v8::Global<v8::WasmMemoryObject>& memory)
    : isolate_(isolate), memory_(&memory) {}

GuestMemory::GuestMemory(base::span<uint8_t> memory_for_testing)
    : loaded_(true),
      base_(memory_for_testing.data()),
      size_(memory_for_testing.size()) {}

uint8_t* GuestMemory::Bytes(uint32_t ptr, uint32_t len) {
  return Resolve(ptr, len);
}

uint8_t* GuestMemory::Array(uint32_t ptr, uint32_t count, size_t elem_size) {
  const uint64_t bytes = static_cast<uint64_t>(count) * elem_size;
  if (bytes > std::numeric_limits<uint32_t>::max()) {
    return nullptr;
  }
  return Resolve(ptr, bytes);
}

bool GuestMemory::ReadBytes(uint32_t ptr,
                            uint32_t size,
                            std::vector<uint8_t>* out) {
  if (size == 0) {
    out->clear();
    return true;
  }
  const uint8_t* bytes = Bytes(ptr, size);
  if (!bytes) {
    return false;
  }
  out->assign(bytes, bytes + size);
  return true;
}

bool GuestMemory::ReadString(uint32_t ptr, uint32_t size, std::string* out) {
  if (size == 0) {
    out->clear();
    return true;
  }
  const uint8_t* bytes = Bytes(ptr, size);
  if (!bytes) {
    return false;
  }
  out->assign(reinterpret_cast<const char*>(bytes), size);
  return true;
}

bool GuestMemory::DiscardPages(uint32_t offset, uint32_t size) {
#if BUILDFLAG(IS_POSIX)
  if (size == 0) {
    return false;
  }
  const size_t page_size = std::max<size_t>(4096u, base::GetPageSize());
  if ((offset % page_size) != 0 || (size % page_size) != 0) {
    return false;
  }
  uint8_t* ptr = Bytes(offset, size);
  if (!ptr || is_shared_ ||
      (reinterpret_cast<uintptr_t>(ptr) % page_size) != 0) {
    return false;
  }
  return madvise(ptr, size, MADV_DONTNEED) == 0;
#else
  return false;
#endif
}

uint8_t* GuestMemory::Resolve(uint32_t ptr, uint64_t len) {
  if (len == 0 || !Load()) {
    return nullptr;
  }
  if (ptr > size_ || len > size_ - ptr) {
    return nullptr;
  }
  return base_ + ptr;
}

bool GuestMemory::Load() {
  if (loaded_) {
    return base_ != nullptr;
  }
  loaded_ = true;
  if (!memory_ || memory_->IsEmpty() || !isolate_) {
    return false;
  }
  v8::HandleScope handle_scope(isolate_);
  v8::Local<v8::ArrayBuffer> buffer = memory_->Get(isolate_)->Buffer();
  std::shared_ptr<v8::BackingStore> backing_store = buffer->GetBackingStore();
  if (!backing_store || backing_store->IsShared()) {
    is_shared_ = true;
    return false;
  }
  base_ = static_cast<uint8_t*>(buffer->Data());
  size_ = buffer->ByteLength();
  return base_ != nullptr;
}

}  // namespace cobalt

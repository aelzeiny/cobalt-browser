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

#include "starboard/shared/modular/starboard_layer_posix_malloc_abi_wrappers.h"

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

namespace {

// Log allocation requests of at least this many bytes (16 MiB).
constexpr uint64_t kLargeAllocLogThreshold = 16 * 1024 * 1024;

// Logs one line for a large allocation request.
//
// These wrappers run inside the loader binary in host-libc context, possibly
// before Starboard is initialized and possibly under an allocator lock, so
// this path must be async-signal-simple: no allocation, no locks, no stdio —
// a fixed-size stack buffer formatted by hand and a single write(2) to
// stderr.
void LogLargeAlloc(const char* function, uint64_t size, const void* caller) {
  char buf[160];
  size_t pos = 0;
  const auto append = [&buf, &pos](const char* s) {
    while (*s != '\0' && pos < sizeof(buf) - 1) {
      buf[pos++] = *s++;
    }
  };
  const auto append_decimal = [&buf, &pos](uint64_t value) {
    char digits[20];
    size_t n = 0;
    do {
      digits[n++] = static_cast<char>('0' + value % 10);
      value /= 10;
    } while (value != 0 && n < sizeof(digits));
    while (n > 0 && pos < sizeof(buf) - 1) {
      buf[pos++] = digits[--n];
    }
  };
  const auto append_hex = [&buf, &pos](uint64_t value) {
    static const char kHex[] = "0123456789abcdef";
    char digits[16];
    size_t n = 0;
    do {
      digits[n++] = kHex[value & 0xf];
      value >>= 4;
    } while (value != 0 && n < sizeof(digits));
    while (n > 0 && pos < sizeof(buf) - 1) {
      buf[pos++] = digits[--n];
    }
  };

  append("[elf_loader] large alloc: ");
  append(function);
  append(" size=");
  append_decimal(size);
  append(" (0x");
  append_hex(size);
  append(") ra=0x");
  append_hex(static_cast<uint64_t>(reinterpret_cast<uintptr_t>(caller)));
  append("\n");
  const ssize_t rv = write(STDERR_FILENO, buf, pos);
  (void)rv;
}

}  // namespace

void* __abi_wrap_malloc(size_t size) {
#if STARBOARD_LOG_LARGE_ALLOCS
  if (size >= kLargeAllocLogThreshold) {
    LogLargeAlloc("malloc", size, __builtin_return_address(0));
  }
#endif
  return malloc(size);
}

void* __abi_wrap_calloc(size_t nmemb, size_t size) {
#if STARBOARD_LOG_LARGE_ALLOCS
  const uint64_t total = static_cast<uint64_t>(nmemb) * size;
  if (total >= kLargeAllocLogThreshold) {
    LogLargeAlloc("calloc", total, __builtin_return_address(0));
  }
#endif
  return calloc(nmemb, size);
}

void* __abi_wrap_realloc(void* ptr, size_t size) {
#if STARBOARD_LOG_LARGE_ALLOCS
  if (size >= kLargeAllocLogThreshold) {
    LogLargeAlloc("realloc", size, __builtin_return_address(0));
  }
#endif
  return realloc(ptr, size);
}

int __abi_wrap_posix_memalign(void** memptr, size_t alignment, size_t size) {
#if STARBOARD_LOG_LARGE_ALLOCS
  if (size >= kLargeAllocLogThreshold) {
    LogLargeAlloc("posix_memalign", size, __builtin_return_address(0));
  }
#endif
  return posix_memalign(memptr, alignment, size);
}

void* __abi_wrap_aligned_alloc(size_t alignment, size_t size) {
#if STARBOARD_LOG_LARGE_ALLOCS
  if (size >= kLargeAllocLogThreshold) {
    LogLargeAlloc("aligned_alloc", size, __builtin_return_address(0));
  }
#endif
  return aligned_alloc(alignment, size);
}

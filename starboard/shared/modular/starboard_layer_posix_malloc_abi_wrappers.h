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

#ifndef STARBOARD_SHARED_MODULAR_STARBOARD_LAYER_POSIX_MALLOC_ABI_WRAPPERS_H_
#define STARBOARD_SHARED_MODULAR_STARBOARD_LAYER_POSIX_MALLOC_ABI_WRAPPERS_H_

#include <stddef.h>

#include "starboard/export.h"

// Diagnostic allocation wrappers.
//
// Unlike the other files in this directory these wrappers perform no type
// translation (the allocation entry points are ABI-identical between musl and
// the platform libc). They exist purely as a diagnostic: any allocation
// request >= 16 MiB is logged together with the caller's return address so
// that large allocations that heapprofd cannot attribute (failed-unwind
// buckets) can still be identified by symbolizing the raw return address
// against the unstripped library.
//
// For Evergreen builds, exported_symbols.cc maps malloc/calloc/realloc/
// posix_memalign/aligned_alloc to these wrappers when
// STARBOARD_LOG_LARGE_ALLOCS is non-zero (default: on). Define it to 0 to
// compile the diagnostic out entirely (the raw libc symbols are then
// registered instead, restoring exactly the previous behavior).
//
// The log lines are additionally gated at runtime by the
// COBALT_MEM_EXP_LARGE_ALLOC_LOG experiment: even when compiled in, nothing
// is logged unless COBALT_MEM_EXP_LARGE_ALLOC_LOG=1 (or COBALT_MEM_EXP_ALL=1)
// is set in the environment. Forwarding to libc is unconditional either way.
#if !defined(STARBOARD_LOG_LARGE_ALLOCS)
#define STARBOARD_LOG_LARGE_ALLOCS 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

SB_EXPORT void* __abi_wrap_malloc(size_t size);
SB_EXPORT void* __abi_wrap_calloc(size_t nmemb, size_t size);
SB_EXPORT void* __abi_wrap_realloc(void* ptr, size_t size);
SB_EXPORT int __abi_wrap_posix_memalign(void** memptr,
                                        size_t alignment,
                                        size_t size);
SB_EXPORT void* __abi_wrap_aligned_alloc(size_t alignment, size_t size);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // STARBOARD_SHARED_MODULAR_STARBOARD_LAYER_POSIX_MALLOC_ABI_WRAPPERS_H_

// Copyright 2018 The Cobalt Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
#include "starboard/media.h"

#include <stdlib.h>

#include "starboard/common/log.h"

namespace {

// Runtime gate for the reduced-media-buffer-budget experiment. OFF (default)
// keeps the upstream values; set COBALT_MEM_EXP_MEDIA_BUDGETS=1 (or
// COBALT_MEM_EXP_ALL=1) to enable the reduced values.
bool MediaBudgetsExperimentEnabled() {
  static const bool enabled = [] {
    const char* v = getenv("COBALT_MEM_EXP_MEDIA_BUDGETS");
    if (!v) {
      v = getenv("COBALT_MEM_EXP_ALL");
    }
    return v && v[0] == '1';
  }();
  return enabled;
}

}  // namespace

int SbMediaGetInitialBufferCapacity() {
  if (MediaBudgetsExperimentEnabled()) {
    // The media buffer pool is allocated on demand and grows in units of
    // SbMediaGetBufferAllocationUnit() (1 MiB), so a large initial block only
    // reserves memory that may never be needed.  Keep the initial capacity at
    // a single allocation unit and let the pool grow to the actual buffered
    // size.
    return 1 * 1024 * 1024;
  }
  return 21 * 1024 * 1024;
}

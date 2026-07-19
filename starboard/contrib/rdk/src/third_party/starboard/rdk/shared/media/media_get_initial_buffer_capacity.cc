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

#include "starboard/common/log.h"
#include "starboard/shared/starboard/features.h"

namespace {

// Runtime gate for the reduced-media-buffer-budget experiment
// (CobaltMemMediaBudgets). OFF (the default, and always before Cobalt pushes
// feature state down through the Starboard features extension) keeps the
// upstream values. Deliberately evaluated on every call rather than cached,
// so callers that run before the FeatureList is initialized read OFF without
// latching it.
bool MediaBudgetsExperimentEnabled() {
  return starboard::features::FeatureList::IsFeatureListInitialized() &&
         starboard::features::FeatureList::IsEnabled(
             starboard::features::kCobaltMemMediaBudgets);
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

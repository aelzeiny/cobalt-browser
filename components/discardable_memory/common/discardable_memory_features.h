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

#ifndef COMPONENTS_DISCARDABLE_MEMORY_COMMON_DISCARDABLE_MEMORY_FEATURES_H_
#define COMPONENTS_DISCARDABLE_MEMORY_COMMON_DISCARDABLE_MEMORY_FEATURES_H_

#include "base/feature_list.h"
#include "components/discardable_memory/common/discardable_memory_export.h"

namespace discardable_memory {
namespace features {

// Cobalt (TV) memory experiment (see cobalt/COBALT_MEMORY_EXPERIMENTS.md):
// caps the default discardable-memory limit at 16 MB and enables synchronous
// release of freed-but-resident discardable memory on low-memory events.
// Declared here (rather than in cobalt/) because both the service-side
// manager in this component and cobalt/app consume it, and components/ cannot
// include cobalt/ headers.
DISCARDABLE_MEMORY_EXPORT BASE_DECLARE_FEATURE(kCobaltMemDiscardable);

}  // namespace features
}  // namespace discardable_memory

#endif  // COMPONENTS_DISCARDABLE_MEMORY_COMMON_DISCARDABLE_MEMORY_FEATURES_H_

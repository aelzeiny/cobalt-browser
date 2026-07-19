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

#ifndef COBALT_BROWSER_MEMORY_EXPERIMENTS_MEMORY_EXPERIMENT_FEATURES_H_
#define COBALT_BROWSER_MEMORY_EXPERIMENTS_MEMORY_EXPERIMENT_FEATURES_H_

#include "base/feature_list.h"

// Memory-experiment base::Features (see cobalt/COBALT_MEMORY_EXPERIMENTS.md).
//
// All experiments are DISABLED by default and are turned on at runtime either
// through the h5vcc experiments API (which registers field-trial overrides BY
// NAME in CobaltContentBrowserClient::SetUpCobaltFeaturesAndParams) or via
// --enable-features on the command line.
//
// Declaration layout: because h5vcc overrides are applied by feature NAME, a
// feature's declaration can live wherever its consumer is; the name string is
// the contract. The experiment features are therefore declared in three
// places:
//
//  1. This file: experiments whose consumers live in cobalt/ code (the
//     browser client and other cobalt/browser call sites).
//  2. starboard/extension/feature_config.h: experiments consumed below
//     Starboard ("CobaltMemMediaBudgets", "CobaltMemGlibcTuning",
//     "CobaltMemGstQueues", "CobaltMem1080pUi"). The feature_config.h macro
//     machinery declares the matching base::Features in cobalt::features
//     (cobalt/common/features/features.h) and
//     cobalt::features::InitializeStarboardFeatures() pushes their states
//     down to Starboard's FeatureList.
//  3. At component-layer consumer sites, since components/, net/, sql/,
//     storage/ and third_party/blink cannot include cobalt/ headers:
//       - "CobaltMemPaTuning":
//         third_party/blink/renderer/platform/wtf/allocator/partitions.cc
//       - "CobaltMemDiscardable":
//         components/discardable_memory/common/discardable_memory_features.h
//         (shared by the service-side manager and cobalt/app)
//       - "CobaltMemBlobLimits": storage/browser/blob/features.h
//       - "CobaltMemCacheSweepNet": net/base/features.h -- the net/ half of
//         "CobaltMemCacheSweep" under a distinct name (one BASE_FEATURE per
//         name in the binary); SetUpCobaltFeaturesAndParams fans the
//         config's "CobaltMemCacheSweep" out to it.
//       - "CobaltMemCacheSweepSql": sql/sql_features.h -- same pattern for
//         the sql/ half.
//     Those BASE_FEATURE declarations live with the consumer code and
//     intentionally do NOT appear here.
//
// Additionally, "CobaltMemAxAutodisable" and "CobaltMemParkableStrings" are
// config-level aliases only: SetUpCobaltFeaturesAndParams fans them out to
// the corresponding upstream features and base::FeatureList::IsEnabled() is
// never called on them, so they need no BASE_FEATURE declaration anywhere.

namespace cobalt {
namespace features {

// Bounds the HTTP cache and shrinks related network/sql caches.
BASE_DECLARE_FEATURE(kCobaltMemCacheSweep);

// Lowers the compositor GPU memory budget (--force-gpu-mem-available-mb=32).
BASE_DECLARE_FEATURE(kCobaltMemGpuBudget);

// Applies the decoded-image working set budget switch
// (--decoded-image-working-set-budget-bytes=25165824).
BASE_DECLARE_FEATURE(kCobaltMemImageCache);

// Purges memory caches after user quiescence (IdleMemoryPurger).
BASE_DECLARE_FEATURE(kCobaltMemIdlePurge);

// Merges platform --js-flags with the Cobalt TV defaults (defaults first, so
// the platform wins per-flag) and lowers --initial-old-space-size 64 -> 16.
BASE_DECLARE_FEATURE(kCobaltMemJsFlags);

// Strips desktop-only browser machinery: DevTools becomes opt-in, WebAuthn,
// Attribution Reporting, FLEDGE storage, Domain Reliability and First-Party
// Sets are disabled.
BASE_DECLARE_FEATURE(kCobaltMemStripDesktop);

// Reduces the default stack size of Chromium-created threads
// (ReduceAndroidThreadStackSize appended to --enable-features).
BASE_DECLARE_FEATURE(kCobaltMemThreadStacks);

}  // namespace features
}  // namespace cobalt

#endif  // COBALT_BROWSER_MEMORY_EXPERIMENTS_MEMORY_EXPERIMENT_FEATURES_H_

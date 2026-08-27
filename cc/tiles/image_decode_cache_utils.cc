// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TILES_IMAGE_DECODE_CACHE_UTILS_CC_
#define CC_TILES_IMAGE_DECODE_CACHE_UTILS_CC_

#include "cc/tiles/image_decode_cache_utils.h"

#include "build/build_config.h"

#if BUILDFLAG(IS_COBALT)
#include <optional>
#include <string>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"
#include "cc/base/features.h"
#include "cc/base/switches.h"
#endif

#include "base/check.h"
#include "cc/paint/paint_flags.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkPixmap.h"

#if !BUILDFLAG(IS_ANDROID)
#include "base/system/sys_info.h"
#endif

namespace cc {

// static
bool ImageDecodeCacheUtils::ShouldEvictCaches(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  switch (memory_pressure_level) {
    case base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE:
    case base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_MODERATE:
      return false;
    case base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL:
      return true;
  }
  NOTREACHED();
}

// static
size_t ImageDecodeCacheUtils::GetWorkingSetBytesForImageDecode(
    bool for_renderer) {
#if BUILDFLAG(IS_COBALT)
  static const size_t cobalt_decoded_image_working_set_budget_bytes = []() {
    size_t budget = 128 * 1024 * 1024;
    auto* command_line = base::CommandLine::ForCurrentProcess();
    if (command_line->HasSwitch(switches::kDecodedImageWorkingSetBudgetBytes)) {
      std::string value = command_line->GetSwitchValueASCII(
          switches::kDecodedImageWorkingSetBudgetBytes);
      int64_t parsed_value;
      if (base::StringToInt64(value, &parsed_value) && parsed_value >= 0) {
        budget = parsed_value;
      }
    }
    return budget;
  }();
  return cobalt_decoded_image_working_set_budget_bytes;
#else
  size_t decoded_image_working_set_budget_bytes = 128 * 1024 * 1024;
#if !BUILDFLAG(IS_ANDROID)
  if (for_renderer) {
    const bool using_low_memory_policy = base::SysInfo::IsLowEndDevice();
    // If there's over 4GB of RAM, increase the working set size to 256MB for
    // both gpu and software.
    const int kImageDecodeMemoryThresholdMB = 4 * 1024;
    if (using_low_memory_policy) {
      decoded_image_working_set_budget_bytes = 32 * 1024 * 1024;
    } else if (base::SysInfo::AmountOfPhysicalMemoryMB() >=
               kImageDecodeMemoryThresholdMB) {
      decoded_image_working_set_budget_bytes = 256 * 1024 * 1024;
    }
  }
#endif  // !BUILDFLAG(IS_ANDROID)
  return decoded_image_working_set_budget_bytes;
#endif
}

#if BUILDFLAG(IS_COBALT)
namespace {

// Reads one integer param of kCobaltImageDecodeCacheLimit. Tries the bare param
// name first (registered by --enable-features=CobaltImageDecodeCacheLimit:mb/4)
// and then the "Feature:param" key that h5vcc.experiments.setExperimentState()
// writes into the shared CobaltExperiment trial. Returns nullopt when the
// feature is off, the param is unset, or the value does not parse -- callers
// then fall back to their command-line switch.
std::optional<int> GetImageDecodeCacheLimitParam(
    const base::FeatureParam<int>& param,
    const char* joined_param_name) {
  // These accessors can run before the FeatureList exists; the command-line
  // fallback is safe there, querying a feature is not.
  if (!base::FeatureList::GetInstance() ||
      !base::FeatureList::IsEnabled(features::kCobaltImageDecodeCacheLimit)) {
    return std::nullopt;
  }
  const int bare_value = param.Get();
  if (bare_value >= 0) {
    return bare_value;
  }
  const std::string joined_value = base::GetFieldTrialParamValueByFeature(
      features::kCobaltImageDecodeCacheLimit, joined_param_name);
  int parsed_value = 0;
  if (!joined_value.empty() &&
      base::StringToInt(joined_value, &parsed_value) && parsed_value >= 0) {
    return parsed_value;
  }
  return std::nullopt;
}

}  // namespace

// static
size_t ImageDecodeCacheUtils::GetPersistentCacheBudgetCount() {
  static const size_t cobalt_decoded_image_persistent_cache_budget_count = []() {
    if (std::optional<int> items = GetImageDecodeCacheLimitParam(
            features::kCobaltImageDecodeCacheLimitItems,
            "CobaltImageDecodeCacheLimit:items")) {
      LOG(INFO) << "[cobalt-exp] CobaltImageDecodeCacheLimit=ON: persistent "
                   "decoded-image cache item budget="
                << *items << " (from the experiment param 'items')";
      return static_cast<size_t>(*items);
    }
    size_t budget = 2000; // kNormalMaxItemsInCacheForGpu default
    auto* command_line = base::CommandLine::ForCurrentProcess();
    if (command_line->HasSwitch(switches::kCCImageCacheLimitItems)) {
      std::string value = command_line->GetSwitchValueASCII(
          switches::kCCImageCacheLimitItems);
      int parsed_value;
      if (base::StringToInt(value, &parsed_value) && parsed_value >= 0) {
        budget = static_cast<size_t>(parsed_value);
      }
    }
    LOG(INFO) << "[cobalt-exp] CobaltImageDecodeCacheLimit items param unset "
                 "(feature OFF or no 'items'): persistent decoded-image cache "
                 "item budget="
              << budget << " (command line/default)";
    return budget;
  }();
  return cobalt_decoded_image_persistent_cache_budget_count;
}

// static
size_t ImageDecodeCacheUtils::GetPersistentCacheBudgetBytes() {
  static const size_t cobalt_decoded_image_persistent_cache_budget_bytes = []() {
    if (std::optional<int> mb = GetImageDecodeCacheLimitParam(
            features::kCobaltImageDecodeCacheLimitMb,
            "CobaltImageDecodeCacheLimit:mb")) {
      LOG(INFO) << "[cobalt-exp] CobaltImageDecodeCacheLimit=ON: persistent "
                   "decoded-image cache byte budget="
                << *mb << " MB (from the experiment param 'mb')";
      return static_cast<size_t>(*mb) * 1024 * 1024;
    }
    size_t budget = std::numeric_limits<size_t>::max();
    auto* command_line = base::CommandLine::ForCurrentProcess();
    if (command_line->HasSwitch(switches::kCCImageCacheLimitMbs)) {
      std::string value = command_line->GetSwitchValueASCII(
          switches::kCCImageCacheLimitMbs);
      int parsed_value;
      if (base::StringToInt(value, &parsed_value) && parsed_value >= 0) {
        budget = static_cast<size_t>(parsed_value) * 1024 * 1024;
      }
    }
    LOG(INFO) << "[cobalt-exp] CobaltImageDecodeCacheLimit mb param unset "
                 "(feature OFF or no 'mb'): persistent decoded-image cache "
                 "byte budget="
              << (budget == std::numeric_limits<size_t>::max()
                      ? std::string("unlimited")
                      : base::NumberToString(budget / (1024 * 1024)) + " MB")
              << " (command line/default)";
    return budget;
  }();
  return cobalt_decoded_image_persistent_cache_budget_bytes;
}
#endif

}  // namespace cc

#endif  // CC_TILES_IMAGE_DECODE_CACHE_UTILS_CC_

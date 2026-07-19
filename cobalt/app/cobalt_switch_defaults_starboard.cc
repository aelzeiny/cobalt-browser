// Copyright 2025 The Cobalt Authors. All Rights Reserved.
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

#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#include "base/base_switches.h"
#include "build/buildflag.h"
#include "cc/base/switches.h"
#include "cobalt/app/cobalt_switch_defaults.h"
#include "cobalt/browser/switches.h"
#include "cobalt/shell/common/shell_switches.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/common/content_switches.h"
#include "gpu/command_buffer/service/gpu_switches.h"
#include "gpu/config/gpu_switches.h"
#include "media/base/media_switches.h"
#include "sandbox/policy/switches.h"
#include "third_party/blink/public/common/switches.h"
#include "ui/base/ui_base_switches.h"
#include "ui/gl/gl_switches.h"

#if BUILDFLAG(IS_OZONE)
#endif

namespace {

// Returns true when the named memory experiment is enabled via environment
// variable. Each experiment can be enabled individually (e.g.
// COBALT_MEM_EXP_GPU_BUDGET=1), or all experiments at once via
// COBALT_MEM_EXP_ALL=1. With no experiment variables set, behavior matches
// upstream defaults exactly.
bool MemExpEnabled(const char* name) {
  const char* v = getenv(name);
  if (!v) {
    v = getenv("COBALT_MEM_EXP_ALL");
  }
  return v && v[0] == '1';
}

}  // namespace

namespace cobalt {

const std::vector<const char*>&
CommandLinePreprocessor::GetCobaltToggleSwitches() {
  // ==========
  // IMPORTANT:
  //
  // These command line switches defaults do not affect non-POSIX platforms.
  // They only affect platforms such as Linux and AOSP. If you are making
  // changes to these values, please check that other platforms (such as
  // AndroidTV) are getting corresponding updates.

  // List of toggleable default switches.
  static const std::vector<const char*> kCobaltToggleSwitches{
      // Enable Blink to work in overlay video mode
      ::switches::kForceVideoOverlays,
      // Disable multiprocess mode.
      ::switches::kSingleProcess,
      // Hide content shell toolbar.
      ::switches::kContentShellHideToolbar,
      // Accelerated GL is blanket disabled for Linux. Ignore the GPU
      // blocklist to enable it.
      ::switches::kIgnoreGpuBlocklist,
      // Disable Zygote (a process fork utility); in turn needs sandbox
      // disabled.
      ::switches::kNoZygote,
      sandbox::policy::switches::kNoSandbox,
      // Rasterize Tiles directly to GPU memory
      // (ZeroCopyRasterBufferProvider).
      blink::switches::kEnableZeroCopy,
      // Enable low-end device mode. This comes with a load of memory and CPU
      // saving goodies but can degrade the experience considerably. One of
      // the known regressions is 4444 textures, which are then disabled
      // explicitly.
      ::switches::kEnableLowEndDeviceMode,
      blink::switches::kDisableRGBA4444Textures,
      // For Starboard the signal handlers are already setup. Disable the
      // Chromium registrations to avoid overriding the Starboard ones.
      ::switches::kDisableInProcessStackTraces,
      // Cobalt doesn't use Chrome's accelerated video decoding/encoding.
      ::switches::kDisableAcceleratedVideoDecode,
      ::switches::kDisableAcceleratedVideoEncode,
      // Force to use dark mode.
      ::switches::kForceDarkMode,
      // Hide scrollbars to avoid memory allocation.
      ::switches::kHideScrollbars,
  };
  return kCobaltToggleSwitches;
}

const base::CommandLine::SwitchMap&
CommandLinePreprocessor::GetCobaltParamSwitchDefaults() {
  // Memory-experiment behaviors below are opt-in via environment variables
  // (see MemExpEnabled). With no experiment enabled, the assembled map is
  // byte-identical to the upstream defaults.
  const bool js_flags_exp = MemExpEnabled("COBALT_MEM_EXP_JS_FLAGS");
  const bool strip_desktop_exp = MemExpEnabled("COBALT_MEM_EXP_STRIP_DESKTOP");
  const bool image_cache_exp = MemExpEnabled("COBALT_MEM_EXP_IMAGE_CACHE");
  const bool ax_autodisable_exp =
      MemExpEnabled("COBALT_MEM_EXP_AX_AUTODISABLE");
  const bool gpu_budget_exp = MemExpEnabled("COBALT_MEM_EXP_GPU_BUDGET");
  const bool parkable_strings_exp =
      MemExpEnabled("COBALT_MEM_EXP_PARKABLE_STRINGS");
  const bool thread_stacks_exp = MemExpEnabled("COBALT_MEM_EXP_THREAD_STACKS");

  // The environment is stable for the lifetime of the process in production,
  // where the preprocessor is constructed exactly once at startup. Tests,
  // however, toggle the experiment variables between constructions, so the
  // assembled map is cached per experiment-state instead of in a single
  // function-local static. std::map references are stable, so the returned
  // reference stays valid for the process lifetime.
  std::string exp_state;
  for (bool exp_enabled :
       {js_flags_exp, strip_desktop_exp, image_cache_exp, ax_autodisable_exp,
        gpu_budget_exp, parkable_strings_exp, thread_stacks_exp}) {
    exp_state += exp_enabled ? '1' : '0';
  }

  static std::map<std::string, base::CommandLine::SwitchMap>
      switch_defaults_cache;
  auto cached = switch_defaults_cache.find(exp_state);
  if (cached != switch_defaults_cache.end()) {
    return cached->second;
  }

  // Assemble the defaults for the current experiment state with an
  // immediately-invoked lambda so entries and feature-list strings can be
  // built conditionally.
  base::CommandLine::SwitchMap cobalt_switch_defaults = [&] {
    base::CommandLine::SwitchMap defaults;

    // Disable Vulkan.
    std::string disable_features = "Vulkan,MemoryCacheStrongReference";
    if (strip_desktop_exp) {
      // Also disable desktop browser features that are inert or unwanted on
      // TV but still allocate (sqlite storage, service heaps, timers):
      // * ConversionMeasurement: Attribution Reporting API infrastructure
      //   (content/browser/attribution_reporting; gates AttributionManager
      //   creation in StoragePartitionImpl).
      // * InterestGroupStorage: FLEDGE/Protected Audience interest-group
      //   storage and AdAuction services; disabling it also forces the
      //   AdInterestGroupAPI and Fledge runtime features off
      //   (content/child/runtime_features.cc).
      disable_features += ",ConversionMeasurement,InterestGroupStorage";
    }
    if (parkable_strings_exp) {
      // LessAggressiveParkableString: suspends ParkableString parking while
      // the renderer is foreground, and a TV app is permanently foreground,
      // so large strings (e.g. JS source) would never compress.
      disable_features += ",LessAggressiveParkableString";
    }
    defaults[::switches::kDisableFeatures] = disable_features;

    std::string enable_features;
    if (!image_cache_exp) {
      // Upstream default. This feature token is not defined anywhere and is
      // silently ignored; the COBALT_MEM_EXP_IMAGE_CACHE experiment replaces
      // it with the kDecodedImageWorkingSetBudgetBytes switch below, which
      // cc::ImageDecodeCacheUtils actually consumes.
      enable_features += "LimitImageDecodeCacheSize:mb/24, ";
    }
    // When DefaultEnableANGLEValidation is disabled (e.g gold/qa), EGL
    // attribute EGL_CONTEXT_OPENGL_NO_ERROR_KHR is set during egl context
    // creation, but egl extension required to support the attribute is
    // missing and causes errors. So Enable it by default. (More context in
    // b/444042898)
    enable_features +=
        "DefaultEnableANGLEValidation, "
        "SmallerInterestArea, "
        "ReclaimPrepaintTilesWhenIdle, "
        "ReclaimOldPrepaintTiles";
    if (ax_autodisable_exp) {
      // Tear down accessibility trees when no assistive technology consumes
      // accessibility events (3 user input events over 30+ seconds with no
      // accessibility API usage). On TV no screen reader runs, but e.g. an
      // attached DevTools/CDP session can flip on an accessibility mode and
      // keep the full AX tree alive and churning. See
      // content/browser/accessibility/browser_accessibility_state_impl.cc.
      enable_features += ", AutoDisableAccessibility";
    }
    if (thread_stacks_exp) {
      // Cap the default stack size of Chromium-created threads at 256 KiB
      // instead of the 8 MiB glibc default. Threads that request an explicit
      // stack size are unaffected. Note that
      // base/threading/platform_thread_linux_base.cc detects this feature by
      // scanning the --enable-features switch string, so it must be enabled
      // here (on the command line) rather than by flipping the declared
      // feature default.
      enable_features += ", ReduceAndroidThreadStackSize";
    }
    defaults[::switches::kEnableFeatures] = enable_features;

  // Force some ozone settings.
#if BUILDFLAG(IS_OZONE)
    defaults[::switches::kUseGL] = "angle";
    defaults[::switches::kUseANGLE] = "gles-egl";
#endif

    // Use passthrough command decoder.
    defaults[::switches::kUseCmdDecoder] = "passthrough";
    if (image_cache_exp) {
      // Limit the decoded-image working set to 24MB. This replaces the
      // inert "LimitImageDecodeCacheSize:mb/24" feature token above.
      defaults[::switches::kDecodedImageWorkingSetBudgetBytes] = "25165824";
    }
    // Set the default size for the content shell/starboard window.
    defaults[::switches::kContentShellHostWindowSize] = "1920x1080";
    if (!strip_desktop_exp) {
      // Enable remote Devtools access.
      defaults[::switches::kRemoteDebuggingPort] = "9222";
      defaults[::switches::kRemoteAllowOrigins] = "http://localhost:9222";
    }
    // else: remote DevTools access is opt-in. Pass
    // --remote-debugging-port=9222 (and, if needed,
    // --remote-allow-origins=http://localhost:9222) explicitly to enable
    // it; without the switch the DevTools HTTP server is not started.

    // kEnableLowEndDeviceMode sets MSAA to 4 (and not 8, the default). But
    // we set it explicitly just in case.
    defaults[blink::switches::kGpuRasterizationMSAASampleCount] = "4";
    // Enable precise memory info so we can make accurate client-side
    // measurements.
    defaults[::switches::kEnableBlinkFeatures] = "PreciseMemoryInfo";
    if (strip_desktop_exp) {
      // TV devices have no FIDO transports; disabling the WebAuthn API
      // (blink runtime feature "WebAuth") prevents page-triggered
      // device/fido authenticator discovery churn in the browser process.
      defaults[::switches::kDisableBlinkFeatures] = "WebAuth";
    }
    // Enable autoplay video/audio, as Cobalt may launch directly into media
    // playback before user interaction.
    defaults[::switches::kAutoplayPolicy] = "no-user-gesture-required";

    std::string js_flags =
        // Disable decommitting pooled pages to prevent virtual memory
        // fragmentation.
        "--no-decommit-pooled-pages "
        // Enable memory saving mode with little v8 performance tradeoff.
        "--optimize-for-size ";
    if (js_flags_exp) {
      // Set initial old space size to 16MB and max old space size to 512MB.
      // A TV app's live JS heap is typically 20-40MB; a small initial old
      // space triggers the first major GC earlier and lowers the plateau.
      js_flags += "--initial-old-space-size=16 ";
    } else {
      // Set initial old space size to 64MB and max old space size to 512MB.
      js_flags += "--initial-old-space-size=64 ";
    }
    js_flags +=
        "--max-old-space-size=512 "
        // Disable v8 optimizing compilers (turbofan, maglev, sparkplug).
        "--disable-optimizing-compilers "
        "--no-sparkplug "
        // Disable v8 concurrent marking by default.
        "--no-concurrent-marking";
    defaults[blink::switches::kJavaScriptFlags] = js_flags;

    // Limit GPU memory available to 32MB (experiment) or 64MB (default).
    defaults[::switches::kForceGpuMemAvailableMb] =
        gpu_budget_exp ? "32" : "64";
    // Disable CC image cache items limit.
    defaults[::switches::kCCImageCacheLimitItems] = "0";

    return defaults;
  }();

  return switch_defaults_cache
      .emplace(exp_state, std::move(cobalt_switch_defaults))
      .first->second;
}

}  // namespace cobalt

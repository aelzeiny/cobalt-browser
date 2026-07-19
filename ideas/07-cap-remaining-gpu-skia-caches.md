# Cap the remaining GPU/Skia caches (program cache, SkResourceCache, transfer buffers)

> **VETTED — downgraded.** All three levers are **Linux/Starboard-only**:
> AndroidTV's program cache is already 2 MB, `--skia-resource-cache-limit-mb`
> is compiled out on Android (`content/common/skia_utils.cc:43` is
> `#if !BUILDFLAG(IS_ANDROID)`), and the transfer-buffer backpressure
> customization is `IS_STARBOARD` (false on AndroidTV). Estimates are
> **ceiling-mostly**: realistic steady-state combined saving is likely
> **<10 MB, Linux only**. Keep as cheap hygiene, not a headline item.

**Estimated savings (corrected): ~4–12 MB ceiling on Linux, realistic
steady-state likely a few MB; ~0 on AndroidTV**
**Effort: trivial (two switches) + one one-line predicate change**

## Corrected findings

1. **GPU program caches — Linux only.**
   `kDefaultMaxProgramCacheMemoryBytes` is 6 MB non-Android / 2 MB Android
   (`gpu/config/gpu_preferences.h:24,26`). The original claim of "doubling via
   ANGLEPerContextBlobCache" was wrong — that feature is disabled by default
   (`gpu/config/gpu_finch_features.cc:857-859`). The real story: the single
   6 MB `gpu_program_cache_size` feeds **two separate caches** on Linux — the
   passthrough-decoder program cache (`gpu_channel_manager.cc:439-447`; Cobalt
   uses passthrough) and Skia's `GrShaderCache` (`gpu_channel_manager.cc:385,391`,
   present under GPU tile rasterization) — so the combined ceiling is ~12 MB.
   `--gpu-program-cache-size-kb=2048` (switch confirmed:
   `gpu/command_buffer/service/gpu_switches.cc:52`, read at
   `service_utils.cc:249-251`) shrinks both → ~4 MB ceiling. A fixed UI's
   shader population probably doesn't fill 12 MB, so realistic saving is a few
   MB. **No-op on AndroidTV** (already 2 MB).

2. **SkResourceCache — Linux only, ceiling only.**
   Default 32 MB (`third_party/skia/src/core/SkResourceCache.cpp:49-50`);
   only capped when `--skia-resource-cache-limit-mb` is passed
   (`content/common/skia_utils.cc:53-60`), which Cobalt doesn't; low-end mode
   does NOT shrink it. But the whole block is `#if !BUILDFLAG(IS_ANDROID)` —
   **the switch is ignored on AndroidTV**. And under GPU raster + zero-copy
   this CPU-side cache (software scaling, mipmaps, blur masks) is largely
   bypassed, so occupancy is probably small. Setting `=4` on Linux is cheap
   ceiling insurance; expect a few MB at best.

3. **Transfer/mapped memory — peak-only, needs the code change.**
   Confirmed: the small-limits branch is gated on
   `AmountOfPhysicalMemoryMB() <= 512`
   (`gpu/command_buffer/client/shared_memory_limits.h:30`), never true on
   shipping TVs; renderer main context uses the default (16 MB transfer,
   `physmem/20` mapped-upload allowance, `mapped_memory_reclaim_limit = kNoLimit`)
   via `content/renderer/render_thread_impl.cc:1224`. OR-ing the guard with
   `base::SysInfo::IsLowEndDevice()` would take effect. But allocation is
   lazy and bulk data mostly flows through the transfer cache under zero-copy,
   so this trims **peaks**, not steady state. The Starboard-only 24 MB cleanup
   threshold (`shared_memory_limits.h:48-53`) already backpressures Linux;
   AndroidTV has neither (IS_STARBOARD false there,
   `cobalt/build/configs/starboard.gni:16`).

## Proposal (Linux `cobalt_switch_defaults_starboard.cc`; skip AndroidTV)

1. `{switches::kGpuProgramCacheSizeKb, "2048"}`
2. `{switches::kSkiaResourceCacheLimitMb, "4"}`
3. Optional: extend the `shared_memory_limits.h:30` guard with
   `IsLowEndDevice()` for peak-trimming on both platforms (this one does
   affect AndroidTV).

## Validation

- memory-infra `gpu` category on Linux before/after; confirm shader-cache and
  SkResourceCache occupancy (expected small — if so, log the result and move
  on; this idea is deliberately low-priority).

# Memory reduction ideas (target: −50 to −100 MB of ~500 MB RSS)

Specs for large (>5 MB each), low-refactor memory savings for Cobalt running
youtube.com/tv on TV-class devices.

**Start here:** [00-memory-profiling-playbook.md](00-memory-profiling-playbook.md)
— how to capture an attribution baseline (smaps → memory-infra → heap
profiler) using the tooling already in this tree, so each idea below can be
confirmed or killed with data before implementation.
Companion: [00b-chromium-memory-tooling.md](00b-chromium-memory-tooling.md) —
the upstream Chromium memory team's tooling (memory-infra dump-provider graph,
memlog/heapprofd heap profilers, TBM memory metrics) and how to get a
per-feature breakdown from it.
External context: [00c-external-research-findings.md](00c-external-research-findings.md)
— verified findings from Chrome/V8/Igalia/cast_shell sources (July 2026):
the 80 MB demuxer-limit rebuffering floor (affects idea 01), the
Android-only ifdef gap in low-end mode (reinforces idea 08), binary-code
residency as a possible ~100–160 MB RSS line item, and what's already
default in this tree.

## Status: all specs adversarially vetted; top 3 implemented (July 2026)

Ideas 01, 02, and 06 are implemented, one per `opt/*` branch:
`opt/mse-video-buffer-budget` (4K budgets → 80 MB, the QoE floor from
crbug 40264947 and exactly the nplb/2024-HW-requirements HDR minimum),
`opt/media-pool-release-on-hide` (idle release default-on via the
`CobaltReleaseIdleMediaBufferMemory` feature, h5vcc opt-in preserved), and
`opt/image-decode-cache-limit` (dead `LimitImageDecodeCacheSize` token
replaced with `--decoded-image-working-set-budget-bytes=25165824`; hardcoded
IS_COBALT default lowered 128→24 MB).

## Vetting: all specs adversarially vetted against the codebase

Every spec below was verified claim-by-claim (build-flag applicability per
target, control-path reachability, estimate realism). Each file carries its
verdict at the top. Post-vetting ranking:

| # | Idea | Vetted estimate | Platform | Effort |
|---|------|-----------------|----------|--------|
| [01](01-clamp-mse-video-buffer-budget.md) | Clamp MSE video SourceBuffer budget (100/160 MB at 4K) | ceiling −20…−80 MB at 4K with the QoE-safe 80 MB clamp (Chrome field data: going lower caused rebuffers, see 00c); steady-state TBD (occupancy trends to limit) | Both | Trivial code; **knob choice matters** — runtime clamp is web-app-JS-only; Cobalt-settable switch disables per-resolution scaling; constant change is the durable option |
| [02](02-media-buffer-pool-idle-release.md) | Enable media pool release on hide/suspend | 30–160 MB when hidden after playback — **if the app doesn't already enable it** (check first!) | Mostly AndroidTV (Linux already on-demand) | Small — call sites exist; make the opt-in default-on |
| [06](06-fix-inert-image-decode-cache-limit.md) | **Confirmed bug:** `LimitImageDecodeCacheSize:mb/24` is inert; effective budget 128 MB | Linux: real, measure occupancy (10–80 MB range); AndroidTV: small (already bounded by 1 MB service cache + items=0) | Linux mainly | Trivial bug fix |
| [04](04-purge-memory-on-conceal-freeze.md) | Deterministic purge on conceal | Net-new mainly on **AndroidTV** (no lifecycle purge there); Linux already purges 1–4 min after backgrounding — win is immediacy | AndroidTV ≫ Linux | Small (~5 lines) |
| [03](03-disable-back-forward-cache-and-prerender.md) | Disable BFCache (6 live docs) + Prerender2 | **~0 expected** (SPA never navigates cross-document) — pure insurance against an uncapped 6×(10–40 MB) reservoir | Both | Trivial |
| [05](05-shrink-font-cache-and-package.md) | Font chunk cache + `limited` package | 12–20 MB but **only CJK sessions on Linux/Starboard**; ~0 AndroidTV (system fonts) | Linux only | Trivial; free option: wire orphaned `PurgeCaches()` |
| [07](07-cap-remaining-gpu-skia-caches.md) | Program cache / SkResourceCache / transfer buffer caps | ceiling ~4–12 MB Linux only; realistic <10 MB; several knobs no-op on Android | Linux only | Trivial; low priority |
| [08](08-os-memory-pressure-evaluator-linux.md) | OS memory-pressure evaluator (PSI/meminfo) — Linux has a pressure monitor with **zero pressure sources** today | No direct RSS cut; makes all purge machinery fire before OOM kill | Linux/RDK (Android covered by onTrimMemory) | Small–medium (~150 lines, prior art: Endless OS / ChromeOS) |

### Key cross-cutting vetting lessons

1. **AndroidTV ≠ Linux.** Many Chromium knobs silently diverge:
   `IS_ANDROID` low-end paths already cap several caches on AndroidTV
   (1 MB service transfer cache, 2 MB program cache), while
   `--skia-resource-cache-limit-mb` is compiled out on Android and
   `MemoryPurgeManager` is compiled out **only** on Android. Every future
   idea must be vetted per-target.
2. **The h5vcc runtime knobs are web-app-only.** `H5vccSettings.set()` is a
   `[Exposed=Window]` JS API (an explicit stop-gap "until Finch") — the
   YouTube app, not Cobalt, controls `Media.VideoBufferSizeClampMb`,
   `DecoderBuffer.ReleaseMemoryOnBackground`, etc. Before claiming savings
   from those paths, check what the app already sets in production.
3. **Measure occupancy before crediting ceiling cuts.** Ideas 01/06/07 lower
   caps; realized savings = current occupancy − new cap, which only the
   profiling playbook (00) can establish.

### Realistic stacked outlook after vetting

- **AndroidTV** (the fleet that matters): idea 02 (media pool on hide) +
  idea 04 (deterministic conceal purge) are the genuine levers —
  **30–160 MB off concealed RSS**, plus idea 01's ceiling cut during 4K
  playback. Foreground browse-UI savings need the profiling baseline first.
- **Linux/Starboard:** idea 06 (image cache bug) is the big foreground
  candidate pending an occupancy measurement; 01 during playback; 05 for CJK.
- The original "40–80 MB foreground browse" claim is **not yet supported** —
  it hinges on measured image-cache and media-buffer occupancy. Run playbook
  00 scenarios 1–2 and 4–5 before committing to a number.

## Audited and found already optimal — no spec needed

- **V8/Oilpan build config**: pointer compression ON (64-bit targets,
  `v8/gni/v8.gni:280`, `v8/BUILD.gn:1113-1128`), semi-space forced to 1 MB by
  `--optimize-for-size` (`v8/src/flags/flag-definitions.h:1674`), memory
  reducer ON, snapshot embedded/shared, optimizing compilers already disabled.
  Remaining knobs (disable wasm, `--jitless`) are ~2–6 MB each — not worth it
  for RSS alone. Updated risk picture (see 00c): V8's own measurement of
  jitless on a simulated **YouTube Living Room session was only ~6% JS
  slowdown** (vs 40% Speedometer), and we already run without optimizers or
  Sparkplug, so the marginal cost is lower still — a viable future option if
  JIT-page elimination or W^X hardening ever matters, just not a big fish.
  **Watch item:** verify shipping targets are 64-bit; on 32-bit ARM pointer
  compression is structurally unavailable. And ensure no partner `args.gn`
  sets `v8_enable_pointer_compression=false` — that would be a huge regression.
- **GPU/compositor big-ticket items**: tile memory correctly capped at 64 MB
  with required-only priority cutoff; GrContext cache already 2 MB
  (Cobalt low-end, `gpu/config/skia_limits.cc:96-98`); framebuffers are 1080p
  RGBA8 via the EGL driver swapchain (no 4K / triple-buffer / F16 bloat);
  raster staging pool bypassed by zero-copy.
- **LevelDB block caches** (LocalStorage/IndexedDB/SW): low-end-device mode
  already shrinks them to ~1 MB total (`third_party/leveldatabase/leveldb_chrome.cc:42-68`).
- **DOM storage database**: write buffers already hand-tuned to 64 KB
  (`components/services/storage/dom_storage/dom_storage_database.cc:55-65`).
- **V8 code cache (GeneratedCodeCache)**: on disk, 3 MB cap
  (`cobalt/browser/cobalt_content_browser_client.cc:280-295`). Fine as-is.

Also audited as already-optimal: **PartitionAlloc-as-malloc** is on,
**BackupRefPtr already compiled out** (`cobalt/build/configs/common.gn:98-99`),
thread caches stay at the small 512 B threshold in single-process mode, small
empty-slot-span ring on Linux, periodic MemoryReclaimer running,
**discardable memory cap already /8 (≈64 MB) via low-end-device mode**.

## Smaller fish noted in passing (<5–10 MB, not specced)

- PartitionAlloc thread-cache multiplier is 2.0 with no low-end reduction on
  Starboard/Linux (the Android/CrOS halving path doesn't apply): ~3–8 MB via
  `ThreadCacheRegistry::SetThreadCacheMultiplier(1.0)` or the
  `EnableConfigurableThreadCacheMultiplier` feature param.
- `MALLOC_ARENA_MAX=2` in the launch env for residual glibc allocations: ~1–3 MB.
- Enable `ReduceAndroidThreadStackSize` (Starboard feature,
  `starboard/extension/feature_config.h:173-175`) to cap every thread stack at
  256 KB — ~2–8 MB RSS across the ~20–40 threads of single-process mode.
- SQLite stores (cookies, trust tokens) each carry SQLite's default ~2 MB page
  cache because `sql::DatabaseOptions.cache_size` is never set
  (`sql/database.cc:2180-2189`): ~4–8 MB total across stores.
- `http_cache_max_size` left at the 80 MB default (disk, small resident index)
  — cheap to cap in `cobalt/browser/cobalt_content_browser_client.cc:349-410`.
- Blink MemoryCache strong-reference cache (15 MB) already disabled on target;
  raster threads already hardcoded to 1
  (`cc/raster/categorized_worker_pool.cc:426`); accessibility tree not built
  by default — confirmed non-issues.

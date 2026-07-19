# External research: Chromium memory reduction on low-end devices

Digest of a deep-research pass (July 2026) over Chrome memory-team / V8 /
embedded-Chromium sources. Every claim below survived adversarial
verification (3-0 verifier votes) including cross-checks against **this**
tree; local file:line references were confirmed here. Findings are grouped by
what they mean for us: things that change our specs, new opportunities, and
things that are already banked.

## 1. Directly changes our specs

### Field-trial warning for spec 01 (MSE buffer clamp) — do not clamp too low

Chrome tried applying **low-end demuxer memory limits to mid-range Android
devices** and reverted it from stable (Dec 2023) because UMA showed **more
rebuffers and less watch time**. Media OWNER (dalecurtis@, 2023-12-15):
"stable channel had more rebuffers and less watch time with this feature
enabled… Forcing small buffers on mid range devices is not appropriate."
The replacement was a **"medium" demuxer limit of 80 MB video / 5 MB audio**
(vs 150/12 default, 30/2 low-end), explicitly judged **"sufficient for 4K60
playback"** (CL 5142588). Present in our tree at
`media/base/demuxer_memory_limit_android.cc`.

- Source: crbug 40264947 (issues.chromium.org/issues/40264947), Gerrit CLs
  5126694 (revert), 5142588 (medium limit).
- **Implication for spec 01:** Chrome's fleet data says ~80 MB video buffer is
  the floor for a 4K-capable video product. Our proposed 4K clamps of
  50–60 MB SDR are **below** that floor — treat 80 MB as the starting clamp
  for any 4K tier and let QoE data justify going lower, not the reverse.
  (Note the caveat: Chrome's demuxer limit and our Starboard
  `SbMediaGetVideoBufferBudget` are analogous but not identical plumbing; the
  lesson transfers, the exact number may not.)

### --jitless numbers for the V8 audit (README "already optimal" note)

V8's own measurements for `--jitless` (v8.dev/blog/jitless): Speedometer 2.0
~40% slower, Web Tooling ~80% slower, **but a simulated YouTube Living Room
app session showed only a ~6% JS-execution slowdown** — i.e. the scary
benchmark numbers do not represent our workload. However the direct V8-heap
saving is only **~1.7% median**; the real wins are the elimination of JIT
code pages and W^X double mappings. Since we already ship
`--disable-optimizing-compilers` and `--no-sparkplug`, the marginal CPU cost
of full jitless is likely below 6%. Verdict unchanged (small fish, keep
disabled), but the risk assessment improves: if we ever need the last few MB
plus security hardening, jitless is cheaper on our workload than the
benchmarks suggest. Open question: end-to-end cost on real TV hardware
(startup, regexp interpretation) — the 6% figure is JS execution only.

## 2. New, verified opportunities

### The Android/ChromeOS ifdef gap — Linux builds get only a subset of low-end mode (reinforces spec 08)

From Igalia's BlinkOn 19 talk (José Dapena Paz, Oct 2024, "Optimizing
Chromium for low memory Linux based systems" — LGE webOS / Bloomberg
downstream maintainer) and confirmed line-by-line in our tree:
`--enable-low-end-device-mode` flips `IsLowEndDevice()` on all platforms
(`base/system/sys_info.cc:70-77`), but many **consumers** are compiled out
off Android/ChromeOS:

- The whole partial-low-end machinery (`Is3GbDevice`, `Is4GbOr6GbDevice`,
  `kPartialLowEndModeOn*`) is inside
  `#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_CHROMEOS)`
  (`base/system/sys_info.cc:79-186`, `base/features.cc:76-100`).
- The 96 MB (vs 256 MB) visible tile-memory clamp is `IS_ANDROID`-only
  (`third_party/blink/renderer/platform/widget/compositing/layer_tree_settings.cc:183-189`).
  (We separately cap tiles at 64 MB via settings — verify which path wins.)
- Blink's `IsCurrentlyLowMemory()` hard-returns `false` off Android
  (`memory_pressure_listener.cc:64-70`), disabling the low-end V8
  GC-on-context-dispose path on Linux.
- RGBA_4444 16-bit textures auto-enable only at ≤512 MB RAM — a 1–2 GB TV
  needs the explicit `--enable-rgba-4444-textures` switch.

**Action:** audit which low-end consumers actually fire on our Linux/
Starboard builds (the feature is undocumented upstream; crbug 40264947,
40343363) and un-gate or replicate the valuable ones. This is the same
"Linux is the neglected port" theme as spec 08.

### Binary residency may be the biggest unattacked Linux RSS line item

BlinkOn 19 again, empirically re-verified by the research pass: Linux
Chrome 128's binary is ~235 MB with **60–90% typically resident**; the
verifier measured 162 MB (69%) resident via `mincore()` during a youtube.com
session. In our single-process mode all resident code accrues to the one
process. If our stripped binary is similar in scale, **~100–160 MB of the
500 MB RSS may be code pages** — which would reframe the whole budget.

- **Action:** the profiling playbook (00) should split file-backed (code)
  vs anonymous RSS first — one `smaps_rollup` read answers it. If code pages
  are ~150 MB, the dead-code-removal track (owned elsewhere) is the single
  biggest memory lever too, plus linker options (-Oz, ICF, section GC,
  orderfile so hot code clusters and cold pages stay evicted).
- Caveat: these are shared, evictable, file-backed pages — cheaper than
  anonymous memory under pressure, but they still count against RSS-based
  kill decisions on TVs.

### cast_shell pattern: own your pressure policy (validates spec 08)

Chromecast's cast_shell replaced the default monitors with a custom 5-second
poll of `/proc/meminfo` `MemAvailable` with Cast-specific thresholds
(critical <25% available, moderate <40%) — from M64 through at least M120
(`chromecast/browser/cast_system_memory_pressure_evaluator.cc`, since removed
from upstream with the chromecast/ tree). The single-app-TV precedent for
spec 08 is exactly this: a device-appropriate evaluator, not desktop
defaults. Their thresholds/cadence are a tested starting point.

### Foreground discardable purge — verify-and-tune item

Chrome M89 shipped foreground-tab discardable reclaim ("up to 100 MiB per
tab" — offscreen decoded images etc.), via the scheduled purge in
`ClientDiscardableSharedMemoryManager` (5-minute minimum age). Default-on
machinery, present in our tree. For a single always-foreground tab this is
one of the few mechanisms that reclaims **while visible**.
**Action:** confirm the purge task actually runs in our single-process setup
and consider lowering the 5-min min-age for the TV UI's browse↔playback
rhythm (playback = huge offscreen-image working set going stale).

### Memory Saver's transferable idea: proactive, not pressure-reactive

Chrome 108's Memory Saver discards *proactively on a timer* rather than
waiting for pressure. Zero direct applicability (nothing to discard with one
tab), but the principle maps to us: e.g. purge browse-UI caches N seconds
into playback rather than waiting for a pressure signal that (per spec 08)
never fires on Linux today. Ideas 02/04 already move this direction.

## 3. Already banked / confirms our audit

| Finding | Status here |
|---|---|
| PartitionAlloc-as-malloc (M89; up to 22% browser-process savings on Win) | Already default in our build (`build_overrides/partition_alloc.gni:86-118`) |
| V8 Lite mode's non-regressing subset — lazy feedback allocation, bytecode flushing (5–15% of V8 heap), lazy source positions: ~18% avg V8 heap reduction | All default since Chrome 78 / V8 7.8; confirmed on in our vendored V8 (`flag-definitions.h:1055,2345`). Full `--lite-mode` adds only a small residual delta today |
| Sparkplug default-on costs code-space memory | We already ship `--no-sparkplug` (`cobalt_switch_defaults_starboard.cc:134`) — saving banked; blog numbers (5–10% Speedometer) quantify the CPU price paid |
| V8 memory-reducer mode (~historically 50% heap reduction on idle) | Fires via our `--enable-low-end-device-mode` → `--optimize-for-size` chain, verified end-to-end |
| PartialLowEndModeOnMidRangeDevices (M118 default) | Android-only feature; on AndroidTV it's active on 4–6 GB devices — irrelevant for ≤2 GB (full low-end mode applies); on Linux it doesn't exist (see ifdef gap above) |
| Isolated Splits (5–7% Chrome-on-Android memory) | Java/DEX-only savings, requires app-bundle + feature-module restructuring — poor fit for a native-heavy single-process TV browser; skip |

## 4. What the research did NOT cover (open)

Nothing survived verification on: **zram/zswap tuning, MADV_FREE vs
MADV_DONTNEED semantics, malloc trim under pressure** (OS layer); or the
named 2023–2026 renderer initiatives — **ParkableStrings/ParkableImages,
canvas hibernation, Oilpan compaction, V8 heap-sandbox reservation overhead**.
Absence = unverified, not nonexistent. ParkableStrings in particular is worth
a local audit: youtube.com/tv ships megabytes of JS source strings and the
feature parks/compresses them — check defaults and whether our single-process
mode changes behavior.

## Sources

- crbug 40264947 — partial low-end mode + demuxer-limit revert (primary)
- v8.dev/blog/{v8-lite, jitless, sparkplug, v8-release-74, optimizing-v8-memory}
- blog.chromium.org 2021/03 (PartitionAlloc everywhere, foreground discardable), 2021/11 (Isolated Splits)
- developer.chrome.com/blog/memory-and-energy-saver-mode
- BlinkOn 19: dape.pages.igalia.com/blink-on-19-chromium-for-low-memory-linux (slides), youtube.com/watch?v=6d6Wn8Nf-Fo
- chromium.googlesource.com M64/M120 `chromecast/browser/cast_*memory_pressure*`

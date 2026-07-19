# Cobalt memory experiments (`mem-combined` branch)

This branch merges all `mem/*` memory-reduction branches. Every behavioral
change is gated behind a **`base::Feature` that is OFF by default**. With no
feature enabled, the build behaves byte-for-byte like `main`.

## Control planes

### 1. h5vcc experiments API (production)

The web app persists experiment state through the H5vccExperiments API
(`cobalt/browser/h5vcc_experiments`). The config is a
`{features: {name: bool}, feature_params: {...}}` dictionary; at the next
startup `CobaltContentBrowserClient::SetUpCobaltFeaturesAndParams()` applies
each entry as a field-trial override **by feature name**.

Example `setExperimentState` payload enabling a soak set:

```json
{
  "features": {
    "CobaltMemMediaBudgets": true,
    "CobaltMemJsFlags": true,
    "CobaltMemStripDesktop": true,
    "CobaltMemGlibcTuning": true,
    "CobaltMemPaTuning": true,
    "CobaltMemImageCache": true,
    "CobaltMemDiscardable": true,
    "CobaltMemGstQueues": true,
    "CobaltMemIdlePurge": true,
    "CobaltMemAxAutodisable": true,
    "CobaltMemCacheSweep": true,
    "CobaltMemGpuBudget": true,
    "CobaltMemParkableStrings": true,
    "CobaltMem1080pUi": true,
    "CobaltMemThreadStacks": true,
    "CobaltMemBlobLimits": true
  },
  "feature_params": {}
}
```

Note: a new config takes effect on the **next** launch (the feature list is
built once, early in startup).

### 2. Command line (development)

```
--enable-features=CobaltMemJsFlags,CobaltMemImageCache
--disable-features=...
```

Explicit command-line entries always win over the experiment config
(`InitFromCommandLine()` runs first and the first override registered for a
name wins). The compound aliases below are expanded only from the h5vcc
config; when driving from the command line, pass the underlying feature
names directly (e.g. for the `CobaltMemAxAutodisable` effect use
`--enable-features=AutoDisableAccessibility`; for the full
`CobaltMemCacheSweep` effect pass all three names:
`--enable-features=CobaltMemCacheSweep,CobaltMemCacheSweepNet,CobaltMemCacheSweepSql`).

## Experiments

"Layer" is where the `BASE_FEATURE` is declared and consumed:

* **starboard** — declared in `starboard/extension/feature_config.h`; the
  macro machinery in `cobalt/common/features/features.{h,cc}` declares the
  matching `base::Feature` in `cobalt::features`, and
  `cobalt::features::InitializeStarboardFeatures()` (end of
  `CreateFeatureListAndFieldTrials()`) pushes the resolved state down to
  Starboard's FeatureList for the platform implementation to read. The push
  goes through the Starboard features extension
  (`kStarboardExtensionFeaturesName`), which is registered by the Android
  port and — as of this branch — the RDK port
  (`starboard/contrib/rdk/.../shared/features_extension.{h,cc}`, wired into
  the RDK `system_get_extensions.cc`). Starboard-side consumers guard every
  read with `FeatureList::IsFeatureListInitialized()` and fall back to the
  OFF behavior before the push arrives (early in browser startup), so
  pre-push callers never crash and never see an experiment ON.
* **browser** — declared in
  `cobalt/browser/memory_experiments/memory_experiment_features.{h,cc}`,
  consumed from cobalt/ code. Switch-bridged experiments are applied by
  `CobaltContentBrowserClient::ApplyMemoryExperimentSwitches()`, which edits
  the process command line right after the feature list is created (before
  the DevTools HTTP handler, renderer/compositor init, and the
  `CobaltCommandLine` log).
* **component** — declared in the consuming component (in its idiomatic
  features file where one exists: `net/base/features.h`,
  `sql/sql_features.h`, `storage/browser/blob/features.h`,
  `components/discardable_memory/common/discardable_memory_features.h`;
  file-locally in blink's `wtf/allocator/partitions.cc`), because
  `components/`, `net/`, `sql/`, `storage/` and `third_party/blink` cannot
  include cobalt/ headers. Since h5vcc overrides are applied by name, the
  name string is the contract and the declaration lives with the consumer.
* **alias** — no `BASE_FEATURE` anywhere; config-level name that
  `SetUpCobaltFeaturesAndParams()` fans out to upstream features (see next
  section).

| Feature name | Idea | ON behavior | Expected saving | Layer |
|---|---|---|---|---|
| `CobaltMemMediaBudgets` | 02 | Media pool initial 21→1 MiB; MSE video budget 30→16 MiB @1080p, 100→50 / 160→60 MiB @4K; audio 5→3 MiB; hard pool cap (24/64 MiB) | 10–15 MB @1080p, 40–100 MB @4K | starboard |
| `CobaltMemJsFlags` | 03 | `--js-flags` set to TV defaults with `--initial-old-space-size` 64→16, then any platform value appended (defaults first, platform wins per-flag) | 10–30 MB + CPU | browser (switch bridge) |
| `CobaltMemStripDesktop` | 04 | DevTools opt-in (default `--remote-debugging-port`/`--remote-allow-origins` removed); `WebAuth` appended to `--disable-blink-features`; ConversionMeasurement + InterestGroupStorage disabled (fan-out); domain reliability off; First-Party Sets off | 8–20 MB | browser (switch bridge + fan-out) |
| `CobaltMemGlibcTuning` | 05 | `mallopt` arena/mmap/trim tuning; periodic + conceal/freeze/low-memory `malloc_trim(0)` | 10–25 MB | starboard |
| `CobaltMemPaTuning` | 08 | PartitionAlloc thread-cache multiplier halved (low-end path) | 3–8 MB | component (blink wtf partitions) |
| `CobaltMemImageCache` | 09 | `--decoded-image-working-set-budget-bytes=25165824` (24 MB working set instead of the inert 128 MB default) | 10–25 MB browse | browser (switch bridge) |
| `CobaltMemDiscardable` | 10 | Discardable memory limit 16 MB; synchronous release on low-memory events | 5–15 MB | component (components/discardable_memory) |
| `CobaltMemGstQueues` | 11 | GStreamer pipeline queues bounded (30 buffers; video 8 MB / audio 4 MB) | 4–15 MB peak | starboard |
| `CobaltMemIdlePurge` | 12 | Quiescence purge sweep (5 min no input, not during playback; pressure notify + PA reclaim + discardable release) | 15–30 MB long sessions | browser |
| `CobaltMemAxAutodisable` | 13 | Enables upstream `AutoDisableAccessibility` (tears down unused AX trees) | 5–15 MB + churn | alias |
| `CobaltMemCacheSweep` | 16 | HTTP cache capped at 32 MB; SSL session cache 1024→32 entries (`CobaltMemCacheSweepNet`, net/); SQLite default page cache ~2 MB→256 KB (`CobaltMemCacheSweepSql`, sql/) | 3–7 MB | browser + component fan-out (net/, sql/) |
| `CobaltMemGpuBudget` | 17 | `--force-gpu-mem-available-mb=32` (compositor GPU budget 64→32 MB; UMA = RAM) | 5–15 MB | browser (switch bridge) |
| `CobaltMemParkableStrings` | 18 | Disables upstream `LessAggressiveParkableString` (ParkableString aging re-enabled in foreground; JS source compresses) | 5–13 MB | alias |
| `CobaltMem1080pUi` | 19 | Native UI window clamped to 1080p on 4K panels (video plane unaffected) | 15–40 MB on 4K panels | starboard |
| `CobaltMemThreadStacks` | 22 | `ReduceAndroidThreadStackSize` appended to `--enable-features`: 256 KiB default stacks for Chromium-created threads | 2–6 MB RSS + VA | browser (switch bridge) |
| `CobaltMemBlobLimits` | 23 | Blob in-memory cap memory/5 → memory/100 (Android formula) | 2–10 MB + tail insurance | component (storage/browser/blob) |

## Compound aliases

`SetUpCobaltFeaturesAndParams()` expands these config names into overrides on
the upstream features, after the config's explicit entries are applied.
Explicit entries win: if the config itself sets one of the target names, the
fan-out skips it (also guarding against the fatal-in-DCHECK-builds
double-registration of a feature name).

| Config name | Fans out to |
|---|---|
| `CobaltMemStripDesktop` | disable `ConversionMeasurement`, disable `InterestGroupStorage` |
| `CobaltMemAxAutodisable` | enable `AutoDisableAccessibility` |
| `CobaltMemParkableStrings` | disable `LessAggressiveParkableString` |
| `CobaltMemCacheSweep` | enable `CobaltMemCacheSweepNet` (net/), enable `CobaltMemCacheSweepSql` (sql/) |

`CobaltMemAxAutodisable` and `CobaltMemParkableStrings` are pure aliases: no
`BASE_FEATURE` is declared for them and `base::FeatureList::IsEnabled()` is
never called on them. `CobaltMemStripDesktop` and `CobaltMemCacheSweep` are
also declared features because the switch bridge and other browser-side sites
consume them directly; their fan-out targets are the extra pieces that live in
layers which cannot share the cobalt/ declaration (command-line users must
enable the component names explicitly, see above).

## Loader-level exceptions (still env/switch gated)

The Evergreen loader runs **before** Cobalt starts, so neither the h5vcc
experiment config nor `base::FeatureList` exists yet. These stay on their
original gates:

| Mechanism | Idea | ON behavior |
|---|---|---|
| `COBALT_MEM_EXP_LZ4_STREAM=1` (env var) | 20 | Loader streams LZ4 decompression (no whole-image buffer) — ~75–80 MB launch peak |
| `COBALT_MEM_EXP_LARGE_ALLOC_LOG=1` (env var) | 25 | Diagnostic: log allocations ≥16 MiB with return address (loader wrapper) |
| `--loader_use_memory_mapped_file` (loader_app switch, default off) | 01 | Decompress-once cache (`libcobalt.mmap.so`) + file-backed mmap of the binary — 40–60 MB |

## Build-time experiments (gn args — cannot be runtime flags)

| gn arg (args.gn) | Idea | ON behavior |
|---|---|---|
| `icu_use_data_file = true` | 07 | ICU data shipped as mmap'd `icudtl.dat` (~6.5 MB) |
| `cobalt_disable_base_tracing_in_gold = true` | 24 | base/perfetto tracing compiled out of gold evergreen images (~2–3 MB) |
| `use_large_empty_slot_span_ring = false` | 08b | Smaller PartitionAlloc empty-slot-span ring (~0.5–2 MB) |

## Limitations

* **`CobaltMemThreadStacks` — partial coverage.** The stack-size consumer
  string-scans the raw `--enable-features` switch, which the bridge edits
  early in startup, but threads created before that point (early browser
  threads) keep the platform default stack size. Bonus from the RDK features
  extension registration: `ReduceStarboardThreadStackSize` (the Starboard
  FeatureList consumer in `starboard/common/thread.cc`, 256 KiB default for
  unsized `starboard::Thread`s) is now functional on RDK too; threads created
  before the feature push still keep the platform default.
* **`CobaltMemGlibcTuning` — not applied at process start.** The `mallopt`
  arena/mmap/trim tuning and the periodic `malloc_trim` chain are applied by
  `ApplicationRdk::OnStarboardFeaturesInitialized()`, invoked by the RDK
  features extension immediately after it populates Starboard's FeatureList
  (i.e. during `CreateFeatureListAndFieldTrials()`, early in browser startup
  — before media playback and most worker threads, but after process start).
  Allocations and arena attachments made before that keep the untuned
  behavior; the arena cap bounds arena growth from feature arrival onward.
* **`CobaltMemMediaBudgets` — read lazily.** The Starboard media-budget
  functions and the `DecoderBufferAllocator` pool cap re-read the FeatureList
  on every call instead of caching at construction, so an allocator created
  before the feature push cannot latch the uncapped/upstream values forever.
  In practice the allocator (and every GStreamer pipeline for
  `CobaltMemGstQueues`) is created well after the push; anything that does
  run earlier simply sees the OFF values.
* **`CobaltMem1080pUi` — pre-push windows are not re-clamped.** The clamp is
  evaluated when the native (Essos) window is materialized. Window creation
  (content shell → ozone → `SbWindowCreate`) happens well after the feature
  push in `PostEarlyInitialization`, so the clamp normally applies. If a
  window were ever created before the push it would stay at panel resolution
  until it is destroyed and recreated (app suspend/resume with
  `COBALT_ESSOS_CONTEXT_DESTROY`, or SbWindow destroy); there is no
  re-clamp-on-feature-arrival path.
* **Config latency.** h5vcc-set experiment state takes effect on the next
  launch, not the current session.

## Migration status

* All browser-code and component-layer call sites (idle purge creation, HTTP
  cache cap, domain reliability and First-Party Sets in
  `cobalt_content_browser_client.cc`; glibc trim and discardable release in
  `cobalt/app/app_event_runner.cc`; `net/`, `sql/`, `storage/`,
  `components/discardable_memory`, `third_party/blink`) now use
  `base::FeatureList::IsEnabled(<feature>)` — no `COBALT_MEM_EXP_*` env-var
  gates remain outside the loader-level exceptions above.
* A feature name must have exactly **one** `BASE_FEATURE` declaration in the
  process (`base` DCHECKs on duplicate feature-name identity). Where one
  experiment spans several layers that cannot share a header, only one layer
  declares the canonical name and the other layers declare distinct names
  that `SetUpCobaltFeaturesAndParams()` enables via the fan-out: this is how
  `CobaltMemCacheSweep` (cobalt/browser) reaches `CobaltMemCacheSweepNet`
  (net/) and `CobaltMemCacheSweepSql` (sql/).
* Feature-check timing: `base::FeatureList::IsEnabled()` without a FeatureList
  instance returns the feature's default state (disabled → upstream
  behavior), **but** the access is recorded and turns into a fatal `CHECK` on
  Linux once `content::RunContentProcess()` has armed
  `FailOnFeatureAccessWithoutFeatureList()` (or at `SetInstance()`); the
  converted call sites therefore all run provably after the feature list is
  created in `CobaltMainDelegate::PostEarlyInitialization()` (see the
  comments at each site).

## Suggested soak protocol

1. Baseline: empty config (must match `main` behavior).
2. Everything-on soak: set all feature names in the table to `true`.
3. Bisect regressions by turning individual features off from the
   everything-on config, or validate individual wins by turning single
   features on from baseline.

# Cobalt memory experiments (`mem-combined` branch)

This branch merges all `mem/*` memory-reduction branches, with every
behavioral change gated behind an **experiment flag that is OFF by default**.
With no flags set, the build behaves byte-for-byte like `main`.

## Runtime flags (environment variables)

Set in the process environment (e.g. the WPEFramework/Thunder plugin launch
config or a wrapper script). A flag is ON when set to `1`. The master switch
`COBALT_MEM_EXP_ALL=1` turns on every runtime experiment at once; individual
flags still work alongside it.

| Env var | Idea | ON behavior | Expected saving |
|---|---|---|---|
| `COBALT_MEM_EXP_MEDIA_BUDGETS` | 02 | Media pool initial 21→1 MiB; MSE video budget 30→16 MiB @1080p, 100→50 / 160→60 MiB @4K; audio 5→3 MiB; hard pool cap (24/64 MiB) enforced | 10–15 MB @1080p, 40–100 MB @4K |
| `COBALT_MEM_EXP_JS_FLAGS` | 03 | Platform `--js-flags` merge with TV defaults (platform wins per-flag) instead of replacing them; `--initial-old-space-size` 64→16 | 10–30 MB + CPU |
| `COBALT_MEM_EXP_STRIP_DESKTOP` | 04 | DevTools opt-in (no default :9222); ConversionMeasurement, InterestGroupStorage, WebAuth disabled; domain reliability off; First-Party Sets off | 8–20 MB |
| `COBALT_MEM_EXP_GLIBC_TUNING` | 05 | `mallopt` arena/mmap/trim tuning at startup; periodic + conceal/freeze/low-memory `malloc_trim(0)` | 10–25 MB |
| `COBALT_MEM_EXP_PA_TUNING` | 08 | PartitionAlloc thread-cache multiplier halved (low-end path) | 3–8 MB |
| `COBALT_MEM_EXP_IMAGE_CACHE` | 09 | Decoded-image working set budget switch applied (24 MB instead of inert 128 MB default) | 10–25 MB browse |
| `COBALT_MEM_EXP_DISCARDABLE` | 10 | Discardable memory limit 16 MB; synchronous release on low-memory events | 5–15 MB |
| `COBALT_MEM_EXP_GST_QUEUES` | 11 | GStreamer pipeline queues bounded (30 buffers; video 8 MB / audio 4 MB) | 4–15 MB peak |
| `COBALT_MEM_EXP_IDLE_PURGE` | 12 | Quiescence purge sweep (5 min no input, not during playback; pressure notify + PA reclaim + discardable release) | 15–30 MB long sessions |
| `COBALT_MEM_EXP_AX_AUTODISABLE` | 13 | AutoDisableAccessibility feature enabled (tears down unused AX trees) | 5–15 MB + churn |
| `COBALT_MEM_EXP_CACHE_SWEEP` | 16 | SSL session cache 1024→32 entries; HTTP cache 32 MB; SQLite default page cache ~2 MB→256 KB | 3–7 MB |
| `COBALT_MEM_EXP_GPU_BUDGET` | 17 | Compositor GPU memory budget 64→32 MB (UMA = RAM) | 5–15 MB |
| `COBALT_MEM_EXP_PARKABLE_STRINGS` | 18 | ParkableString aging re-enabled in foreground (JS source compresses) | 5–13 MB |
| `COBALT_MEM_EXP_1080P_UI` | 19 | Native UI window clamped to 1080p on 4K panels (video plane unaffected) | 15–40 MB on 4K panels |
| `COBALT_MEM_EXP_LZ4_STREAM` | 20 | Evergreen loader streams LZ4 decompression (no whole-image buffer) | ~75–80 MB launch peak |
| `COBALT_MEM_EXP_THREAD_STACKS` | 22 | ReduceAndroidThreadStackSize enabled: 256 KiB default stacks for Chromium threads | 2–6 MB RSS + VA |
| `COBALT_MEM_EXP_BLOB_LIMITS` | 23 | Blob in-memory cap memory/5 → memory/100 (Android formula) | 2–10 MB + tail insurance |
| `COBALT_MEM_EXP_LARGE_ALLOC_LOG` | 25 | Diagnostic: log allocations ≥16 MiB with return address (loader wrapper) | — (attribution tool) |

## Flags with their own pre-existing gate

| Mechanism | Idea | ON behavior |
|---|---|---|
| `--loader_use_memory_mapped_file` (loader_app switch, default off) | 01 | Decompress-once cache (`libcobalt.mmap.so`) + file-backed mmap of the binary — 40–60 MB |

## Build-time experiments (gn args — cannot be runtime flags)

| gn arg (args.gn) | Idea | ON behavior |
|---|---|---|
| `icu_use_data_file = true` | 07 | ICU data shipped as mmap'd `icudtl.dat` (~6.5 MB) |
| `cobalt_disable_base_tracing_in_gold = true` | 24 | base/perfetto tracing compiled out of gold evergreen images (~2–3 MB) |
| `use_large_empty_slot_span_ring = false` | 08b | Smaller PartitionAlloc empty-slot-span ring (~0.5–2 MB) |

## Always-on (no behavior change)

* Idea 15: surface-readback canary logging (`Surface readback requested`,
  `CopyOutputRequest created`) — production should log zero of these.
* Idea 13's AXMode startup/change logging; idea 03's effective `--js-flags`
  log; idea 08's reclaimer-start log — observability only.
* Idea 05/25 loader symbol registrations (`mallopt`, `malloc_trim`, alloc
  wrappers) — inert until the corresponding experiment calls them.
* Idea 22's crossed-gating fix in `starboard/common/thread.cc` — inert on RDK
  until the platform registers the Starboard features extension.

## Suggested soak protocol

1. Baseline: no flags (must match `main` behavior).
2. `COBALT_MEM_EXP_ALL=1` — the everything-on soak.
3. Bisect regressions by turning individual flags off from the ALL state, or
   validate individual wins by turning single flags on from baseline.

# Memory profiling playbook: attributing Cobalt's ~500 MB

Goal: produce an **attribution table — memory category × app scenario** — so
we know who owns the 500 MB before optimizing, and can verify each idea in
this folder against reality. Almost everything needed already ships in this
tree; this doc is the assembly manual.

Use a **non-release build** (`COBALT_IS_RELEASE_BUILD=0`): tracing
(`tracing::InitTracingPostFeatureList`, `cobalt_browser_main_parts.cc`) and
the in-process heap profiler are compiled out of release builds. Single
process means one PID owns everything — one smaps, one trace.

## The three layers

Measure top-down. Each layer explains the layer above.

| Layer | Question | Tool |
|---|---|---|
| 1. OS | How big, and in which mappings? | `/proc/<pid>/smaps` + `cobalt/tools/performance/smaps/` |
| 2. memory-infra | Which subsystem owns it? (V8, Oilpan, PartitionAlloc, cc, gpu, media, discardable, sqlite…) | tracing with `disabled-by-default-memory-infra` via DevTools port 9222 |
| 3. Heap/JS drill-down | Which call stacks / JS objects? | `--enable-heap-profiling` (native, 128 KB sampling) + DevTools heap snapshot (JS) |

## Scenario matrix

Capture every layer at each checkpoint; 2–3 minutes settle time each so
periodic reclaim and GC reach steady state. This mirrors what
`rdk_yttv_memory_benchmark.sh` already automates for RDK devices.

1. Boot → splash → home feed loaded (idle)
2. Home feed, continuous scroll for 60 s (image cache churn)
3. Search results page
4. 1080p playback, 60 s in
5. 4K (HDR if available) playback, 60 s in ← peak case
6. Playback paused 5 min, then back to home feed (what shrinks back?)
7. App concealed/backgrounded 5 min (what should idea 04 reclaim?)

## Layer 1 — OS ground truth (smaps)

The in-tree pipeline does capture → parse → categorize → visualize:

```bash
# On/against the device (adb or ssh), periodic snapshots:
python3 cobalt/tools/performance/smaps/smaps_capture.py   # → cobalt_smaps_logs/
# Then:
python3 cobalt/tools/performance/smaps/run_analysis_pipeline.py
python3 cobalt/tools/performance/smaps/visualize_smaps_analysis.py
```

Quick manual look: `adb shell cat /proc/$(adb shell pidof dev.cobalt.coat)/smaps_rollup`
(RSS/PSS/Private_Dirty/Swap). Full `smaps` grouped by mapping name splits:
`libchrobalt.so` code, other libraries/GPU drivers, anonymous heaps, stacks.
On Android, named anon VMAs (`[anon:partition_alloc]`, `[anon:v8]`, …) give a
free first-cut attribution.

**First split to make: file-backed (code) vs anonymous RSS.** External data
(see [00c](00c-external-research-findings.md)) says a Chromium binary of this
era is ~235 MB with 60–90% typically resident — Chrome 128 measured ~162 MB
(69%) resident on a youtube.com session. If our binary's resident code pages
are on that order, **up to a third of the 500 MB may be code**, none of which
any idea in this folder touches (that's the dead-code/binary-size track, plus
linker/orderfile work). One `smaps_rollup` read settles it before anything
else is measured.

Two things RSS **misses** on TVs — capture separately or the table won't add up:
- **GPU/driver memory** (dmabuf/gralloc/Mali): `adb shell dumpsys meminfo
  dev.cobalt.coat` (Graphics row + EGL mtrack) on Android; Mali debugfs on RDK
  (the rdk benchmark script already reads it).
- **Platform media decoder memory** (SbPlayer side): vendor-specific; note it,
  don't chase it from this repo.

## Layer 2 — memory-infra trace (the money layer)

DevTools is on by default (`--remote-debugging-port=9222`). Take a detailed
memory dump via CDP — `Tracing.requestMemoryDump` is purpose-built for this:

```python
# pip install websocket-client requests; adb forward tcp:9222 tcp:9222
import json, requests, websocket
ws_url = requests.get("http://localhost:9222/json/version").json()["webSocketDebuggerUrl"]
ws = websocket.create_connection(ws_url, max_size=100 * 1024 * 1024)
def rpc(id_, method, params=None):
    ws.send(json.dumps({"id": id_, "method": method, "params": params or {}}))
    while True:
        msg = json.loads(ws.recv())
        if msg.get("id") == id_:
            return msg

rpc(1, "Tracing.start", {"traceConfig": {
    "includedCategories": ["disabled-by-default-memory-infra"],
    "memoryDumpConfig": {"triggers": []}}})
print(rpc(2, "Tracing.requestMemoryDump", {"levelOfDetail": "detailed"}))
rpc(3, "Tracing.end")
# collect Tracing.dataCollected events until Tracing.tracingComplete, save as .json
```

Load the trace in Perfetto UI (or catapult trace viewer) → the memory-infra
panel shows effective_size per dump provider. The rows to read against our
specs:

| Trace category | Confirms/kills |
|---|---|
| `cc/image_decode_cache` (per LayerTreeHostImpl) | **Idea 06** — is it really >24 MB on the home feed? |
| `media/source_buffer` / demuxer streams, DecoderBufferAllocator | **Ideas 01/02** — encoded-buffer residency during 4K |
| `discardable` | Ideas 04/06 interplay |
| `gpu/shared_images`, `gpu/gl`, `skia/gpu_resources`, `gpu/shader_cache` | **Idea 07** |
| `font_caches` + malloc slice of SkFontMgr chunks | **Idea 05** |
| `v8/main/heap/*`, `blink_gc/main/heap` | JS/DOM baseline; worker isolates show up as extra `v8/` rows |
| `partition_alloc/partitions/*` vs its allocated_objects | PA slack → idea 04's reclaim target |
| `malloc` minus everything attributed | the "dark matter" → layer 3 |

Also enable the in-tree attribution rollup: add
`CobaltMemoryAttributionManager` to `--enable-features` — it registers its own
dump provider and periodically reports `Memory.Experimental.Browser2.*` UMA
pillars (Malloc, LibChrobaltRss, PartitionAlloc, GPU…), which
`cobalt/tools/performance/memory/compare_histograms.py` /
`analyze_cumulative_memory.py` turn into "% of RSS accounted for" reports, and
`compare_accuracy.py` cross-checks against the smaps ground truth.

## Layer 3 — drill into the dark matter

- **Native heap:** launch with `--enable-heap-profiling`
  (`cobalt_browser_main_parts.cc:155-167`; 128 KB sampling,
  native-with-thread-names stacks). Heap dumps ride along inside memory-infra
  traces; symbolize and diff with
  `cobalt/tools/performance/memory/symbolize_in_process_heap.py` and
  `diff_in_process_heaps.py` (diff two scenarios to see what grew, e.g. home
  feed vs after-playback).
- **JS heap:** open `http://localhost:9222` in desktop Chrome → inspect the
  youtube.com/tv target → Memory tab → heap snapshot. Snapshot at scenarios 1,
  5, 6 and diff. (`PreciseMemoryInfo` is already enabled, so
  `performance.memory` in the console gives cheap continuous numbers.)
- **Counts for idea 03:** in the inspected page,
  `window.performance.navigation` history depth won't show BFCache; instead
  log `BackForwardCacheImpl` entries (`chrome://process-internals` isn't
  available in shell — simplest is a temporary VLOG or checking the
  `BackForwardCache.HistoryNavigationOutcome` UMA via `chrome://histograms`
  through DevTools: `Page.navigate` to `chrome://histograms/BackForwardCache`).

## Reading the results (what "good" looks like)

- The pillars (Malloc + PartitionAlloc + libchrobalt code + GPU + V8/Oilpan +
  media pool) should account for ≥85–90 % of RSS (that's what
  `analyze_cumulative_memory.py` computes). If not, the gap is usually GPU
  driver memory or untracked mmaps — go back to smaps.
- Compare scenario 6 vs scenario 1: anything that didn't return to baseline is
  a retention bug (ideas 02/04 territory).
- Keep the raw traces; every idea spec in this folder has a "Validation"
  section that names the row it needs.

## Multi-hour (soak) profiling on Linux

Long runs are not only possible, they're the only way to see slow growth
(cache creep, fragmentation, leaks). The key insight: **don't record one
continuous trace for hours** — trace buffers overflow and the file becomes
unmanageable. Memory state is a *snapshot*, so profile as a snapshot loop.

### Overhead budget (why hours are fine)

- `--enable-heap-profiling`: Poisson sampling at 128 KB — roughly 1–3 % CPU,
  constant over time. Its bookkeeping holds the live sampled-allocation map
  in-process (can reach tens of MB on a big heap): run one soak **with** it
  for attribution and one **without** it for clean RSS numbers.
- A detailed memory-infra dump: ~100 ms–1 s pause, a few MB of JSON. At one
  dump every 5 minutes this is invisible (12/hour ≈ 50 MB/day of traces).
- What does NOT work for hours: heaptrack / valgrind-massif (record every
  allocation; 10–50× slowdown, unbounded output) — don't bother; the in-tree
  sampling profiler answers the same question.

### The loop

Start Cobalt once (non-release build, `--enable-heap-profiling`, DevTools on
9222 as default), then from a workstation or the same host:

```bash
while true; do
  ts=$(date +%s)
  python3 snapshot.py > "soak/trace_${ts}.json"       # CDP snippet from above:
                                                      # Tracing.start → requestMemoryDump
                                                      # (detailed) → Tracing.end → save
  cat /proc/$(pidof cobalt)/smaps_rollup > "soak/smaps_${ts}.txt"
  sleep 300
done
```

Each trace file is a self-contained snapshot including the heap-profiler dump
(allocation stacks) at that moment. Nothing accumulates in the browser between
snapshots — the loop can run for days.

For continuous coarse curves between snapshots, run the in-tree UMA loop in
parallel (it was built for exactly this):
`python3 cobalt/tools/performance/memory/compare_histograms.py --interval 60
--duration 28800` plus `smaps_capture.py` — see that README's guidance on
median-vs-max jitter across long windows.

### Reading a soak

1. Plot `smaps_rollup` RSS/PSS over time first — is there growth at all, and
   is it linear (leak-shaped) or step/plateau (cache-shaped)?
2. Feed the trace snapshots to Perfetto `trace_processor` and query the
   `memory_snapshot_node` tables to get a per-category time series — this
   tells you *which provider row* grows (image cache? PA slack? V8 old space?).
3. For the growing row, diff heap dumps far apart in time:
   `diff_in_process_heaps.py trace_t0.json trace_t8h.json` (symbolize with
   `symbolize_in_process_heap.py`) — allocation stacks present at t=8h but not
   t=0, sorted by bytes, is your leak/creep suspect list.
4. Keep the scenario stationary during the soak (e.g. looped playback, or an
   idle home feed with screensaver disabled) so growth is attributable to
   time, not scenario changes. A TV remote-input replay loop makes a good
   "browse forever" soak.

### Long-run gotchas

- Disable any screensaver/suspend on the device and the app-side idle timeout,
  or the run silently turns into a "concealed" scenario mid-soak.
- V8/PA reclaim runs on timers — a dump taken seconds after a GC differs from
  one taken before; at 5-minute cadence this jitter averages out, so trust
  trends, not adjacent-sample deltas.
- DevTools connections occasionally drop on long runs; make the snapshot
  script reconnect per iteration (fresh websocket each time, as in the loop
  above) rather than holding one connection open.
- If the heap-profiler's own bookkeeping growth muddies the water, re-run the
  suspect window at a coarser sampling rate (e.g. 512 KB) — attribution keeps
  working, overhead and self-memory shrink.

## Gotchas

- File-backed mappings (fonts, ICU, snapshot, .so text) are shared/clean —
  look at PSS/Private_Dirty, not RSS, before celebrating "savings" there.
- UMA histograms have 1 MiB resolution and 60 s cadence; median across
  snapshots, ignore the first (see `cobalt/tools/performance/memory/README.md`
  "Gotchas" — timing jitter between smaps and UMA produces fake deltas).
- memory-infra's `effective_size` de-duplicates shared ownership; don't sum it
  with smaps numbers from a different instant.
- Release builds silently lack tracing + heap profiler; if
  `Tracing.requestMemoryDump` returns nothing, check the build flavor first.

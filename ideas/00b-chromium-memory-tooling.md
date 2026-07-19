# What upstream Chromium ships for per-feature memory attribution

The Chromium memory team's answer to "how much memory does feature X use" is
**not** smaps. It's a three-part system, all of which is present in this tree,
with the team's own docs checked in under `docs/memory/` and
`docs/memory-infra/` (start with `docs/memory/README.md`,
`docs/memory/key_concepts.md`, `docs/memory/tools.md`).

## 1. memory-infra: the per-subsystem accounting graph

Every major subsystem implements `base::trace_event::MemoryDumpProvider`.
On a *detailed* dump, each provider emits a tree of `MemoryAllocatorDump`
nodes with byte counts **and ownership edges** between providers (e.g. a cc
image-cache entry *owns* a discardable segment which is *backed by* shared
memory). The graph is deduplicated into `effective_size`, so the whole thing
sums to something meaningful — this is the "who owns what" answer, designed by
the Chrome memory team precisely because smaps can't attribute.

Docs: `docs/memory-infra/README.md`, plus per-probe deep dives
`probe-cc.md`, `probe-gpu.md`, `probe-net.md`, and
`adding_memory_infra_tracing.md` (for adding Cobalt-specific probes, e.g. a
`DecoderBufferAllocator` provider if media attribution proves thin).

The provider vocabulary in this tree (~60 registered, grep
`RegisterDumpProvider`) — i.e. the feature breakdown you actually get:

| Domain | Providers |
|---|---|
| JS/DOM | `V8Isolate` (per-space: code, old, new, map…; per-isolate incl. workers), `BlinkGC` (Oilpan), `BlinkObjectCounters`, `V8SharedMemory` |
| Renderer caches | `MemoryCache` (Blink resource cache), `FontCaches`, `ParkableStrings`, `ParkableImages`, `Canvas`, `hibernated_canvas` |
| Compositor | `cc::GpuImageDecodeCache`, `cc::SoftwareImageDecodeCache`, `cc::ResourcePool`, `cc::StagingBufferPool`, `TileManager`, `FrameEvictionManager` |
| GPU service | `gpu::TextureManager`, `gpu::BufferManager`, `gpu::TransferBufferManager`, `gpu::ServiceTransferCache`, `gpu::ServiceDiscardableManager`, `SharedImageManager`, `SharedContextState`, `GrShaderCache`, `CommandBuffer`, `Skia`, `vulkan`, `AndroidGraphics` |
| Allocators | `Malloc`, `PartitionAlloc` (per-partition!), `PartitionAlloc.AddressSpace`, `DiscardableSharedMemoryManager`, `ClientDiscardableSharedMemoryManager`, `MadvFreeDiscardableMemoryAllocator`, `SharedMemoryTracker` |
| Storage | `DOMStorage`, `LocalStorage`, `SessionStorage`, `IndexedDBBackingStore`, `BlobStorageContext`, `LeveldbValueStore`, `MojoLevelDB`, `sql::Database` (per-DB!) |
| Media | `media::VideoResourceUpdater`, `GpuMemoryBufferVideoFramePool`, `FrameBufferPool`, `TextureOwner` |
| Net | HTTP cache / SSL / socket dumps (see `probe-net.md`) |

How to trigger: any tracing session including the
`disabled-by-default-memory-infra` category — DevTools CDP
`Tracing.requestMemoryDump` (see playbook 00), Perfetto config with a
`memory_dump_config`, or startup tracing
(`docs/memory-infra/memory_infra_startup_tracing.md`) for boot-time footprint.
View in Perfetto UI or catapult trace viewer; both understand the dump graph.

Caveat: providers cover "reported_by_chrome". The remainder —
`Malloc`/`PartitionAlloc` totals minus everyone's `allocated_objects` — is
dark matter that only layer 2 explains.

## 2. Heap profilers: attribute the dark matter by call stack

Chromium's answer for "which *code* allocated this" — the missing piece once
you know the allocator but not the feature:

- **Out-of-process heap profiler ("memlog")** —
  `components/services/heap_profiling/` + `docs/memory-infra/heap_profiler.md`.
  In stock Chrome: `--memlog=browser --memlog-stack-mode=native-with-thread-names
  --memlog-sampling-rate=100000`, results embedded as heap dumps inside
  memory-infra traces. Cobalt already wires this same service behind
  `--enable-heap-profiling` (`cobalt_browser_main_parts.cc`, 128 KB Poisson
  sampling). Chrome's trigger UI (`chrome://memory-internals`) isn't built
  into the Cobalt shell, but the trace path works headlessly.
  `docs/memory/investigating_heap_dump_example.md` is a worked example of
  going from heap dump → responsible feature/CL.
- **Perfetto heapprofd (AndroidTV)** —
  `third_party/perfetto/tools/heap_profile -n <process>` profiles native
  malloc in *any* profileable process with **no special build flags**,
  produces pprof flamegraphs. This is the Chrome-on-Android team's
  recommended tool today (`docs/memory/android_dev_tips.md`). Aggregating the
  flamegraph by source directory gives a defensible per-component ranking.
  `tools/java_heap_dump` covers the Java side of the APK.
- Both sample from the same `PoissonAllocationSampler`, so numbers are
  statistically sound for MB-scale questions.

## 3. Telemetry memory benchmarks: dumps → named per-feature metrics in CI

This is how the Chrome memory team tracks regressions per feature:
`tools/perf/benchmarks/memory.py` (+ `docs/memory-infra/memory_benchmarks.md`)
drives scripted scenarios, takes periodic detailed dumps, and the catapult
TBMv2 memory metric flattens the dump graph into named series like

```
memory:chrome:all_processes:reported_by_chrome:v8:effective_size
memory:chrome:all_processes:reported_by_chrome:cc:effective_size
memory:chrome:all_processes:reported_by_malloc:allocated_objects_size
memory:chrome:all_processes:private_memory_footprint
```

`private_memory_footprint` (PMF) is Chrome's canonical topline number
(`docs/memory/key_concepts.md`), computed by
`services/resource_coordinator/memory_instrumentation` — the same service
Cobalt's UMA pillars ride on. For programmatic diffs, Perfetto
`trace_processor` exposes the dumps as SQL tables (`memory_snapshot_node`), so
"scenario A vs scenario B per category" is a query, not eyeballing.

Full Telemetry likely won't drive a Starboard TV device out of the box, but
the *metric* half is reusable: record the trace on-device (playbook 00), run
it through trace_processor/TBM off-device.

## Putting it together for "memory by feature"

| Question | Upstream tool |
|---|---|
| MB per subsystem, right now | memory-infra detailed dump (one CDP call) |
| MB per code path / component, incl. dark matter | memlog heap dump in trace (Linux/all) or heapprofd (AndroidTV), rolled up by directory |
| MB per JS feature / retained object graph | DevTools heap snapshot + per-context `V8Isolate` dumps |
| Trend per feature across builds/scenarios | trace_processor SQL / TBM memory metric over recorded traces |
| Topline number to optimize | `private_memory_footprint` (PMF), not RSS |

Relationship to the Cobalt-specific playbook (00): same trace, same dumps —
Cobalt's scripts add TV-device capture and UMA cross-checks; everything in
this doc is the upstream machinery underneath, plus the heap-profiler layers
smaps can't provide.

# Add an OS memory-pressure evaluator for Linux/Starboard (PSI/meminfo)

**Estimated impact: not a direct RSS reduction — it makes every purge
mechanism in this folder actually fire before the kernel OOM-kills the app.
On 1–2 GB devices this is the difference between "purge 30–80 MB" and "app
restart." Complements ideas 02/04/05.**
**Effort: small–medium (~150 lines, self-contained, prior art exists)**

## Problem (verified in-tree)

Cobalt on Linux creates a `MultiSourceMemoryPressureMonitor`
(`content/browser/browser_main_loop.cc:371-376`) — but the monitor has **no
system pressure source on Linux**:

- `SystemMemoryPressureEvaluator::CreateDefaultSystemEvaluator`
  (`components/memory_pressure/system_memory_pressure_evaluator.cc:30-45`)
  returns an evaluator only for **Fuchsia, Mac, and Windows**; the `#else`
  branch returns `nullptr` ("Chrome OS and Chromecast evaluators are created
  in separate components" — neither is built here).
- So on Linux/Starboard TVs, `base::MemoryPressureListener` CRITICAL — the
  trigger for discardable purge-to-zero, PartitionAlloc reclaim, Skia cache
  purge, Blink cache drop, and V8 low-memory GC (all verified working
  in-process in idea 04's vetting) — fires only from:
  1. `SbEventTypeLowMemory` (`cobalt/app/app_event_runner.cc:268-281`) —
     vendor-dependent, typically delivered when the platform is already
     desperate, and
  2. the delayed background purge (`MemoryPurgeManager`, 1–4 min after hide).
  Nothing reacts to actual memory tightness while the app is **foreground**.
- AndroidTV is covered by Java `onTrimMemory`
  (`CobaltActivity.java:506`, `MemoryPressureMonitor.java:122`) — this spec
  is for the Linux/Starboard family (including RDK/Evergreen devices, often
  the lowest-RAM fleet).

## Prior art

- **Endless OS** carried downstream Chromium patches doing exactly this for
  sub-2 GB devices: a Linux `MemoryPressureMonitor` modeled on ChromiumOS's,
  because upstream Chromium reads `/proc/meminfo` globally and is blind to
  cgroup limits (relevant if Cobalt runs in a container, as on some RDK
  deployments).
- **ChromeOS** ships its own evaluator component (the `#else` comment above),
  margin-based on available memory.
- Modern kernels expose **PSI** (`/proc/pressure/memory`, kernel ≥4.20) —
  the cleanest signal: `some`/`full` stall percentages map naturally to
  MODERATE/CRITICAL votes.

## Proposal

Implement `SystemMemoryPressureEvaluatorLinux` in
`components/memory_pressure/` (or a Cobalt-specific evaluator registered from
`CobaltBrowserMainParts`):

1. Primary signal: poll or epoll `/proc/pressure/memory`; vote MODERATE at
   e.g. `some avg10 > 10`, CRITICAL at `full avg10 > 5` (tune on target).
2. Fallback for older TV kernels (4.9/4.14 are common): available-memory
   margin from `/proc/meminfo` (`MemAvailable` below X → MODERATE, below Y →
   CRITICAL), and read cgroup v1/v2 limits when present so containers work.
3. Register via the existing voter API
   (`monitor->CreateVoter()`); the `kRenotifyVotePeriod = 5 s` renotify logic
   already exists in the base class, which keeps CRITICAL re-broadcasting
   while pressure persists.

## Why this is worth it despite not lowering steady-state RSS

Every idea in this folder makes memory *reclaimable*; this makes reclamation
*happen* at the right moment on the platform family that has no other trigger.
It also protects against regressions: any future cache that respects memory
pressure gets TV coverage for free.

## Risks / notes

- Threshold thrash: CRITICAL purges are expensive (full V8 GC + re-decode);
  hysteresis and the 5 s renotify period mitigate. Start conservative.
- PSI requires `CONFIG_PSI=y` (and sometimes `psi=1` on the cmdline) — the
  meminfo fallback must be solid, not an afterthought.
- Validate against vendor low-memory killers (LMKD-like daemons on some TV
  stacks) so both mechanisms don't fight.

## Validation

- On a 1 GB Linux/RDK device, run a memory-hog alongside Cobalt: without the
  evaluator expect an OOM kill; with it expect staged purges (watch
  memory-infra dumps + `Memory.Experimental` UMA) and survival.
- Soak with playback: confirm no purge-thrash during normal 4K sessions.

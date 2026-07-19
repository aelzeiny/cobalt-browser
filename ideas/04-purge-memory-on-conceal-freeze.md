# Purge memory deterministically on the TV lifecycle (conceal/freeze)

> **VETTED — premise corrected, value re-scoped.** The original claim
> "lifecycle transitions free nothing" was **half wrong**:
> on **Linux/Starboard**, Blink's `MemoryPurgeManager` already fires a
> CRITICAL memory-pressure purge ~1 s after freeze and 1–4 min after the
> renderer is backgrounded (`kMemoryPurgeInBackground` default-enabled,
> `third_party/blink/renderer/platform/scheduler/main_thread/memory_purge_manager.cc:27-42,82-98,130-184`).
> On **AndroidTV** the purge manager is compiled off (`kPurgeEnabled = false`
> on Android, `memory_purge_manager.h:75-81`) and the app relies solely on
> system `onTrimMemory`. So the win is: **immediacy** (purge now instead of
> up to 4 min later) on Linux, and a genuinely **net-new deterministic purge
> on AndroidTV** — the platform where low-memory kills matter most.

**Estimated savings: 30–80 MB purged on conceal (absolute volume confirmed
plausible); as *net-new* background RSS it's mainly an AndroidTV win plus a
1–4-minute head start on Linux. Compounds with idea 02.**
**Effort: small — ~5 lines copied into the lifecycle hooks**

## Confirmed mechanics

- Lifecycle flow is unified through the Starboard event pump on both
  platforms: `SbEvent` → `AppEventRunner::OnConceal/OnFreeze`
  (`cobalt/app/app_event_runner.cc:525,545`) → `Shell::OnConceal/OnFreeze` →
  `ShellPlatformDelegate` (`cobalt/shell/browser/shell_platform_delegate.cc:129,183`),
  which today only calls `WasHidden()` / `SetPageFrozen(true)` +
  HangWatcher suspend.
- The purge broadcast works in single-process: `base::MemoryPressureListener`
  is process-global; `RenderThreadImpl` registers a listener
  (`content/renderer/render_thread_impl.cc:643-648`) and on CRITICAL runs the
  full chain — Blink registry purge + PA `ReclaimAll`, discardable
  `ReleaseFreeMemory`, `SkGraphics::PurgeAllCaches`, `ImageDecodingStore`
  clear (`render_thread_impl.cc:1675-1685`), and a real V8 GC via
  `MemoryPressureNotificationToAllIsolates`
  (`kForwardMemoryPressureToBlinkIsolates` default-on,
  `content_features.cc:485-487`). CRITICAL is downgraded for *visible*
  renderers (`render_thread_impl.cc:1822-1828`) — on conceal the widget is
  hidden, so the full GC happens.
- The template to copy already exists at `app_event_runner.cc:268-281`
  (`OnLowMemory`): `NotifyMemoryPressure(CRITICAL)` +
  `partition_alloc::MemoryReclaimer::Instance()->ReclaimAll()`.
- Confirmed non-lever: `PartitionAllocSupport::OnBackgrounded()` early-returns
  unless the process type is renderer
  (`base/allocator/partition_alloc_support.cc:1404-1406`) — don't bother with
  it in single-process; `ReclaimAll()` covers PA.
- What already exists per platform:
  - Linux/Starboard: backgrounding schedules a CRITICAL purge with a
    **1–4 minute** random delay; freeze purges after **~1 s**. So `OnFreeze`
    additions are largely redundant there.
  - AndroidTV: no lifecycle purge at all; only system-pressure
    `onTrimMemory` (`CobaltActivity.java:506`,
    `MemoryPressureMonitor.java:122,259-263` — BACKGROUND trim maps to
    MODERATE, not CRITICAL).

## Proposal

In `ShellPlatformDelegate::OnConceal()`:

```cc
base::MemoryPressureListener::NotifyMemoryPressure(
    base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL);
partition_alloc::MemoryReclaimer::Instance()->ReclaimAll();
```

Rationale for conceal (not blur): concealed means not visible, so the
foreground-GC downgrade doesn't apply and re-decode cost is deferred to
reveal. Skip the `OnFreeze` duplicate on non-Android (already covered ~1 s in
by `MemoryPurgeManager`); on AndroidTV, wire the same call in whichever path
freeze actually takes there.

Include idea 02's media-pool decommit in the same hook if playback state
allows (gate on audio-only-concealed playback if Cobalt supports it).

## Savings math (corrected framing)

The purge volume is real — up to the decoded-image working set (see idea 06:
effectively up to 128 MB ceiling on Linux today), discardable memory (purges
to zero on CRITICAL, `discardable_shared_memory_manager.cc:553-555`), V8
garbage, PA empty slot spans. But:

- **Linux/Starboard:** the same volume would have been purged 1–4 min later
  anyway — the net-new is the window where a low-memory killer would have
  acted, plus determinism.
- **AndroidTV:** genuinely net-new on lifecycle: 30–80 MB lower concealed RSS
  right when the platform evaluates candidates to kill.

## Validation

- AndroidTV: RSS at 10 s / 60 s / 5 min after conceal, with and without the
  change (expect the 5-min numbers to converge on Linux, diverge on Android).
- Resume-to-interactive latency after conceal→reveal.
- Soak repeated conceal/reveal cycles.

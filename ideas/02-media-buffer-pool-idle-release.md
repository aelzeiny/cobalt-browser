# Enable release of the media decoder-buffer pool on hide/suspend

> **VETTED — corrected.** Three findings changed this spec: (1) the
> on-hidden call sites **already exist** in the renderer — what's missing is
> only the enable, which today is a **web-app-only JS flag**; (2) savings do
> NOT apply while merely *paused* (release requires allocation to drain to
> zero, i.e. SourceBuffer teardown); (3) the "Linux 21 MB startup footprint"
> claim was wrong — Linux allocates the pool on demand, nothing is resident at
> startup. **First action is now a question, not a code change: does the
> YouTube app already set `DecoderBuffer.ReleaseMemoryOnBackground`?** If yes,
> these savings are already captured.

**Estimated savings: 30–160 MB when the app is hidden/concealed/suspended
after playback (i.e., once SourceBuffers are torn down); a smaller
slack-decommit is possible during pause. Zero change to steady playback.**
**Effort: small — make an existing opt-in the default (or confirm the app
already opts in)**

## What the code actually does (verified)

- The Starboard `DecoderBufferAllocator`
  (`media/starboard/decoder_buffer_allocator.cc`) backs all encoded MSE
  buffers, growing in 4 MB units up to the demuxer budget (idea 01).
  The default `DefaultReuseAllocatorStrategy` (line 319-320) **retains freed
  blocks forever** — grown capacity stays resident until explicitly released.
- **Release machinery and lifecycle wiring already exist:** on page-hidden,
  `content/renderer/media/renderer_web_media_player_delegate.cc:241-253`
  already calls `DecommitAllDecommitableBlocks()` and `ReleaseIdleMemory()`
  (gated on `USE_STARBOARD_MEDIA`).
- **But both are no-ops until enabled**, and the enables are reachable only
  via the `window.H5vccSettings.set()` JS API:
  `DecoderBuffer.ReleaseMemoryOnBackground` and
  `DecoderBuffer.EnableConfigurableDecommitStrategy`
  (`third_party/blink/renderer/modules/cobalt/h5vcc_settings/h_5_vcc_settings.cc:190,206`).
  Only the YouTube web app can call this; nothing in Cobalt enables it.
- **When release actually frees:** `ReleaseIdleMemory()` frees only when
  `GetAllocated() == 0` (`decoder_buffer_allocator.cc:100-106`), otherwise it
  defers until the last buffer drains (`Free()`, lines 162-173). Pause keeps
  SourceBuffers populated ⇒ allocation ≠ 0 ⇒ **no release during pause**.
  Allocation reaches zero on watch-page exit / app suspend when the
  `ChunkDemuxer`/SourceBuffers are destroyed.
  `DecommitAllDecommitableBlocks()` (line 109-113) can shed
  capacity-minus-live-allocation slack without full drain — the only lever
  that does anything during pause, and only for slack.
- Platform behavior (verified): **AndroidTV** preallocates eagerly but only
  4 MB initial (`media_is_buffer_pool_allocate_on_demand.cc` = false,
  `media_get_initial_buffer_capacity.cc` = 4 MB). **Linux** is fully
  on-demand (shared impl returns true) — the pool doesn't exist until first
  playback and resets when allocation drains, so there is **no startup cost
  and no post-teardown retention on Linux today**. The retention problem this
  spec fixes is primarily **AndroidTV's** (eager, never-shrinking pool).

## Proposal

1. **Ask the app team / check a live session** whether YouTube already sets
   `DecoderBuffer.ReleaseMemoryOnBackground`. If yes — close this idea as
   already-shipped.
2. If no: make it default-on in Cobalt — set `should_release_idle_memory_`
   default true in `DecoderBufferAllocator` (or add a Cobalt feature flag /
   Starboard feature), rather than waiting on the app's JS call. The
   `[Exposed=Window]` h5vcc API is explicitly a stop-gap, so a native default
   is the durable fix.
3. Optionally call `DecommitAllDecommitableBlocks()` from the conceal
   lifecycle hook (idea 04) for the pause-slack case.

## Savings math (corrected)

- **AndroidTV, app hidden/suspended after a 4K session:** pool capacity
  (up to the 100–160 MB budget) minus zero live allocation → **30–160 MB**,
  contingent on YouTube's suspend actually tearing down SourceBuffers
  (verify: `GetAllocated()` reaching 0 — log it).
- **During pause:** only capacity-vs-live slack, typically small.
- **Linux:** little to gain — already on-demand with drain-time reset.

## Trade-offs / risks

- Re-growing the pool on resume: negligible (a few 4 MB mmaps). The eager
  preallocation on Android exists to avoid fragmentation on weak allocators —
  prefer decommit (keep VA) over full free if fragmentation soak shows issues.

## Validation

- Instrument `GetAllocated()`/capacity across watch → browse → conceal on
  AndroidTV; confirm capacity drops after enable.
- Soak repeated play/stop cycles for fragmentation regressions.

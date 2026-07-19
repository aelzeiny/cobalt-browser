# Clamp the MSE video SourceBuffer budget (biggest single lever)

> **VETTED.** Mechanism confirmed end-to-end on both shipping targets
> (`USE_STARBOARD_MEDIA` on for AndroidTV + Linux,
> `starboard/build/buildflags.gni:22-25`). Two corrections: (1) the runtime
> clamp is a **web-app-only JS API** — Cobalt cannot set it from config; the
> Cobalt-settable switch exists but behaves differently (below). (2) The 170 s
> GC-duration threshold cited before is **dead code** on this path — eviction
> is purely byte-budget based. Savings are a **ceiling** reduction; typical
> steady-state savings depend on YouTube's forward-buffer target and must be
> measured.

**Estimated savings: reduces the 4K ceiling by 40–100 MB (SDR/HDR); typical
steady-state savings TBD by measurement (plausibly large — eviction only runs
at the byte limit, so occupancy trends toward it)**
**Effort: trivial code-wise; requires a decision about which of two knobs, and
possibly YouTube-app cooperation**

## The mechanism (confirmed)

Budgets come from Starboard, not Chromium defaults:

- `starboard/android/shared/media_get_video_buffer_budget.cc:44-52` and
  `starboard/shared/starboard/media/media_get_video_buffer_budget.cc:19-51`:
  **30 MB** ≤1080p, **100 MB** 4K SDR, **160 MB** 4K HDR (200/300 MB 8K caps).
- Flow: `SourceBufferStream` ctor (`media/filters/source_buffer_stream.cc:173-174`)
  → `GetDemuxerStreamVideoMemoryLimit`
  (`media/base/starboard/demuxer_memory_limit_starboard.cc:93-113`)
  → `SbMediaGetVideoBufferBudget`.
- The limit is **not** read once: `UpdateVideoConfig`
  (`source_buffer_stream.cc:1870-1878`) ratchets `memory_limit_` **up** when a
  higher-res config arrives (never back down). So a stream that switches to 4K
  gets the 100/160 MB budget mid-session.
- Eviction (`GarbageCollectIfNeeded`, `source_buffer_stream.cc:792-800`) fires
  only when `ranges_size + newData > memory_limit_` — purely byte-driven.
  (The 170 s `SbMediaGetBufferGarbageCollectionDurationThreshold` has **no
  caller** in the shipping MSE path — don't bother tuning it.)

## Two knobs — mutually exclusive, pick deliberately

1. **h5vcc clamp `Media.VideoBufferSizeClampMb`** — applied as
   `std::min(budget, clamp)` inside `GetDemuxerStreamVideoMemoryLimit`
   (`demuxer_memory_limit_starboard.cc:113`), so it caps both the initial and
   the ratcheted 4K value while still scaling below the clamp.
   **Reality check:** it is exposed only as `window.H5vccSettings.set()` — a
   `[Exposed=Window]` Blink JS API
   (`third_party/blink/renderer/modules/cobalt/h5vcc_settings/h_5_vcc_settings.cc:246-251`,
   `.idl:15-22`, explicitly a "stop-gap until Finch"). **Only the YouTube web
   app can call it.** Nothing in `cobalt/` or `starboard/` sets it today.
2. **`--mse-video-buffer-size-limit-mb` switch** (`media/base/media_switches.cc:194`
   → `source_buffer_state.cc:816-823`) — the knob **Cobalt can set itself**
   at launch. Gotcha: it sets `memory_limit_overridden_`, which **disables the
   clamp and the resolution ratchet entirely**
   (`source_buffer_stream.cc:1860-1869`) — it pins one absolute limit for all
   resolutions. Simple, but 1080p sessions then get the same limit as 4K.
3. **One-line constant change** in the two `media_get_video_buffer_budget.cc`
   files — Cobalt-controlled, keeps per-resolution behavior, needs a rebuild.

Recommendation: option 3 (lower `kVideoBufferBudget4KSdr/Hdr`) for a
Cobalt-owned durable change, or coordinate with the app team on option 1.
Use option 2 only if a single absolute cap is acceptable.

> **Field-trial warning on the clamp value (external research, July 2026).**
> Chrome tried pushing low-end demuxer limits to mid-range Android and
> **reverted from stable** (Dec 2023, crbug 40264947): UMA showed more
> rebuffers and less watch time. The media OWNER's replacement was a
> "medium" limit of **80 MB video / 5 MB audio**, explicitly judged
> "sufficient for 4K60 playback". Chrome's demuxer limit isn't the same
> plumbing as our Starboard budget, but it's the only fleet-scale QoE data
> point available: **start any 4K clamp at ~80 MB** (not the 50–60 MB
> originally floated) and only go lower with rebuffer-ratio data in hand.

## Savings math (corrected framing)

Clamping to 80 MB lowers the 4K ceiling by **20 MB (SDR)** / **80 MB (HDR)**
(a 60 MB clamp saves 40/100 MB but sits below Chrome's validated 4K60 floor —
see the warning above).
Actual steady-state savings equal current occupancy minus the clamp; because
GC only runs at the limit, occupancy trends toward the budget on long
playbacks, but YouTube's own forward-buffer targeting may keep it lower —
**measure occupancy first** (memory-infra media rows, or
`chrome://media-internals` buffered ranges) before crediting the full number.

## Trade-offs / risks

- Smaller forward buffer ⇒ rebuffer risk on flaky networks; at 4K bitrates
  (~15–25 Mbps) a 60 MB budget holds ~20–30 s. Validate against YouTube QoE
  (rebuffer ratio, seek latency) on low-end network profiles; consider a
  gentler HDR clamp (80 MB).

## Validation

- Measure real SourceBuffer occupancy during long 4K SDR/HDR playback (this
  decides how much of the ceiling reduction is real).
- RSS during 4K HDR playback before/after on a 2 GB AndroidTV device.
- YouTube QoE metrics on the clamped build.

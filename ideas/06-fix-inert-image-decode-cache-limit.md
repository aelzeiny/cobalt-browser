# Fix the inert image-decode-cache limit (intended 24 MB, actually 128 MB)

> **VETTED.** The bug is real and fully confirmed: commit `86018a098cc`
> ("Reduce decoded image cache size to 32 mb", May 2025) added the
> `LimitImageDecodeCacheSize` token to the command lines but **never added a
> consumer** — no `BASE_FEATURE` with that name exists; the flag is silently
> ignored on both platforms. However, the savings are **platform-asymmetric**:
> this is a solid **Linux** win and mostly a hygiene/ceiling fix on
> **AndroidTV**, where two other mechanisms already bound decoded-image
> residency (details below).

**Estimated savings: Linux — real ceiling cut 128→24 MB, plausibly tens of MB
steady-state (measure first). AndroidTV — small; already bounded by other
caps. Both platforms: correctness/robustness win.**
**Effort: trivial — one switch default + delete a dead flag token**

## The bug (confirmed)

- The intended cap `--enable-features=LimitImageDecodeCacheSize:mb/24`
  (`cobalt/app/cobalt_switch_defaults_starboard.cc:92`,
  `CommandLineOverrideHelper.java:116`) matches **no feature anywhere in the
  tree** — unknown feature names are ignored without error.
- The live control is the `IS_COBALT` branch in
  `cc/tiles/image_decode_cache_utils.cc:46-60`: default **128 MB**, override
  only via `--decoded-image-working-set-budget-bytes`
  (`cc/base/switches.cc:118`) — which nothing sets. `IS_COBALT` is confirmed
  true on both AndroidTV and Linux configs (`cobalt/build/configs/common.gn:1`).
- Verified flow: `blink layer_tree_settings.cc:532-534` →
  `LayerTreeSettings::decoded_image_working_set_budget_bytes` →
  `GpuImageDecodeCache::max_working_set_bytes_`
  (`cc/trees/layer_tree_host_impl.cc:250-253`,
  `gpu_image_decode_cache.cc:1228,1240`). No other override exists.

## Why realistic savings differ by platform (vetting findings)

1. **Decoded images live in the service-side `ServiceTransferCache`**
   (`use_transfer_cache=true`), whose limit is
   `DiscardableCacheSizeLimit()` (`gpu/command_buffer/service/service_discardable_manager.cc:23-53`):
   - **AndroidTV:** `IS_ANDROID` + low-end-device → **1 MB** — unlocked
     decoded images are already evicted almost immediately.
   - **Linux:** **192 MB** (<4 GB RAM) — looser than the client budget, so
     the 128 MB client budget is the binding cap there.
2. **`--cc-image-cache-limit-items=0` is not "disable the limit"** (the
   comment at `cobalt_switch_defaults_starboard.cc:139` is wrong): in
   `ExceedsCacheLimits()` (`gpu_image_decode_cache.cc:2362-2382`), items=0
   makes eviction fire whenever *any* unreferenced image is cached — i.e.,
   zero retention of unlocked decodes. Resident decoded memory ≈ the
   currently-referenced working set.
3. Also refuted during vetting: `--force-gpu-mem-available-mb=64` does **not**
   bound this cache (that's tile memory; the discardable limit is a separate,
   unset switch `--force-gpu-mem-discardable-limit-mb`,
   `service_utils.cc:238-246`).

Net: on Linux the working set can legally reach 128 MB of simultaneously
referenced decodes during dense home-feed raster; on AndroidTV retention is
already near-zero and only the instantaneous locked set is resident.

## Proposal (unchanged, still worth doing on both platforms)

1. Add `{cc::switches::kDecodedImageWorkingSetBudgetBytes, "25165824"}` to
   `GetCobaltParamSwitchDefaults()` (starboard + tvos) and the equivalent
   switch string in `CommandLineOverrideHelper` (AndroidTV — harmless there,
   caps worst-case locked set).
2. Delete the dead `LimitImageDecodeCacheSize` tokens and the Java test
   asserting them.
3. Lower the hardcoded 128 MB default at `image_decode_cache_utils.cc:48` so
   future targets can't silently regress.

## Validation (now the gating step for the savings claim)

- memory-infra dump on the Linux home feed: `cc/image_decode_cache` row —
  current occupancy decides whether this is a 10 MB or 80 MB win.
- AndroidTV: confirm occupancy is already low (expected), treat the change as
  ceiling insurance.
- After the change: scroll-jank/decode-latency check on the lowest-end SoC.

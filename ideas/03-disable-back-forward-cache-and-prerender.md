# Disable BackForwardCache and Prerender2 (insurance, not measured savings)

> **VETTED — reframed as insurance.** Every mechanism claim held up
> (feature on, cache_size=6, no low-end cull off Android, Shell opts in,
> single-process keeps entries fully resident). But vetting also confirmed
> the app **almost never creates entries**: the splash screen is a separate
> WebContents (not a navigation of the app contents,
> `cobalt/shell/browser/shell.cc:494-503`), deep links arrive via mojo
> (`h5vcc_runtime/deep_link_manager`, not navigations), and SPA route changes
> are same-document. Expected steady-state savings ≈ **0** in a normal
> session; the change is about **capping an unbounded reservoir and removing
> a foot-gun**, at zero cost.

**Estimated savings: ~0 expected; up to 6 × (10–40 MB) ceiling if
cross-document navigations ever occur (error/consent/login redirects, future
app changes). Prerender: 15–40 MB episodic, only if the app ever ships
speculation rules.**
**Effort: trivial — one feature-flag string edit and/or two one-line delegate
changes**

## Confirmed mechanics

- `kBackForwardCache` enabled by default
  (`content/public/common/content_features.cc:114-116`); `kBackForwardCacheSize`
  param gives `GetCacheSize() = 6`
  (`content/browser/renderer_host/back_forward_cache_impl.cc:72-77,698-706`);
  the foreground-size param is 0, so all 6 entries share one pool.
- The low-memory auto-disable (`kBackForwardCacheMemoryControls`) is
  default-enabled **only on Android**; on Starboard/Linux
  `DeviceHasEnoughMemoryForBackForwardCache()` is unconditionally true
  (`content_navigation_policy.cc:20-47`).
- Cobalt opts in: `Shell::IsBackForwardCacheSupported()` returns `true`
  (`cobalt/shell/browser/shell.cc:1006-1008`) and is the actual gate
  (`back_forward_cache_impl.cc:879`). `BackForwardCache` is absent from the
  default `--disable-features` (`cobalt_switch_defaults_starboard.cc:90`).
- Same-site proactive BrowsingInstance swap is on, so same-site
  cross-document navigations *would* create entries — the app just doesn't do
  them today.
- In single-process there is no process teardown to reclaim a cached
  document; an entry is fully resident until evicted.
- Prerender2: `Shell::IsPrerender2Supported()` returns `kEligible`
  (`shell.cc:1010-1014`) but only fires on speculation rules — inert for the
  current app.

## Proposal

1. Add `BackForwardCache` to the default `--disable-features` (both platform
   default files) and/or return `false` from
   `Shell::IsBackForwardCacheSupported()`.
2. Return `PreloadingEligibility::kIneligible` from
   `Shell::IsPrerender2Supported()`.
3. Softer option: disable only `kBackForwardCacheSize` → capacity falls to
   `kDefaultBackForwardCacheSize = 1` (`back_forward_cache_impl.cc:89`).

## Why bother if expected savings are ~0

A single retained YouTube-TV document is 10–40 MB and the reservoir holds 6
with no memory-based culling on the main TV targets. Any future app-side
change (consent redirect, error page, login flow, A/B experiment doing a real
navigation) silently converts this from 0 MB to tens of MB on 1–2 GB devices.
Single-app browsers get no benefit from instant back/forward restore —
disabling is free.

## Validation

- Instrument a real session first (log `BackForwardCacheImpl::StoreEntry` or
  check BackForwardCache UMA via `chrome://histograms` through DevTools) —
  confirm the entry count is actually 0 today; then land as insurance with no
  savings claim attached.

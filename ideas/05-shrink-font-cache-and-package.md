# Shrink the font chunk cache and font package (CJK/emoji)

> **VETTED — significantly re-scoped.** Original claim of 8–20 MB fleet-wide
> was wrong. Applies to the **Linux/Starboard (hermetic) target only**, and the
> resident savings materialize mainly in **CJK/emoji sessions**. AndroidTV: ~0
> (uses Android system fonts, no Cobalt font cache, no bundled fonts —
> `skia/ext/font_utils.cc:71-95`, `skia/BUILD.gn:460-491`,
> `starboard/android/shared/platform_configuration/configuration.gni:36` sets
> `cobalt_font_package = "android_system"` with `copy_font_files = false`).

**Estimated savings (corrected): ~12–20 MB for CJK-locale sessions on
Linux/Starboard; ~0 for Latin sessions and ~0 on AndroidTV. Plus ~18 MB
storage on Linux/Starboard.**
**Effort: trivial–small**

## Where the costs actually are (Linux/Starboard hermetic builds)

Blink's font manager on this target is `SkFontMgr_Cobalt`, selected in
`skia/ext/font_utils.cc:69` under `BUILDFLAG(IS_COBALT_HERMETIC_BUILD)`
(`is_starboard && use_custom_libc`, `cobalt/build/configs/starboard.gni:16-31`).

1. **16 MB local typeface chunk cache.** `SkFontMgr_cobalt.cc:85-88` sizes an
   `SkFileMemoryChunkStreamManager` from
   `CobaltLocalTypefaceCacheSizeInBytes()` — default 16 MB in both paths
   (`cobalt/configuration/configuration.cc:54-58`,
   `starboard/common/configuration_defaults.cc:59`). It caches 32 KB chunks of
   *bundled* font files in malloc'd RAM and degrades gracefully to file I/O
   above the cap (`SkStream_cobalt.h:29-59`). Web fonts (YouTube Sans etc.)
   do NOT live here — they have a separate 4 MB remote cache
   (`CobaltRemoteTypefaceCacheSizeInBytesDefault`). So this cache only fills
   when bundled fallback fonts are shaped — i.e., CJK, emoji, symbols. A
   Latin-locale session touches ~126 KB Roboto files and never approaches
   16 MB.

2. **The `standard` font package bundles full CJK + color emoji**
   (`starboard/content/fonts/font_configuration.gni:18`):
   `NotoSansCJK-Regular.woff2` = **11.1 MB** (4-face collection),
   `NotoColorEmoji.woff2` = **6.8 MB**. Crucially, woff2 is whole-file
   brotli: FreeType (built with Brotli support, mandatory per
   `SkFreeType_cobalt.cc:28-30`) holds a **separate decompressed sfnt** for
   any opened face, on top of the compressed chunks in the cache. So for CJK
   sessions the true residency is chunk-cache + decompressed face + HarfBuzz
   shaping caches — and **only the package lever (not the cache dial) removes
   the decompressed-face cost**.

## Proposal (in order of value)

1. **Wire the existing-but-orphaned purge hook (free, no quality loss).**
   `SkFontMgr_Cobalt::PurgeCaches()` (`SkFontMgr_cobalt.cc:135-137`) calls
   `SkGraphics::PurgeFontCache()` + `PurgeUnusedMemoryChunks()` — and has
   **no caller anywhere in the repo**. Hook it to the memory-pressure signal
   (natural companion to idea 04). Reclaims the cache under pressure without
   any first-paint cost in the steady state.
2. Lower the chunk-cache default 16 → 4–8 MB
   (`configuration.cc:58` / `configuration_defaults.cc:59`). Only pays off in
   CJK/emoji sessions; bounded by the ~11 MB compressed CJK file.
3. Ship `cobalt_font_package = "limited"` where product accepts
   DroidSansFallback CJK quality and no color emoji
   (`variables.gni:33-47`) — this is the lever that removes the decompressed
   CJK face + emoji + shaping residency (~8–15 MB in CJK sessions) and ~18 MB
   of storage. Per-region GN decision.

## Corrected savings math

| Scenario | Chunk cache 16→4 | `limited` package | Combined |
|---|---|---|---|
| AndroidTV (any locale) | 0 | 0 (no bundled fonts) | **0** |
| Linux, Latin locale | ~0 (cache barely fills) | ~0 resident, 18 MB storage | **~0 resident** |
| Linux, CJK locale | ~4–10 MB | ~8–15 MB | **~12–20 MB** |

## Trade-offs / risks

- `limited` visibly degrades CJK quality and drops color emoji — product
  decision per region.
- Smaller chunk cache ⇒ more file reads while shaping CJK; measure text-paint
  latency on slow flash at 4 MB.
- The purge-hook option has neither cost; do it first.

## Validation

- Linux build, ja-JP/ko-KR/zh-CN locale session: RSS and
  `Font.LocalTypefaceCache` (the manager registers under that name) before/after.
- Confirm AndroidTV non-applicability with a memory-infra `FontCaches` dump.

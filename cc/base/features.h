// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_BASE_FEATURES_H_
#define CC_BASE_FEATURES_H_

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "build/build_config.h"
#include "cc/base/base_export.h"

namespace features {

CC_BASE_EXPORT BASE_DECLARE_FEATURE(kAlignSurfaceLayerImplToPixelGrid);
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kSynchronizedScrolling);

// Enables partial raster in ZeroCopyRasterBufferProvider when used with the GPU
// compositor.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kZeroCopyRBPPartialRasterWithGpuCompositor);

// Sets raster tree priority to NEW_CONTENT_TAKES_PRIORITY when performing a
// unified scroll with main-thread repaint reasons.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kMainRepaintScrollPrefersNewContent);

// When enabled, the scheduler will allow deferring impl invalidation frames
// for N frames (default 1) to reduce contention with main frames, allowing
// main a chance to commit.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kDeferImplInvalidation);
CC_BASE_EXPORT extern const base::FeatureParam<int>
    kDeferImplInvalidationFrames;

// Use DMSAA instead of MSAA for rastering tiles.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kUseDMSAAForTiles);

// When LayerTreeHostImpl::ReclaimResources() is called in background, trigger a
// additional delayed flush to reclaim resources.
//
// Enabled 03/2024, kept to run a holdback experiment.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kReclaimResourcesDelayedFlushInBackground);

// Use 4x MSAA (vs 8) on High DPI screens.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kDetectHiDpiForMsaa);

// When no frames are produced in a certain time interval, reclaim prepaint
// tiles.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kReclaimPrepaintTilesWhenIdle);

// Feature to reduce the area in which invisible tiles are kept around.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kSmallerInterestArea);

constexpr static int kDefaultInterestAreaSizeInPixels = 3000;
constexpr static int kDefaultInterestAreaSizeInPixelsWhenEnabled = 500;
CC_BASE_EXPORT extern const base::FeatureParam<int> kInterestAreaSizeInPixels;

// When enabled, old prepaint tiles in the "eventually" region get reclaimed
// after some time.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kReclaimOldPrepaintTiles);
CC_BASE_EXPORT extern const base::FeatureParam<int> kReclaimDelayInSeconds;

// Kill switch for using MapRect() to compute filter pixel movement.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kUseMapRectForPixelMovement);

// When enabled, we will not schedule drawing for viz::Surfaces that have been
// evicted. Instead waiting for an ActiveTree that is defining a newer
// viz::Surface.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kEvictionThrottlesDraw);

// When a LayerTreeHostImpl is not visible, clear its transferable resources
// that haven't been imported into viz.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kClearCanvasResourcesInBackground);

// Currently CC Metrics does a lot of calculations for UMA and Tracing. While
// Traces themselves won't run when we are not tracing, some of the calculation
// work is done regardless. When enabled this feature reduces extra calculation
// to when tracing is enabled.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kMetricsTracingCalculationReduction);

// Currently there is a race between OnBeginFrames from the GPU process and
// input arriving from the Browser process. Due to this we can start to produce
// a frame while scrolling without any input events. Late arriving events are
// then enqueued for the next VSync.
//
// When this feature is enabled we will use the corresponding mode definted by
// `kScrollEventDispatchModeParamName`.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kWaitForLateScrollEvents);
CC_BASE_EXPORT extern const base::FeatureParam<double>
    kWaitForLateScrollEventsDeadlineRatio;

// When enabled we stop always pushing PictureLayerImpl properties on
// tree Activation. See crbug.com/40335690.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kDontAlwaysPushPictureLayerImpls);

// When enabled, image quality settings will be preserved in the discardable
// image map.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kPreserveDiscardableImageMapQuality);

// Kill switch for a bunch of optimizations for cc-slimming project.
// Please see crbug.com/335450599 for more details.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kCCSlimming);
// Check if the above feature is enabled. For performance purpose.
CC_BASE_EXPORT bool IsCCSlimmingEnabled();

// Modes for `kWaitForLateScrollEvents` changing event dispatch. Where the
// default is to just always enqueue scroll events.
//
// The ideal goal for both
// `kScrollEventDispatchModeNameDispatchScrollEventsImmediately` and
// `kScrollEventDispatchModeDispatchScrollEventsUntilDeadline` is that they will
// wait for `kWaitForLateScrollEventsDeadlineRatio` of the frame interval for
// input. During this time the first scroll event will be dispatched
// immediately. Subsequent scroll events will be enqueued. At the deadline we
// will resume frame production and enqueuing input.
//
// `kScrollEventDispatchModeNameDispatchScrollEventsImmediately` relies on
// `cc::Scheduler` to control the deadline. However this is overridden if we are
// waiting for Main-thread content. There are also fragile bugs which currently
// prevent enforcing the deadline if frame production is no longer required.
//
// `kScrollEventDispatchModeNameUseScrollPredictorForEmptyQueue` checks when
// we begin frame production, if the event queue is empty, we will generate a
// new prediction and dispatch a synthetic scroll event.
//
// `kScrollEventDispatchModeUseScrollPredictorForDeadline` will perform the
// same as `kScrollEventDispatchModeDispatchScrollEventsImmediately` until
// the deadline is encountered. Instead of immediately resuming frame
// production, we will first attempt to generate a new prediction to dispatch.
// As in `kScrollEventDispatchModeUseScrollPredictorForEmptyQueue`. After
// which we will resume frame production and enqueuing input.
//
// `kScrollEventDispatchModeDispatchScrollEventsUntilDeadline` relies on
// `blink::InputHandlerProxy` to directly enforce the deadline. This isolates us
// from cc scheduling bugs. Allowing us to no longer dispatch events, even if
// frame production has yet to complete.
CC_BASE_EXPORT extern const base::FeatureParam<std::string>
    kScrollEventDispatchMode;
CC_BASE_EXPORT extern const char
    kScrollEventDispatchModeDispatchScrollEventsImmediately[];
CC_BASE_EXPORT extern const char
    kScrollEventDispatchModeUseScrollPredictorForEmptyQueue[];
CC_BASE_EXPORT extern const char
    kScrollEventDispatchModeUseScrollPredictorForDeadline[];
CC_BASE_EXPORT extern const char
    kScrollEventDispatchModeDispatchScrollEventsUntilDeadline[];

// Enables Viz service-side layer trees for content rendering.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kTreesInViz);

// Enables Viz service-side layer tree animations for content rendering.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kTreeAnimationsInViz);

// When enabled HTMLImageElement::decode() will initiate the decode task right
// away rather than piggy-backing on the next BeginMainFrame.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kSendExplicitDecodeRequestsImmediately);

// When enabled, the CC tree priority will be switched to
// NEW_CONTENT_TAKES_PRIORITY during long scroll that cause checkerboarding.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kNewContentForCheckerboardedScrolls);

// When enabled, LCD text is allowed with some filters and backdrop filters.
// Killswitch M135.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kAllowLCDTextWithFilter);

// When enabled, impl-only scroll animations may execute concurrently.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kMultipleImplOnlyScrollAnimations);
CC_BASE_EXPORT extern bool MultiImplOnlyScrollAnimationsSupported();

// When enabled, for a render surface with fractional translation, we'll try to
// align the texels in the render surface to screen pixels to avoid blurriness
// during compositing.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kRenderSurfacePixelAlignment);

// When enabled, and an image decode is requested by both a tile task and
// explicitly via img.decode(), it will be decoded only once.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kPreventDuplicateImageDecodes);

// When enabled, fix bug where an image decode cache entry last use timestamp is
// initialized to 0 instead of now.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kInitImageDecodeLastUseTime);

// The position affected by the safe area inset bottom will be handled by CC in
// the Render Compositor Thread. The transform metrix y is adjusted for all
// affected nodes.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kDynamicSafeAreaInsetsSupportedByCC);

// On devices with a high refresh rate, whether to throttle main (not impl)
// frame production to 60Hz.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kThrottleMainFrameTo60Hz);

// We only want to test the feature value if the client satisfies an eligibility
// criteria, as testing the value enters the client into an experimental group,
// and we only want the groups (including control) to only contain eligibilie
// clients. This is also used for other feature that want to select from the
// samt pool.
CC_BASE_EXPORT bool IsEligibleForThrottleMainFrameTo60Hz();
CC_BASE_EXPORT void SetIsEligibleForThrottleMainFrameTo60Hz(bool is_eligible);

// A mode of ViewTransition capture that does not display unstyled frame,
// instead displays the properly constructed frame while at the same doing
// capture.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kViewTransitionCaptureAndDisplay);

// When enabled, we save the `EventMetrics` for a scroll, even when the result
// is no damage. So that the termination can be per properly attributed to the
// end of frame production for the given VSync.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kZeroScrollMetricsUpdate);

// When enabled, the view transition capture transform is floored instead of
// rounded and we use the render surface pixel snapping to counteract the blurry
// effect.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kViewTransitionFloorTransform);

// Allow the main thread to throttle the main frame rate.
// Note that the composited animations will not be affected.
// Typically the throttle is triggered with the render-blocking API <link
// rel="expect" blocking="full-frame-rate"/>.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kRenderThrottleFrameRate);
// The throttled frame rate when the main thread requests a throttle.
CC_BASE_EXPORT extern const base::FeatureParam<int>
    kRenderThrottledFrameIntervalHz;

// Adds a fast path to avoid waking up the thread pool when there are no raster
// tasks.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kFastPathNoRaster);

// When enabled, moves the layer tree client's metric export call
// for from beginning of the subsequent frame to the end of the subsequent
// frame.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kExportFrameTimingAfterFrameDone);

// When enabled, internal begin frame source will be used in cc to reduce IPC
// between cc and viz when there were many "did not produce frame" recently,
// and SetAutoNeedsBeginFrame will be called on CompositorFrameSink.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(
    kInternalBeginFrameSourceOnManyDidNotProduceFrame);
CC_BASE_EXPORT extern const base::FeatureParam<int>
    kNumDidNotProduceFrameBeforeInternalBeginFrameSource;

// When enabled, the LayerTreeHost will expect to use layer lists instead of
// layer trees by default; the caller can explicitly opt into enabled or
// disabled if need be to override this.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kUseLayerListsByDefault);

// When enabled, the default programmatic scroll animation curve can be
// overridden with extra params.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kProgrammaticScrollAnimationOverride);
// Extra params to override the programmatic scroll animation.
CC_BASE_EXPORT BASE_DECLARE_FEATURE_PARAM(double, kCubicBezierX1);
CC_BASE_EXPORT BASE_DECLARE_FEATURE_PARAM(double, kCubicBezierY1);
CC_BASE_EXPORT BASE_DECLARE_FEATURE_PARAM(double, kCubicBezierX2);
CC_BASE_EXPORT BASE_DECLARE_FEATURE_PARAM(double, kCubicBezierY2);
CC_BASE_EXPORT BASE_DECLARE_FEATURE_PARAM(base::TimeDelta,
                                          kMaxAnimtionDuration);

#if BUILDFLAG(IS_COBALT)
// When enabled, some or all SDR tiles are rasterized into 16-bit formats
// (RGBA_4444 / BGR_565) instead of a 32-bit format, halving cc tile memory
// for the affected tiles. Intended to be controlled per-device via
// h5vcc.experiments.
//
// Params (set via the h5vcc experiments native-flag syntax, e.g.
// "CobaltLowBitDepthTiles:mode=no-text"; the pipeline registers each param
// in the shared Cobalt field trial under its full "Feature:param" key, and
// the lookups below also accept the plain name for --enable-features use):
//   mode:
//     "all" (default) - every eligible SDR tile is demoted to RGBA_4444.
//     "no-text" - only tiles whose content contains no draw-text ops are
//         demoted; tiles with text keep full precision for crisp antialiased
//         edges (4-bit alpha quantizes text edge coverage).
//     "none" - no 4444 demotion (useful to run the opaque-565 policy alone).
//   gate:
//     Which tiles are protected from demotion (kept at full precision).
//     "opaque" (default) - only tiles from layers with opaque contents may
//         demote. Measured to protect nearly everything on Kabuki (whose
//         composited layers are almost never contents_opaque), keeping ~10%
//         of the all-tiles win.
//     "video" - tiles whose screen rect intersects a surface layer (the
//         video underlay on this port) keep full precision; everything else
//         demotes. This targets the actual hazard - translucent pixels
//         blending over the video plane, where 4-bit alpha bands and the
//         frozen dither pattern reads as static over moving video - without
//         sacrificing pages that have no video at all.
//     "none" - demote everything (the arm that showed video static).
//   allow_non_opaque:
//     Legacy alias: true behaves as gate=none when no explicit gate param
//     is set.
//   opaque_565:
//     When true, tiles from layers with opaque contents use BGR_565 instead
//     (5/6-bit color, no alpha), which bands noticeably less than 4444. Takes
//     precedence over the 4444 mode for those tiles.
//   dither:
//     When true (default), demoted tiles are rasterized at 8888 and
//     downconverted with dithering. Rasterizing directly at low bit depth
//     quantizes gradients with no dithering, which is the artifact that
//     historically made RGBA_4444 tiles unshippable.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kCobaltLowBitDepthTiles);

// Parsed, cached param values for kCobaltLowBitDepthTiles. Safe to call from
// any thread after FeatureList initialization (values are computed once).
struct CC_BASE_EXPORT CobaltLowBitDepthTilesConfig {
  enum class Rgba4444Mode { kNone, kAll, kNoText };
  enum class Gate { kOpaque, kVideoOverlap, kNone };
  bool enabled = false;
  Rgba4444Mode rgba_4444_mode = Rgba4444Mode::kNone;
  Gate gate = Gate::kOpaque;
  bool opaque_565 = false;
  bool dither = true;
};
CC_BASE_EXPORT const CobaltLowBitDepthTilesConfig&
GetCobaltLowBitDepthTilesConfig();

// Generic param lookups for Cobalt experiment features. The h5vcc experiments
// pipeline registers params in the shared Cobalt field trial under their full
// "Feature:param" key; --enable-features associates plain names. These accept
// both, preferring the h5vcc form. Values from h5vcc always arrive
// stringified ("true"/"false" for bools).
CC_BASE_EXPORT std::string GetCobaltFeatureParam(const base::Feature& feature,
                                                 const char* param_name);
CC_BASE_EXPORT bool GetCobaltFeatureParamAsBool(const base::Feature& feature,
                                                const char* param_name,
                                                bool default_value);
CC_BASE_EXPORT int GetCobaltFeatureParamAsInt(const base::Feature& feature,
                                              const char* param_name,
                                              int default_value);

// When enabled, opaque images decode to BGR_565 (2 bytes/px instead of 4) in
// the GPU image decode cache, halving the decoded-pixel bytes in flight —
// the dominant image cost on Cobalt, where nothing decoded is cached and the
// binding ceiling is the global discardable limit. Non-opaque images keep the
// full-precision format (565 has no alpha channel). The downconvert happens
// on the CPU decode path, so dithering works here.
// Param: dither (default true) - dither the N32->565 downconvert.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kCobaltLowBitDepthImages);

struct CC_BASE_EXPORT CobaltLowBitDepthImagesConfig {
  bool enabled = false;
  bool dither = true;
};
CC_BASE_EXPORT const CobaltLowBitDepthImagesConfig&
GetCobaltLowBitDepthImagesConfig();

// When enabled, images drawn with a clipping src_rect (fcrop64 thumbnails,
// storyboard sprite cells) are still decoded at their mip-scaled drawn size
// instead of at full source size. Equivalent to --enable-scaling-clipped-
// images, but reachable through h5vcc experiments. The upstream caveat is
// minor edge color-bleeding on scaled sprite-sheet cells.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kCobaltScaleClippedImages);

// When enabled, overrides how long freed tile backings stay in the
// ResourcePool before expiring (default 5s upstream). Motion holds a
// measured 1x-3.5x (median ~2.3x) of live tile bytes as pool slack, which a
// shorter hold drains sooner at the cost of more reallocation during
// sustained scrolling.
// Param: ms (default 5000) - expiration delay in milliseconds.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kCobaltTilePoolExpiration);

// Returns the tile ResourcePool expiration delay: |default_delay| unless
// kCobaltTilePoolExpiration overrides it.
CC_BASE_EXPORT base::TimeDelta GetCobaltTilePoolExpirationDelay(
    base::TimeDelta default_delay);

// When enabled, clamps the GPU-raster tile size (viewport-width strips by
// default, e.g. 1920x288 at 1080p) via LayerTreeSettings::
// max_gpu_raster_tile_size. Smaller tiles cost more quads but give the
// solid-color-analysis gate and pool eviction finer granularity.
// Params: width, height (pixels; a missing/zero param leaves that axis
// unclamped).
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kCobaltTileSize);

// When enabled, reduces composited-layer creation in blink's compositing
// decisions. Every composited layer tiles independently (full-layer-width
// strips), so speculative and animation-driven layers are a top
// tile-memory driver; collapsing them lands the same pixels in shared
// viewport-width strips.
// Params:
//   ignore_will_change (default true): will-change hints no longer force
//     layers. Kills standing speculative layers that exist while nothing is
//     animating; a later JS-driven animation repaints on the main thread
//     until promoted for another reason.
//   main_thread_animations (default false): active CSS transform/opacity/
//     filter animations no longer force layers either; they fall back to
//     blink's main-thread animation path with a repaint per frame - cheaper
//     standing memory, more raster work while animating.
CC_BASE_EXPORT BASE_DECLARE_FEATURE(kCobaltLimitLayerCompositing);

struct CC_BASE_EXPORT CobaltLimitLayerCompositingConfig {
  bool enabled = false;
  bool ignore_will_change = true;
  bool main_thread_animations = false;
};
CC_BASE_EXPORT const CobaltLimitLayerCompositingConfig&
GetCobaltLimitLayerCompositingConfig();
#endif  // BUILDFLAG(IS_COBALT)

}  // namespace features

#endif  // CC_BASE_FEATURES_H_

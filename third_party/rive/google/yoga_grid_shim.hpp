/*
 * Copyright 2026 Google LLC
 *
 * Temporary compatibility shim for Yoga CSS Grid support in Rive Core.
 * This file stubs out Yoga 3.x Grid APIs that are not yet available in the
 * standard //third_party/yoga (Yoga 1.x / 2.0) package in Google3.
 *
 * TODO(b/rive-upgrade): Delete this file once //third_party/yoga is upgraded to
 * Yoga 3+.
 */

#ifndef THIRD_PARTY_RIVE_GOOGLE_YOGA_GRID_SHIM_HPP_
#define THIRD_PARTY_RIVE_GOOGLE_YOGA_GRID_SHIM_HPP_

#include <cstdint>
#include <vector>

#include "third_party/yoga/yoga/YGEnums.h"
#include "third_party/yoga/yoga/YGNode.h"
#include "third_party/yoga/yoga/YGStyle.h"
#include "third_party/yoga/yoga/Yoga.h"

#ifndef YGDisplayGrid
#define YGDisplayGrid static_cast<YGDisplay>(2)
#endif

#ifndef YGConfigIsMeasuringMinContent
inline bool YGConfigIsMeasuringMinContent(YGConfigRef) {
  return false;
}
#endif

#ifndef YGJustifyAuto
#define YGJustifyAuto static_cast<YGJustify>(0)
#endif

#ifndef YGJustifyStretch
#define YGJustifyStretch static_cast<YGJustify>(6)
#endif

#ifndef YGJustifyStart
#define YGJustifyStart YGJustifyFlexStart
#endif

#ifndef YGJustifyEnd
#define YGJustifyEnd YGJustifyFlexEnd
#endif

#ifndef YGAlignStart
#define YGAlignStart YGAlignFlexStart
#endif

#ifndef YGAlignEnd
#define YGAlignEnd YGAlignFlexEnd
#endif

#ifndef setJustifySelf
#define setJustifySelf(x) justifyContent()
#endif

#ifndef setJustifyItems
#define setJustifyItems(x) justifyContent()
#endif

#ifndef setGridTemplateColumns
#define setGridTemplateColumns(x) justifyContent()
#endif

#ifndef setGridTemplateRows
#define setGridTemplateRows(x) justifyContent()
#endif

#ifndef setGridAutoColumns
#define setGridAutoColumns(x) justifyContent()
#endif

#ifndef setGridAutoRows
#define setGridAutoRows(x) justifyContent()
#endif

#ifndef setGridColumnStart
#define setGridColumnStart(x) justifyContent()
#endif

#ifndef setGridColumnEnd
#define setGridColumnEnd(x) justifyContent()
#endif

#ifndef setGridRowStart
#define setGridRowStart(x) justifyContent()
#endif

#ifndef setGridRowEnd
#define setGridRowEnd(x) justifyContent()
#endif

struct YGStyleSizeLength {
  float value = 0.0f;
  int unit = 0;
  static YGStyleSizeLength points(float v) { return {v, 1}; }
  static YGStyleSizeLength percent(float v) { return {v, 2}; }
  static YGStyleSizeLength stretch(float v) { return {v, 3}; }
  static YGStyleSizeLength ofAuto() { return {0.0f, 0}; }
};

struct YGGridTrackSize {
  float value = 0.0f;
  int type = 0;
  static YGGridTrackSize minmax(const YGStyleSizeLength&,
                                const YGStyleSizeLength&) {
    return {};
  }
  static YGGridTrackSize length(float v) { return {v, 1}; }
  static YGGridTrackSize percent(float v) { return {v, 2}; }
  static YGGridTrackSize fr(float v) { return {v, 3}; }
  static YGGridTrackSize auto_() { return {0.0f, 0}; }
};

struct YGGridLine {
  int line = 0;
  static YGGridLine auto_() { return {0}; }
  static YGGridLine fromInteger(int v) { return {v}; }
  static YGGridLine span(int v) { return {v}; }
};

namespace facebook {
namespace yoga {
using GridTrackList = std::vector<YGGridTrackSize>;
}
}  // namespace facebook

#endif  // THIRD_PARTY_RIVE_GOOGLE_YOGA_GRID_SHIM_HPP_

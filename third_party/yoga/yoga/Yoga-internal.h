/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include <yoga/Yoga.h>

#include "CompactValue.h"

using YGVector = std::vector<YGNodeRef>;

YG_EXTERN_C_BEGIN

void YGNodeCalculateLayoutWithContext(
    YGNodeRef node,
    float availableWidth,
    float availableHeight,
    YGDirection ownerDirection,
    void* layoutContext);

// Deallocates a Yoga Node. Unlike YGNodeFree, does not remove the node from
// its parent or children.
void YGNodeDeallocate(YGNodeRef node);

YG_EXTERN_C_END

namespace facebook {
namespace yoga {

inline bool isUndefined(float value) {
  return std::isnan(value);
}

inline bool isUndefined(double value) {
  return std::isnan(value);
}

void throwLogicalErrorWithMessage(const char* message);

} // namespace yoga
} // namespace facebook

extern const std::array<YGEdge, 4> trailing;
extern const std::array<YGEdge, 4> leading;
extern const YGValue YGValueUndefined;
extern const YGValue YGValueAuto;
extern const YGValue YGValueZero;

struct YGCachedMeasurement {
  float availableWidth;
  float availableHeight;

  float computedWidth;
  float computedHeight;

  // rive: the owner size it was made under, which only a node that reads it
  // (YGNodeReadsOwnerSize) is matched on.
  float ownerWidth;
  float ownerHeight;

  // rive: 8 bits each (a mode is 0-2), so the flags below fit in the space
  // the two enums leave.
  YGMeasureMode widthMeasureMode : 8;
  YGMeasureMode heightMeasureMode : 8;
  // rive: made inside a grid min-content probe. Measure funcs report their
  // content to a probe and clamp to the space offered otherwise
  // (YGConfig::beginMinContentProbe), so an entry only answers a query made in
  // the same state.
  bool probe : 1;
  // rive: a probe's layout of this node rather than a measurement. A probe
  // lays its item out (TrackSizing::measureItemMinContent), and its layouts
  // are cached here among the measurements, not in cachedLayout: a node is
  // laid out under several probe constraints a frame, which one slot would
  // keep evicting, and they would evict the layout after the probe too. A
  // layout and a measurement can size a node differently (a measure pass
  // takes shortcuts a layout doesn't), so each only answers its own kind.
  bool layout : 1;
  // rive: sized something below this node to the space it was offered rather
  // than its content, so it can't stand in for a measurement under other
  // limits.
  bool limitDependent : 1;
  // rive: holds a container with children that measures as nothing in either
  // axis, which a limit that only just holds it would leave nothing past its
  // margins.
  bool emptyContainer : 1;

  YGCachedMeasurement()
      : availableWidth(-1),
        availableHeight(-1),
        computedWidth(-1),
        computedHeight(-1),
        ownerWidth(-1),
        ownerHeight(-1),
        widthMeasureMode(YGMeasureModeUndefined),
        heightMeasureMode(YGMeasureModeUndefined),
        probe(false),
        layout(false),
        limitDependent(false),
        emptyContainer(false) {}

  bool operator==(YGCachedMeasurement measurement) const {
    using namespace facebook;

    bool isEqual = widthMeasureMode == measurement.widthMeasureMode &&
        heightMeasureMode == measurement.heightMeasureMode &&
        probe == measurement.probe && layout == measurement.layout &&
        limitDependent == measurement.limitDependent &&
        emptyContainer == measurement.emptyContainer;

    if (!yoga::isUndefined(availableWidth) ||
        !yoga::isUndefined(measurement.availableWidth)) {
      isEqual = isEqual && availableWidth == measurement.availableWidth;
    }
    if (!yoga::isUndefined(availableHeight) ||
        !yoga::isUndefined(measurement.availableHeight)) {
      isEqual = isEqual && availableHeight == measurement.availableHeight;
    }
    if (!yoga::isUndefined(computedWidth) ||
        !yoga::isUndefined(measurement.computedWidth)) {
      isEqual = isEqual && computedWidth == measurement.computedWidth;
    }
    if (!yoga::isUndefined(computedHeight) ||
        !yoga::isUndefined(measurement.computedHeight)) {
      isEqual = isEqual && computedHeight == measurement.computedHeight;
    }
    if (!yoga::isUndefined(ownerWidth) ||
        !yoga::isUndefined(measurement.ownerWidth)) {
      isEqual = isEqual && ownerWidth == measurement.ownerWidth;
    }
    if (!yoga::isUndefined(ownerHeight) ||
        !yoga::isUndefined(measurement.ownerHeight)) {
      isEqual = isEqual && ownerHeight == measurement.ownerHeight;
    }

    return isEqual;
  }
};

// rive: 24 before the flags and owner size were added; 28 where bitfields of
// different types share a unit (clang, gcc), 32 where they don't (msvc).
static_assert(
    sizeof(YGCachedMeasurement) <= 32,
    "YGCachedMeasurement grew; every node holds at least nine of them");

// This value was chosen based on empirical data:
// 98% of analyzed layouts require less than 8 entries.
#define YG_MAX_CACHED_RESULT_COUNT 8
// rive: see YGLayout::extraCachedMeasurements.
#define YG_MAX_CACHED_RESULT_LIMIT 32

namespace facebook {
namespace yoga {
namespace detail {

template <size_t Size>
class Values {
private:
  std::array<CompactValue, Size> values_;

public:
  Values() = default;
  Values(const Values& other) = default;

  explicit Values(const YGValue& defaultValue) noexcept {
    values_.fill(defaultValue);
  }

  const CompactValue& operator[](size_t i) const noexcept { return values_[i]; }
  CompactValue& operator[](size_t i) noexcept { return values_[i]; }

  template <size_t I>
  YGValue get() const noexcept {
    return std::get<I>(values_);
  }

  template <size_t I>
  void set(YGValue& value) noexcept {
    std::get<I>(values_) = value;
  }

  template <size_t I>
  void set(YGValue&& value) noexcept {
    set<I>(value);
  }

  bool operator==(const Values& other) const noexcept {
    for (size_t i = 0; i < Size; ++i) {
      if (values_[i] != other.values_[i]) {
        return false;
      }
    }
    return true;
  }

  Values& operator=(const Values& other) = default;
};
} // namespace detail
} // namespace yoga
} // namespace facebook

static const float kDefaultFlexGrow = 0.0f;
static const float kDefaultFlexShrink = 0.0f;
static const float kWebDefaultFlexShrink = 1.0f;

extern bool YGFloatsEqual(const float a, const float b);

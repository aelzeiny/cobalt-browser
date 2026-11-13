/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include "BitUtils.h"
#include "YGFloatOptional.h"
#include "Yoga-internal.h"

struct YGLayout {
  std::array<float, 4> position = {};
  std::array<float, 2> dimensions = {{YGUndefined, YGUndefined}};
  std::array<float, 4> margin = {};
  std::array<float, 4> border = {};
  std::array<float, 4> padding = {};

private:
  static constexpr size_t directionOffset = 0;
  static constexpr size_t hadOverflowOffset =
      directionOffset + facebook::yoga::detail::bitWidthFn<YGDirection>();
  uint8_t flags = 0;

public:
  // rive: YGNodeContentFlags' answer for this node, kept until it's dirtied.
  uint8_t contentFlags = 0;
  // rive: YGNodeReadsOwnerSize's answer for this node, kept the same way.
  uint8_t ownerSizeFlags = 0;
  uint32_t computedFlexBasisGeneration = 0;
  YGFloatOptional computedFlexBasis = {};

  // Instead of recomputing the entire layout every single time, we cache some
  // information to break early when nothing changed
  uint32_t generationCount = 0;
  YGDirection lastOwnerDirection = YGDirectionInherit;

  uint32_t nextCachedMeasurementsIndex = 0;
  std::array<YGCachedMeasurement, YG_MAX_CACHED_RESULT_COUNT>
      cachedMeasurements = {};
  // rive: more measurement entries, only for a node that fills
  // cachedMeasurements and would start overwriting them. A grid item is
  // probed under several constraints a frame, and nested grids multiply them,
  // so a node inside one can be asked more distinct questions than eight
  // between changes; wrapping would then evict each answer before it is asked
  // again. Grows by YG_MAX_CACHED_RESULT_COUNT up to
  // YG_MAX_CACHED_RESULT_LIMIT, past which entries wrap as before. Empty, and
  // without a heap allocation, everywhere else.
  std::vector<YGCachedMeasurement> extraCachedMeasurements = {};

  uint32_t cachedMeasurementCapacity() const {
    return YG_MAX_CACHED_RESULT_COUNT +
        static_cast<uint32_t>(extraCachedMeasurements.size());
  }
  YGCachedMeasurement& cachedMeasurement(uint32_t index) {
    return index < YG_MAX_CACHED_RESULT_COUNT
        ? cachedMeasurements[index]
        : extraCachedMeasurements[index - YG_MAX_CACHED_RESULT_COUNT];
  }
  std::array<float, 2> measuredDimensions = {{YGUndefined, YGUndefined}};

  YGCachedMeasurement cachedLayout = YGCachedMeasurement();

  // Grid line positions (column/row) in this node's local content space,
  // matching the coordinate space of child layout positions. Populated only
  // for grid nodes during a performLayout pass; empty (no heap) otherwise.
  // Exposes the cell/track geometry the layout engine otherwise discards, so
  // tooling can hit-test a point to a grid cell.
  std::vector<float> gridColumnLineOffsets = {};
  std::vector<float> gridRowLineOffsets = {};
  // rive: the gaps between those lines' tracks, as content distribution
  // spaced them.
  float gridColumnGap = 0.0f;
  float gridRowGap = 0.0f;

  YGDirection direction() const {
    return facebook::yoga::detail::getEnumData<YGDirection>(
        flags, directionOffset);
  }

  void setDirection(YGDirection direction) {
    facebook::yoga::detail::setEnumData<YGDirection>(
        flags, directionOffset, direction);
  }

  bool hadOverflow() const {
    return facebook::yoga::detail::getBooleanData(flags, hadOverflowOffset);
  }
  void setHadOverflow(bool hadOverflow) {
    facebook::yoga::detail::setBooleanData(
        flags, hadOverflowOffset, hadOverflow);
  }

  bool operator==(YGLayout layout) const;
  bool operator!=(YGLayout layout) const { return !(*this == layout); }
};

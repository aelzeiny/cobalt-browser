// Copyright 2026 The Cobalt Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/rive/layout_rive.h"

#include "third_party/blink/renderer/core/html/rive/html_rive_element.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context_state_saver.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"

namespace blink {

LayoutRive::LayoutRive(HTMLRiveElement* element)
    : LayoutReplaced(element),
      natural_size_(LayoutUnit(kDefaultWidth), LayoutUnit(kDefaultHeight)) {
  IntrinsicSizeChanged();
}

void LayoutRive::IntrinsicSizeChanged() {
  NOT_DESTROYED();
  auto* element = static_cast<HTMLRiveElement*>(GetNode());
  gfx::SizeF artboard_size = element ? element->ArtboardSize()
                                     : gfx::SizeF(kDefaultWidth, kDefaultHeight);
  PhysicalSize zoomed_size(LayoutUnit(artboard_size.width()),
                           LayoutUnit(artboard_size.height()));
  if (Style()) {
    zoomed_size.Scale(StyleRef().EffectiveZoom());
  }

  if (zoomed_size == natural_size_) {
    return;
  }

  natural_size_ = zoomed_size;

  if (!Parent()) {
    return;
  }

  SetIntrinsicLogicalWidthsDirty();
  SetNeedsLayoutAndFullPaintInvalidation(
      layout_invalidation_reason::kSizeChanged);
}

PhysicalNaturalSizingInfo LayoutRive::GetNaturalDimensions() const {
  NOT_DESTROYED();
  return PhysicalNaturalSizingInfo::MakeFixed(natural_size_);
}

void LayoutRive::PaintReplaced(const PaintInfo& paint_info,
                               const PhysicalOffset& paint_offset) const {
  NOT_DESTROYED();
  if (DrawingRecorder::UseCachedDrawingIfPossible(paint_info.context, *this,
                                                  paint_info.phase)) {
    return;
  }

  PhysicalRect content_rect = ReplacedContentRect();
  content_rect.Move(paint_offset);
  gfx::Rect snapped_rect = ToPixelSnappedRect(content_rect);
  if (snapped_rect.IsEmpty()) {
    return;
  }

  DrawingRecorder recorder(paint_info.context, *this, paint_info.phase,
                           snapped_rect);

  auto* element = static_cast<HTMLRiveElement*>(GetNode());
  if (!element) {
    return;
  }

  GraphicsContextStateSaver state_saver(paint_info.context);
  paint_info.context.Clip(gfx::RectF(snapped_rect));
  paint_info.context.Translate(snapped_rect.x(), snapped_rect.y());
  element->PaintRive(paint_info.context.Canvas(),
                     gfx::SizeF(snapped_rect.size()));
}

}  // namespace blink

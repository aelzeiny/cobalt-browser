// Copyright 2026 The Cobalt Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_RIVE_LAYOUT_RIVE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_RIVE_LAYOUT_RIVE_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/layout/layout_replaced.h"
#include "third_party/blink/renderer/core/layout/natural_sizing_info.h"

namespace blink {

class HTMLRiveElement;

class CORE_EXPORT LayoutRive final : public LayoutReplaced {
 public:
  explicit LayoutRive(HTMLRiveElement* element);
  ~LayoutRive() override = default;

  void IntrinsicSizeChanged();

  const char* GetName() const override {
    NOT_DESTROYED();
    return "LayoutRive";
  }

 private:
  void PaintReplaced(const PaintInfo& paint_info,
                     const PhysicalOffset& paint_offset) const override;
  void NaturalSizeChanged() override {
    NOT_DESTROYED();
    IntrinsicSizeChanged();
  }
  PhysicalNaturalSizingInfo GetNaturalDimensions() const override;

  PhysicalSize natural_size_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_RIVE_LAYOUT_RIVE_H_

// Copyright 2026 The Cobalt Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_RIVE_RIVE_PAINT_CANVAS_RENDERER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_RIVE_RIVE_PAINT_CANVAS_RENDERER_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "cc/paint/paint_canvas.h"
#include "cc/paint/paint_flags.h"
#include "cc/paint/paint_image.h"
#include "cc/paint/paint_shader.h"
#include "third_party/blink/renderer/core/core_export.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextra-semi"
#pragma clang diagnostic ignored "-Wignored-qualifiers"
#pragma clang diagnostic ignored "-Wshadow"
#include "third_party/rive/include/rive/factory.hpp"
#include "third_party/rive/include/rive/math/raw_path.hpp"
#include "third_party/rive/include/rive/renderer.hpp"
#pragma clang diagnostic pop
#include "third_party/skia/include/core/SkPath.h"

namespace blink {

class CORE_EXPORT RivePaintCanvasFactory final : public rive::Factory {
 public:
  static RivePaintCanvasFactory& Instance();

  RivePaintCanvasFactory() = default;
  ~RivePaintCanvasFactory() override = default;

  rive::rcp<rive::RenderBuffer> makeRenderBuffer(rive::RenderBufferType type,
                                                 rive::RenderBufferFlags flags,
                                                 size_t size_in_bytes) override;

  rive::rcp<rive::RenderShader> makeLinearGradient(
      float sx,
      float sy,
      float ex,
      float ey,
      const rive::ColorInt colors[],
      const float stops[],
      size_t count) override;

  rive::rcp<rive::RenderShader> makeRadialGradient(
      float cx,
      float cy,
      float radius,
      const rive::ColorInt colors[],
      const float stops[],
      size_t count) override;

  rive::rcp<rive::RenderPath> makeRenderPath(rive::RawPath& raw_path,
                                             rive::FillRule fill_rule) override;

  rive::rcp<rive::RenderPath> makeEmptyRenderPath() override;

  rive::rcp<rive::RenderPaint> makeRenderPaint() override;

  rive::rcp<rive::RenderImage> decodeImage(
      rive::Span<const uint8_t> encoded_bytes) override;
};

// Wraps an already-loaded image (e.g. from Blink's image loader, possibly
// lazily decoded) as a Rive image without decoding it here.
CORE_EXPORT rive::rcp<rive::RenderImage> MakeRiveRenderImage(
    cc::PaintImage paint_image);

class CORE_EXPORT RivePaintCanvasRenderer final : public rive::Renderer {
 public:
  explicit RivePaintCanvasRenderer(cc::PaintCanvas* canvas);
  ~RivePaintCanvasRenderer() override = default;

  void save() override;
  void restore() override;
  void transform(const rive::Mat2D& transform) override;
  void drawPath(rive::RenderPath* path, rive::RenderPaint* paint) override;
  void clipPath(rive::RenderPath* path) override;
  void drawImage(const rive::RenderImage* image,
                 rive::ImageSampler sampler,
                 rive::BlendMode blend_mode,
                 float opacity) override;
  void drawImageMesh(const rive::RenderImage* image,
                     rive::ImageSampler sampler,
                     rive::rcp<rive::RenderBuffer> vertices_f32,
                     rive::rcp<rive::RenderBuffer> uv_coords_f32,
                     rive::rcp<rive::RenderBuffer> indices_u16,
                     uint32_t vertex_count,
                     uint32_t index_count,
                     rive::BlendMode blend_mode,
                     float opacity) override;
  void modulateOpacity(float opacity) override;

 private:
  raw_ptr<cc::PaintCanvas> canvas_;
  std::vector<float> opacity_stack_{1.0f};
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_RIVE_RIVE_PAINT_CANVAS_RENDERER_H_

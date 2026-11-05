// Copyright 2026 The Cobalt Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/rive/rive_paint_canvas_renderer.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/no_destructor.h"
#include "cc/paint/paint_filter.h"
#include "cc/paint/paint_op_buffer.h"
#include "third_party/blink/renderer/platform/graphics/color_behavior.h"
#include "third_party/blink/renderer/platform/image-decoders/image_decoder.h"
#include "third_party/blink/renderer/platform/wtf/shared_buffer.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextra-semi"
#pragma clang diagnostic ignored "-Wignored-qualifiers"
#pragma clang diagnostic ignored "-Wshadow"
#include "third_party/rive/include/utils/factory_utils.hpp"
#pragma clang diagnostic pop
#include "third_party/skia/include/core/SkBlendMode.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkM44.h"
#include "third_party/skia/include/core/SkMatrix.h"
#include "third_party/skia/include/core/SkPathTypes.h"
#include "third_party/skia/include/core/SkPoint.h"
#include "third_party/skia/include/core/SkSamplingOptions.h"
#include "third_party/skia/include/core/SkTileMode.h"

namespace blink {

namespace {

SkPathFillType ToSkPathFillType(rive::FillRule fill_rule) {
  switch (fill_rule) {
    case rive::FillRule::evenOdd:
      return SkPathFillType::kEvenOdd;
    case rive::FillRule::nonZero:
    case rive::FillRule::clockwise:
      return SkPathFillType::kWinding;
  }
  return SkPathFillType::kWinding;
}

SkMatrix ToSkMatrix(const rive::Mat2D& m) {
  return SkMatrix::MakeAll(m[0], m[2], m[4], m[1], m[3], m[5], 0.0f, 0.0f,
                           1.0f);
}

SkM44 ToSkM44(const rive::Mat2D& m) {
  return SkM44(m[0], m[2], 0.0f, m[4],
               m[1], m[3], 0.0f, m[5],
               0.0f, 0.0f, 1.0f, 0.0f,
               0.0f, 0.0f, 0.0f, 1.0f);
}

SkBlendMode ToSkBlendMode(rive::BlendMode mode) {
  switch (mode) {
    case rive::BlendMode::srcOver:
      return SkBlendMode::kSrcOver;
    case rive::BlendMode::additive:
      return SkBlendMode::kPlus;
    case rive::BlendMode::screen:
      return SkBlendMode::kScreen;
    case rive::BlendMode::overlay:
      return SkBlendMode::kOverlay;
    case rive::BlendMode::darken:
      return SkBlendMode::kDarken;
    case rive::BlendMode::lighten:
      return SkBlendMode::kLighten;
    case rive::BlendMode::colorDodge:
      return SkBlendMode::kColorDodge;
    case rive::BlendMode::colorBurn:
      return SkBlendMode::kColorBurn;
    case rive::BlendMode::hardLight:
      return SkBlendMode::kHardLight;
    case rive::BlendMode::softLight:
      return SkBlendMode::kSoftLight;
    case rive::BlendMode::difference:
      return SkBlendMode::kDifference;
    case rive::BlendMode::exclusion:
      return SkBlendMode::kExclusion;
    case rive::BlendMode::multiply:
      return SkBlendMode::kMultiply;
    case rive::BlendMode::hue:
      return SkBlendMode::kHue;
    case rive::BlendMode::saturation:
      return SkBlendMode::kSaturation;
    case rive::BlendMode::color:
      return SkBlendMode::kColor;
    case rive::BlendMode::luminosity:
      return SkBlendMode::kLuminosity;
  }
  return SkBlendMode::kSrcOver;
}

cc::PaintFlags::Cap ToPaintCap(rive::StrokeCap cap) {
  switch (cap) {
    case rive::StrokeCap::butt:
      return cc::PaintFlags::kButt_Cap;
    case rive::StrokeCap::round:
      return cc::PaintFlags::kRound_Cap;
    case rive::StrokeCap::square:
      return cc::PaintFlags::kSquare_Cap;
  }
  return cc::PaintFlags::kButt_Cap;
}

cc::PaintFlags::Join ToPaintJoin(rive::StrokeJoin join) {
  switch (join) {
    case rive::StrokeJoin::miter:
      return cc::PaintFlags::kMiter_Join;
    case rive::StrokeJoin::round:
      return cc::PaintFlags::kRound_Join;
    case rive::StrokeJoin::bevel:
      return cc::PaintFlags::kBevel_Join;
  }
  return cc::PaintFlags::kMiter_Join;
}

class RivePaintCanvasRenderPath
    : public rive::LITE_RTTI_OVERRIDE(rive::RenderPath,
                                      RivePaintCanvasRenderPath) {
 public:
  RivePaintCanvasRenderPath() = default;
  RivePaintCanvasRenderPath(rive::RawPath& raw_path, rive::FillRule fill_rule) {
    path_.setFillType(ToSkPathFillType(fill_rule));
    addRawPath(raw_path);
  }

  void rewind() override { path_.rewind(); }

  void fillRule(rive::FillRule value) override {
    path_.setFillType(ToSkPathFillType(value));
  }

  void moveTo(float x, float y) override { path_.moveTo(x, y); }
  void lineTo(float x, float y) override { path_.lineTo(x, y); }
  void cubicTo(float ox,
               float oy,
               float ix,
               float iy,
               float x,
               float y) override {
    path_.cubicTo(ox, oy, ix, iy, x, y);
  }
  void close() override { path_.close(); }

  void addRenderPath(const rive::RenderPath* path,
                     const rive::Mat2D& transform) override {
    LITE_RTTI_CAST_OR_RETURN(sk_path, const RivePaintCanvasRenderPath*, path);
    path_.addPath(sk_path->path_, ToSkMatrix(transform));
  }

  void addRawPath(const rive::RawPath& raw_path) override {
    for (auto [verb, pts] : raw_path) {
      switch (verb) {
        case rive::PathVerb::move:
          path_.moveTo(pts[0].x, pts[0].y);
          break;
        case rive::PathVerb::line:
          path_.lineTo(pts[1].x, pts[1].y);
          break;
        case rive::PathVerb::quad:
          path_.quadTo(pts[1].x, pts[1].y, pts[2].x, pts[2].y);
          break;
        case rive::PathVerb::cubic:
          path_.cubicTo(pts[1].x, pts[1].y, pts[2].x, pts[2].y, pts[3].x,
                        pts[3].y);
          break;
        case rive::PathVerb::close:
          path_.close();
          break;
      }
    }
  }

  const SkPath& sk_path() const { return path_; }

 private:
  SkPath path_;
};

class RivePaintCanvasRenderShader
    : public rive::LITE_RTTI_OVERRIDE(rive::RenderShader,
                                      RivePaintCanvasRenderShader) {
 public:
  explicit RivePaintCanvasRenderShader(sk_sp<cc::PaintShader> shader)
      : shader_(std::move(shader)) {}

  sk_sp<cc::PaintShader> shader() const { return shader_; }

 private:
  sk_sp<cc::PaintShader> shader_;
};

class RivePaintCanvasRenderImage
    : public rive::LITE_RTTI_OVERRIDE(rive::RenderImage,
                                      RivePaintCanvasRenderImage) {
 public:
  explicit RivePaintCanvasRenderImage(cc::PaintImage paint_image)
      : paint_image_(std::move(paint_image)) {
    m_Width = paint_image_.width();
    m_Height = paint_image_.height();
  }

  const cc::PaintImage& paint_image() const { return paint_image_; }

 private:
  cc::PaintImage paint_image_;
};

class RivePaintCanvasRenderPaint
    : public rive::LITE_RTTI_OVERRIDE(rive::RenderPaint,
                                      RivePaintCanvasRenderPaint) {
 public:
  RivePaintCanvasRenderPaint() {
    flags_.setAntiAlias(true);
  }

  void style(rive::RenderPaintStyle style) override {
    flags_.setStyle(style == rive::RenderPaintStyle::fill
                        ? cc::PaintFlags::kFill_Style
                        : cc::PaintFlags::kStroke_Style);
  }

  void color(rive::ColorInt value) override {
    flags_.setColor(static_cast<SkColor>(value));
  }

  void thickness(float value) override { flags_.setStrokeWidth(value); }

  void join(rive::StrokeJoin value) override {
    flags_.setStrokeJoin(ToPaintJoin(value));
  }

  void cap(rive::StrokeCap value) override {
    flags_.setStrokeCap(ToPaintCap(value));
  }

  void feather(float value) override {
    if (value > 0.0f) {
      const float sigma = value * 0.5f;
      flags_.setImageFilter(sk_make_sp<cc::BlurPaintFilter>(
          sigma, sigma, SkTileMode::kDecal, nullptr));
    } else {
      flags_.setImageFilter(nullptr);
    }
  }

  void blendMode(rive::BlendMode value) override {
    flags_.setBlendMode(ToSkBlendMode(value));
  }

  void shader(rive::rcp<rive::RenderShader> shader) override {
    if (!shader) {
      flags_.setShader(nullptr);
      return;
    }
    LITE_RTTI_CAST_OR_RETURN(sk_shader, const RivePaintCanvasRenderShader*,
                             shader.get());
    flags_.setShader(sk_shader->shader());
  }

  void shaderTransform(const rive::Mat2D& transform) override {}

  void invalidateStroke() override {}

  const cc::PaintFlags& flags() const { return flags_; }

 private:
  cc::PaintFlags flags_;
};

}  // namespace

// static
RivePaintCanvasFactory& RivePaintCanvasFactory::Instance() {
  static base::NoDestructor<RivePaintCanvasFactory> instance;
  return *instance;
}

rive::rcp<rive::RenderBuffer> RivePaintCanvasFactory::makeRenderBuffer(
    rive::RenderBufferType type,
    rive::RenderBufferFlags flags,
    size_t size_in_bytes) {
  return rive::make_rcp<rive::DataRenderBuffer>(type, flags, size_in_bytes);
}

rive::rcp<rive::RenderShader> RivePaintCanvasFactory::makeLinearGradient(
    float sx,
    float sy,
    float ex,
    float ey,
    const rive::ColorInt colors[],
    const float stops[],
    size_t count) {
  const SkPoint pts[2] = {SkPoint::Make(sx, sy), SkPoint::Make(ex, ey)};
  std::vector<SkColor4f> sk_colors(count);
  for (size_t i = 0; i < count; ++i) {
    sk_colors[i] = SkColor4f::FromColor(static_cast<SkColor>(colors[i]));
  }
  return rive::make_rcp<RivePaintCanvasRenderShader>(
      cc::PaintShader::MakeLinearGradient(
          pts, sk_colors.data(), stops, static_cast<int>(count),
          SkTileMode::kClamp));
}

rive::rcp<rive::RenderShader> RivePaintCanvasFactory::makeRadialGradient(
    float cx,
    float cy,
    float radius,
    const rive::ColorInt colors[],
    const float stops[],
    size_t count) {
  std::vector<SkColor4f> sk_colors(count);
  for (size_t i = 0; i < count; ++i) {
    sk_colors[i] = SkColor4f::FromColor(static_cast<SkColor>(colors[i]));
  }
  return rive::make_rcp<RivePaintCanvasRenderShader>(
      cc::PaintShader::MakeRadialGradient(
          SkPoint::Make(cx, cy), radius, sk_colors.data(), stops,
          static_cast<int>(count), SkTileMode::kClamp));
}

rive::rcp<rive::RenderPath> RivePaintCanvasFactory::makeRenderPath(
    rive::RawPath& raw_path,
    rive::FillRule fill_rule) {
  return rive::make_rcp<RivePaintCanvasRenderPath>(raw_path, fill_rule);
}

rive::rcp<rive::RenderPath> RivePaintCanvasFactory::makeEmptyRenderPath() {
  return rive::make_rcp<RivePaintCanvasRenderPath>();
}

rive::rcp<rive::RenderPaint> RivePaintCanvasFactory::makeRenderPaint() {
  return rive::make_rcp<RivePaintCanvasRenderPaint>();
}

rive::rcp<rive::RenderImage> RivePaintCanvasFactory::decodeImage(
    rive::Span<const uint8_t> encoded_bytes) {
  if (encoded_bytes.empty()) {
    return nullptr;
  }
  scoped_refptr<SharedBuffer> buffer = SharedBuffer::Create(
      base::span<const uint8_t>(encoded_bytes.data(), encoded_bytes.size()));
  std::unique_ptr<ImageDecoder> decoder = ImageDecoder::Create(
      std::move(buffer), /*data_complete=*/true,
      ImageDecoder::kAlphaPremultiplied, ImageDecoder::kDefaultBitDepth,
      ColorBehavior::kTag, cc::AuxImage::kDefault,
      ImageDecoder::kNoDecodedImageByteLimit);
  if (!decoder || decoder->FrameCount() == 0) {
    return nullptr;
  }
  ImageFrame* frame = decoder->DecodeFrameBufferAtIndex(0);
  if (!frame || frame->GetStatus() != ImageFrame::kFrameComplete) {
    return nullptr;
  }
  SkBitmap bitmap = frame->Bitmap();
  bitmap.setImmutable();
  return rive::make_rcp<RivePaintCanvasRenderImage>(
      cc::PaintImage::CreateFromBitmap(std::move(bitmap)));
}

RivePaintCanvasRenderer::RivePaintCanvasRenderer(cc::PaintCanvas* canvas)
    : canvas_(canvas) {}

void RivePaintCanvasRenderer::save() {
  canvas_->save();
  opacity_stack_.push_back(opacity_stack_.back());
}

void RivePaintCanvasRenderer::restore() {
  canvas_->restore();
  if (opacity_stack_.size() > 1) {
    opacity_stack_.pop_back();
  }
}

void RivePaintCanvasRenderer::transform(const rive::Mat2D& transform) {
  canvas_->concat(ToSkM44(transform));
}

void RivePaintCanvasRenderer::modulateOpacity(float opacity) {
  opacity_stack_.back() *= std::clamp(opacity, 0.0f, 1.0f);
}

void RivePaintCanvasRenderer::drawPath(rive::RenderPath* path,
                                       rive::RenderPaint* paint) {
  LITE_RTTI_CAST_OR_RETURN(sk_path, const RivePaintCanvasRenderPath*, path);
  LITE_RTTI_CAST_OR_RETURN(sk_paint, const RivePaintCanvasRenderPaint*, paint);

  const float current_opacity = opacity_stack_.back();
  if (current_opacity >= 1.0f) {
    canvas_->drawPath(sk_path->sk_path(), sk_paint->flags());
  } else {
    cc::PaintFlags modulated_flags = sk_paint->flags();
    modulated_flags.setAlphaf(modulated_flags.getAlphaf() * current_opacity);
    canvas_->drawPath(sk_path->sk_path(), modulated_flags);
  }
}

void RivePaintCanvasRenderer::clipPath(rive::RenderPath* path) {
  LITE_RTTI_CAST_OR_RETURN(sk_path, const RivePaintCanvasRenderPath*, path);
  canvas_->clipPath(sk_path->sk_path(), SkClipOp::kIntersect,
                    /*do_anti_alias=*/true);
}

void RivePaintCanvasRenderer::drawImage(const rive::RenderImage* image,
                                        rive::ImageSampler sampler,
                                        rive::BlendMode blend_mode,
                                        float opacity) {
  LITE_RTTI_CAST_OR_RETURN(sk_image, const RivePaintCanvasRenderImage*, image);

  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setFilterQuality(cc::PaintFlags::FilterQuality::kLow);
  flags.setBlendMode(ToSkBlendMode(blend_mode));
  flags.setAlphaf(std::clamp(opacity * opacity_stack_.back(), 0.0f, 1.0f));

  const SkRect rect = SkRect::MakeWH(image->width(), image->height());
  canvas_->drawImageRect(sk_image->paint_image(), rect, rect,
                         SkSamplingOptions(SkFilterMode::kLinear), &flags,
                         SkCanvas::kFast_SrcRectConstraint);
}

void RivePaintCanvasRenderer::drawImageMesh(
    const rive::RenderImage* image,
    rive::ImageSampler sampler,
    rive::rcp<rive::RenderBuffer> vertices_f32,
    rive::rcp<rive::RenderBuffer> uv_coords_f32,
    rive::rcp<rive::RenderBuffer> indices_u16,
    uint32_t vertex_count,
    uint32_t index_count,
    rive::BlendMode blend_mode,
    float opacity) {
  LITE_RTTI_CAST_OR_RETURN(sk_image, const RivePaintCanvasRenderImage*, image);
  LITE_RTTI_CAST_OR_RETURN(verts_buf, const rive::DataRenderBuffer*,
                           vertices_f32.get());
  LITE_RTTI_CAST_OR_RETURN(uvs_buf, const rive::DataRenderBuffer*,
                           uv_coords_f32.get());
  LITE_RTTI_CAST_OR_RETURN(indices_buf, const rive::DataRenderBuffer*,
                           indices_u16.get());

  if (vertex_count == 0 || index_count == 0) {
    return;
  }

  std::vector<SkPoint> vertices(vertex_count);
  std::vector<SkPoint> uvs(vertex_count);
  const float* raw_verts = verts_buf->f32s();
  const float* raw_uvs = uvs_buf->f32s();
  const float img_w = static_cast<float>(image->width());
  const float img_h = static_cast<float>(image->height());

  for (uint32_t i = 0; i < vertex_count; ++i) {
    vertices[i] = SkPoint::Make(raw_verts[i * 2], raw_verts[i * 2 + 1]);
    uvs[i] = SkPoint::Make(raw_uvs[i * 2] * img_w, raw_uvs[i * 2 + 1] * img_h);
  }

  std::vector<uint16_t> indices(indices_buf->u16s(),
                                indices_buf->u16s() + index_count);

  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setFilterQuality(cc::PaintFlags::FilterQuality::kLow);
  flags.setBlendMode(ToSkBlendMode(blend_mode));
  flags.setAlphaf(std::clamp(opacity * opacity_stack_.back(), 0.0f, 1.0f));
  flags.setShader(cc::PaintShader::MakeImage(
      sk_image->paint_image(), SkTileMode::kClamp, SkTileMode::kClamp,
      /*local_matrix=*/nullptr));

  canvas_->drawVertices(
      base::MakeRefCounted<cc::RefCountedBuffer<SkPoint>>(std::move(vertices)),
      base::MakeRefCounted<cc::RefCountedBuffer<SkPoint>>(std::move(uvs)),
      base::MakeRefCounted<cc::RefCountedBuffer<uint16_t>>(std::move(indices)),
      flags);
}

}  // namespace blink

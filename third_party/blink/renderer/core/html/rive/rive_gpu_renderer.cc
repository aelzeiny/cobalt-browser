// Copyright 2026 The Cobalt Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/rive/rive_gpu_renderer.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "components/viz/common/resources/shared_image_format.h"
#include "components/viz/common/resources/transferable_resource.h"
#include "gpu/command_buffer/client/client_shared_image.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/client/shared_image_interface.h"
#include "gpu/command_buffer/common/shared_image_usage.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_graphics_context_3d_provider.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/renderer/core/html/rive/rive_paint_canvas_renderer.h"
#include "third_party/blink/renderer/platform/graphics/gpu/shared_gpu_context.h"
#include "third_party/blink/renderer/platform/graphics/web_graphics_context_3d_provider_wrapper.h"
#include "third_party/blink/renderer/platform/wtf/wtf.h"
#include "third_party/rive/cobalt/rive_renderer_gl.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextra-semi"
#pragma clang diagnostic ignored "-Wignored-qualifiers"
#pragma clang diagnostic ignored "-Wshadow"
#include "third_party/rive/include/rive/factory.hpp"
#include "third_party/rive/include/rive/renderer.hpp"
#pragma clang diagnostic pop

namespace blink {

BASE_FEATURE(kRiveRenderer, "RiveRenderer", base::FEATURE_ENABLED_BY_DEFAULT);

class RiveGpuContext::Impl final : public rive::Factory {
 public:
  Impl(std::unique_ptr<WebGraphicsContext3DProviderWrapper> wrapper,
       std::unique_ptr<cobalt_rive::RendererGL> renderer)
      : wrapper_(std::move(wrapper)), renderer_(std::move(renderer)) {}

  WebGraphicsContext3DProviderWrapper* wrapper() { return wrapper_.get(); }
  gpu::gles2::GLES2Interface* gl() {
    return wrapper_->ContextProvider().ContextGL();
  }
  bool IsLost() { return wrapper_->ContextProvider().IsContextLost(); }
  cobalt_rive::RendererGL* renderer() { return renderer_.get(); }

  rive::rcp<rive::RenderImage> MakeImageFromBitmap(const SkBitmap& bitmap) {
    // Rive Renderer takes tightly packed premultiplied RGBA.
    SkImageInfo info =
        SkImageInfo::Make(bitmap.width(), bitmap.height(),
                          kRGBA_8888_SkColorType, kPremul_SkAlphaType);
    Vector<uint8_t> pixels(
        base::checked_cast<wtf_size_t>(info.computeMinByteSize()));
    if (pixels.empty() ||
        !bitmap.readPixels(info, pixels.data(), info.minRowBytes(), 0, 0)) {
      return nullptr;
    }
    return renderer_->MakeImage(info.width(), info.height(), pixels.data());
  }

  // rive::Factory: everything comes from Rive Renderer, except images, which
  // Blink decodes (Rive's own decoders aren't built).
  rive::rcp<rive::RenderBuffer> makeRenderBuffer(
      rive::RenderBufferType type,
      rive::RenderBufferFlags flags,
      size_t size_in_bytes) override {
    return rive_factory()->makeRenderBuffer(type, flags, size_in_bytes);
  }
  rive::rcp<rive::RenderShader> makeLinearGradient(
      float sx,
      float sy,
      float ex,
      float ey,
      const rive::ColorInt colors[],
      const float stops[],
      size_t count) override {
    return rive_factory()->makeLinearGradient(sx, sy, ex, ey, colors, stops,
                                              count);
  }
  rive::rcp<rive::RenderShader> makeRadialGradient(
      float cx,
      float cy,
      float radius,
      const rive::ColorInt colors[],
      const float stops[],
      size_t count) override {
    return rive_factory()->makeRadialGradient(cx, cy, radius, colors, stops,
                                              count);
  }
  rive::rcp<rive::RenderShader> makeAngularGradient(
      const rive::ColorInt colors[],
      const float stops[],
      size_t count) override {
    return rive_factory()->makeAngularGradient(colors, stops, count);
  }
  rive::rcp<rive::RenderPath> makeRenderPath(
      rive::RawPath& raw_path,
      rive::FillRule fill_rule) override {
    return rive_factory()->makeRenderPath(raw_path, fill_rule);
  }
  rive::rcp<rive::RenderPath> makeEmptyRenderPath() override {
    return rive_factory()->makeEmptyRenderPath();
  }
  rive::rcp<rive::RenderPaint> makeRenderPaint() override {
    return rive_factory()->makeRenderPaint();
  }
  rive::rcp<rive::RenderImage> decodeImage(
      rive::Span<const uint8_t> encoded_bytes) override {
    SkBitmap bitmap = DecodeRiveImage(encoded_bytes);
    return bitmap.drawsNothing() ? nullptr : MakeImageFromBitmap(bitmap);
  }
  bool supportsLayerMask() const override {
    return renderer_->factory()->supportsLayerMask();
  }

 private:
  rive::Factory* rive_factory() { return renderer_->factory(); }

  std::unique_ptr<WebGraphicsContext3DProviderWrapper> wrapper_;
  std::unique_ptr<cobalt_rive::RendererGL> renderer_;
};

RiveGpuContext::RiveGpuContext(std::unique_ptr<Impl> impl)
    : impl_(std::move(impl)) {}

RiveGpuContext::~RiveGpuContext() = default;

// static
RiveGpuContext* RiveGpuContext::Get() {
  DCHECK(IsMainThread());
  if (!base::FeatureList::IsEnabled(kRiveRenderer)) {
    return nullptr;
  }
  static RiveGpuContext* instance = nullptr;
  static bool tried = false;
  if (instance) {
    // A lost context stays lost; players fall back to Skia when they next
    // load. (The context is leaked: images in flight may still use it.)
    return instance->impl_->IsLost() ? nullptr : instance;
  }
  if (tried) {
    return nullptr;
  }
  tried = true;

  if (!SharedGpuContext::IsGpuCompositingEnabled()) {
    LOG(WARNING) << "Rive Renderer: no GPU compositing; using Skia.";
    return nullptr;
  }
  Platform::ContextAttributes attributes;
  attributes.context_type = Platform::kGLES3ContextType;
  Platform::GraphicsInfo graphics_info;
  std::unique_ptr<WebGraphicsContext3DProvider> provider =
      Platform::Current()->CreateOffscreenGraphicsContext3DProvider(
          attributes, WebURL(), &graphics_info);
  if (!provider || !provider->BindToCurrentSequence() ||
      !provider->ContextGL()) {
    LOG(WARNING) << "Rive Renderer: no GLES3 context ("
                 << graphics_info.error_message.Utf8() << "); using Skia.";
    return nullptr;
  }
  auto wrapper = std::make_unique<WebGraphicsContext3DProviderWrapper>(
      std::move(provider));
  std::unique_ptr<cobalt_rive::RendererGL> renderer =
      cobalt_rive::RendererGL::Create(wrapper->ContextProvider().ContextGL());
  if (!renderer) {
    LOG(WARNING) << "Rive Renderer: rejected the GPU context; using Skia.";
    return nullptr;
  }
  LOG(INFO) << "Rive Renderer: " << renderer->Description() << " (GPU: "
            << graphics_info.renderer_info.Utf8() << ")";

  instance = new RiveGpuContext(
      std::make_unique<Impl>(std::move(wrapper), std::move(renderer)));
  return instance;
}

rive::Factory* RiveGpuContext::factory() {
  return impl_.get();
}

rive::rcp<rive::RenderImage> RiveGpuContext::MakeImage(
    const cc::PaintImage& image) {
  sk_sp<SkImage> sk_image = image.GetSwSkImage();
  if (!sk_image) {
    return nullptr;
  }
  SkBitmap bitmap;
  if (!bitmap.tryAllocPixels(SkImageInfo::Make(
          sk_image->width(), sk_image->height(), kRGBA_8888_SkColorType,
          kPremul_SkAlphaType)) ||
      !sk_image->readPixels(nullptr, bitmap.pixmap(), 0, 0)) {
    return nullptr;
  }
  return impl_->MakeImageFromBitmap(bitmap);
}

struct RiveGpuSurface::Buffer {
  ~Buffer() { texture.reset(); }

  scoped_refptr<gpu::ClientSharedImage> shared_image;
  std::unique_ptr<gpu::SharedImageTexture> texture;
  // Where the compositor's last read of the buffer ends.
  gpu::SyncToken release_sync_token;
};

RiveGpuSurface::RiveGpuSurface() = default;

RiveGpuSurface::~RiveGpuSurface() {
  if (layer_) {
    layer_->ClearClient();
  }
}

bool RiveGpuSurface::Draw(const gfx::Size& size,
                          base::FunctionRef<void(rive::Renderer*)> draw) {
  RiveGpuContext* context = RiveGpuContext::Get();
  if (!context || size.IsEmpty()) {
    return false;
  }
  RiveGpuContext::Impl* impl = context->impl_.get();
  gpu::gles2::GLES2Interface* gl = impl->gl();

  if (!layer_) {
    layer_ = cc::TextureLayer::Create(this);
    layer_->SetIsDrawable(true);
    layer_->SetHitTestable(true);
    layer_->SetContentsOpaque(false);
    layer_->SetBlendBackgroundColor(true);
  }
  if (!target_ || size != size_) {
    free_buffers_.clear();
    pending_.reset();
    size_ = size;
    target_ = std::make_unique<cobalt_rive::RendererGL::Target>(size.width(),
                                                                size.height());
  }

  // Reuse the frame the compositor hasn't taken yet, else a returned buffer.
  std::unique_ptr<Buffer> buffer = std::move(pending_);
  if (!buffer && !free_buffers_.empty()) {
    buffer = std::move(free_buffers_.front());
    free_buffers_.EraseAt(0);
  }
  if (!buffer) {
    gpu::SharedImageInterface* sii =
        impl->wrapper()->ContextProvider().SharedImageInterface();
    buffer = std::make_unique<Buffer>();
    buffer->shared_image = sii->CreateSharedImage(
        {viz::SinglePlaneFormat::kRGBA_8888, size,
         gfx::ColorSpace::CreateSRGB(), kTopLeft_GrSurfaceOrigin,
         kPremul_SkAlphaType,
         gpu::SHARED_IMAGE_USAGE_GLES2_READ |
             gpu::SHARED_IMAGE_USAGE_GLES2_WRITE |
             gpu::SHARED_IMAGE_USAGE_DISPLAY_READ,
         "RivePlayer"},
        gpu::kNullSurfaceHandle);
    if (!buffer->shared_image) {
      return false;
    }
    buffer->texture = buffer->shared_image->CreateGLTexture(gl);
    buffer->release_sync_token = buffer->shared_image->creation_sync_token();
  }

  // Waits for the shared image to exist, or for the compositor to be done
  // reading the buffer's previous frame.
  std::unique_ptr<gpu::SharedImageTexture::ScopedAccess> access =
      buffer->texture->BeginAccess(buffer->release_sync_token,
                                   /*readonly=*/false);
  impl->renderer()->Draw(*target_, access->texture_id(),
                         [&](rive::Renderer* renderer) { draw(renderer); });
  gpu::SharedImageTexture::ScopedAccess::EndAccess(std::move(access));
  // The compositor waits on this before reading, so nothing blocks here.
  gl->GenSyncTokenCHROMIUM(buffer->release_sync_token.GetData());

  pending_ = std::move(buffer);
  layer_->SetNeedsDisplay();
  return true;
}

bool RiveGpuSurface::PrepareTransferableResource(
    viz::TransferableResource* out_resource,
    viz::ReleaseCallback* out_release_callback) {
  if (!pending_) {
    return false;
  }
  std::unique_ptr<Buffer> buffer = std::move(pending_);
  *out_resource = viz::TransferableResource::Make(
      buffer->shared_image, viz::TransferableResource::ResourceSource::kCanvas,
      buffer->release_sync_token);
  *out_release_callback =
      base::BindOnce(&RiveGpuSurface::Recycle, weak_factory_.GetWeakPtr(),
                     std::move(buffer));
  return true;
}

void RiveGpuSurface::Recycle(std::unique_ptr<Buffer> buffer,
                             const gpu::SyncToken& sync_token,
                             bool is_lost) {
  if (is_lost || buffer->shared_image->size() != size_) {
    return;
  }
  buffer->release_sync_token = sync_token;
  free_buffers_.push_back(std::move(buffer));
}

}  // namespace blink

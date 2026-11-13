// Copyright 2026 The Cobalt Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_RIVE_RIVE_GPU_RENDERER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_RIVE_RIVE_GPU_RENDERER_H_

#include <memory>

#include "base/feature_list.h"
#include "base/functional/function_ref.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "cc/layers/texture_layer.h"
#include "cc/layers/texture_layer_client.h"
#include "cc/paint/paint_image.h"
#include "components/viz/common/resources/release_callback.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"
#include "third_party/rive/cobalt/rive_renderer_gl.h"
#include "third_party/rive/include/rive/refcnt.hpp"
#include "ui/gfx/geometry/size.h"

namespace rive {
class Factory;
class RenderImage;
class Renderer;
}  // namespace rive

namespace blink {

// Draws <rive-player> with Rive Renderer, Rive's own GPU renderer, instead of
// Skia. Disabling it (--disable-features=RiveRenderer) selects Skia, which is
// also the fallback wherever Rive Renderer can't run.
CORE_EXPORT BASE_DECLARE_FEATURE(kRiveRenderer);

// The GPU context and Rive Renderer state shared by all players on the main
// thread.
class CORE_EXPORT RiveGpuContext {
 public:
  // Null if Rive Renderer is disabled or can't run here (no GPU compositing,
  // no GLES3 context, Rive Renderer rejects the context, or it was lost).
  static RiveGpuContext* Get();

  // Makes the paths, paints and images a Rive file needs to be drawn here.
  rive::Factory* factory();

  // Uploads an image (e.g. one a list item loaded) for drawing here.
  rive::rcp<rive::RenderImage> MakeImage(const cc::PaintImage& image);

 private:
  friend class RiveGpuSurface;
  class Impl;

  explicit RiveGpuContext(std::unique_ptr<Impl> impl);
  ~RiveGpuContext();

  std::unique_ptr<Impl> impl_;
};

// The buffers one player renders into, shown by its own texture layer so the
// compositor uses them directly (no raster copy), like a WebGL canvas.
class CORE_EXPORT RiveGpuSurface : public cc::TextureLayerClient {
 public:
  RiveGpuSurface();
  ~RiveGpuSurface() override;

  // Renders `draw` into a buffer of `size` for the layer to show next;
  // false on failure.
  bool Draw(const gfx::Size& size,
            base::FunctionRef<void(rive::Renderer*)> draw);

  cc::Layer* layer() const { return layer_.get(); }

  // cc::TextureLayerClient:
  bool PrepareTransferableResource(
      viz::TransferableResource* out_resource,
      viz::ReleaseCallback* out_release_callback) override;

  struct Buffer;

 private:
  void Recycle(std::unique_ptr<Buffer> buffer,
               const gpu::SyncToken& sync_token,
               bool is_lost);

  scoped_refptr<cc::TextureLayer> layer_;
  gfx::Size size_;
  std::unique_ptr<cobalt_rive::RendererGL::Target> target_;
  // The latest frame, until the compositor takes it.
  std::unique_ptr<Buffer> pending_;
  // Buffers of `size_` the compositor gave back, oldest first.
  Vector<std::unique_ptr<Buffer>> free_buffers_;
  base::WeakPtrFactory<RiveGpuSurface> weak_factory_{this};
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_RIVE_RIVE_GPU_RENDERER_H_

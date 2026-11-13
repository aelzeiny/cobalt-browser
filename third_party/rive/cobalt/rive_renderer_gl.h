// Copyright 2026 The Cobalt Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_RIVE_COBALT_RIVE_RENDERER_GL_H_
#define THIRD_PARTY_RIVE_COBALT_RIVE_RENDERER_GL_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "rive/refcnt.hpp"

namespace gpu::gles2 {
class GLES2Interface;
}  // namespace gpu::gles2

namespace rive {
class Factory;
class RenderImage;
class Renderer;
namespace gpu {
class RenderContext;
class TextureRenderTargetGL;
}  // namespace gpu
}  // namespace rive

namespace cobalt_rive {

// Rive Renderer's GL backend on a Chromium GPU command-buffer context.
//
// Keeps Rive Renderer's headers (and the GL macros they define) out of its
// users. The context passed to Create() becomes, and must stay, the one
// gles2_c_lib calls go to on this thread: Rive objects free GL resources
// whenever they are released.
class RendererGL {
 public:
  // Null if Rive Renderer can't run on `gl` (it needs GLES 3.0).
  static std::unique_ptr<RendererGL> Create(gpu::gles2::GLES2Interface* gl);
  ~RendererGL();

  RendererGL(const RendererGL&) = delete;
  RendererGL& operator=(const RendererGL&) = delete;

  // Makes the paths, paints, etc. a Rive file needs to be drawn here.
  rive::Factory* factory();

  // GL renderer and the interlock mode Rive Renderer picked.
  std::string Description() const;

  // Points gles2_c_lib, which Rive's GL calls go through, at this renderer's
  // context. Needed before freeing its Rive objects once another RendererGL
  // exists (e.g. after a context loss).
  void MakeCurrent() const;

  // Uploads tightly packed, premultiplied RGBA pixels.
  rive::rcp<rive::RenderImage> MakeImage(uint32_t width,
                                         uint32_t height,
                                         const uint8_t* rgba_premul);

  // Where one player draws; reusable across textures of its size.
  class Target {
   public:
    Target(uint32_t width, uint32_t height);
    ~Target();
    uint32_t width() const;
    uint32_t height() const;

   private:
    friend class RendererGL;
    rive::rcp<rive::gpu::TextureRenderTargetGL> target_;
    // Pixel local storage needs an immutable texture, which SharedImage
    // textures aren't on some platforms (e.g. Android). There we draw here
    // and copy.
    bool checked_texture_ = false;
    uint32_t offscreen_texture_ = 0;
  };

  // Clears `texture` (a GL_TEXTURE_2D of `target`'s size) and draws into it,
  // top row first.
  void Draw(Target& target,
            uint32_t texture,
            const std::function<void(rive::Renderer*)>& draw);

 private:
  RendererGL(gpu::gles2::GLES2Interface* gl,
             std::unique_ptr<rive::gpu::RenderContext> render_context);

  gpu::gles2::GLES2Interface* const gl_;
  std::unique_ptr<rive::gpu::RenderContext> render_context_;
};

}  // namespace cobalt_rive

#endif  // THIRD_PARTY_RIVE_COBALT_RIVE_RENDERER_GL_H_

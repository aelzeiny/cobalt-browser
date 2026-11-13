// Copyright 2026 The Cobalt Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/rive/cobalt/rive_renderer_gl.h"

#include <utility>

#include "gpu/command_buffer/client/gles2_lib.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextra-semi"
#pragma clang diagnostic ignored "-Wignored-qualifiers"
#pragma clang diagnostic ignored "-Wshadow"
#include "rive/math/math_types.hpp"
#include "rive/renderer/gl/render_context_gl_impl.hpp"
#include "rive/renderer/gl/render_target_gl.hpp"
#include "rive/renderer/render_context.hpp"
#include "rive/renderer/rive_render_image.hpp"
#include "rive/renderer/rive_renderer.hpp"
#pragma clang diagnostic pop

namespace cobalt_rive {

namespace {

const char* InterlockModeName(rive::gpu::InterlockMode mode) {
  switch (mode) {
    case rive::gpu::InterlockMode::rasterOrdering:
      return "rasterOrdering";
    case rive::gpu::InterlockMode::atomics:
      return "atomics";
    case rive::gpu::InterlockMode::clockwise:
      return "clockwise";
    case rive::gpu::InterlockMode::clockwiseAtomic:
      return "clockwiseAtomic";
    case rive::gpu::InterlockMode::depthStencil:
      return "depthStencil";
  }
  return "unknown";
}

rive::gpu::RenderContextGLImpl* GLImpl(rive::gpu::RenderContext* context) {
  return context->static_impl_cast<rive::gpu::RenderContextGLImpl>();
}

}  // namespace

// static
std::unique_ptr<RendererGL> RendererGL::Create(
    gpu::gles2::GLES2Interface* gl) {
  static bool initialized = false;
  if (!initialized) {
    gles2::Initialize();
    initialized = true;
  }
  gles2::SetGLContext(gl);
  std::unique_ptr<rive::gpu::RenderContext> render_context =
      rive::gpu::RenderContextGLImpl::MakeContext({});
  if (!render_context) {
    return nullptr;
  }
  return std::unique_ptr<RendererGL>(
      new RendererGL(gl, std::move(render_context)));
}

RendererGL::RendererGL(gpu::gles2::GLES2Interface* gl,
                       std::unique_ptr<rive::gpu::RenderContext> render_context)
    : gl_(gl), render_context_(std::move(render_context)) {}

RendererGL::~RendererGL() {
  gles2::SetGLContext(gl_);
  render_context_.reset();
}

rive::Factory* RendererGL::factory() {
  return render_context_.get();
}

std::string RendererGL::Description() const {
  gles2::SetGLContext(gl_);
  rive::gpu::RenderContext::FrameDescriptor frame;
  frame.renderTargetWidth = frame.renderTargetHeight = 1;
  const GLCapabilities& caps =
      GLImpl(render_context_.get())->capabilities();
  std::string description =
      std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER))) +
      ", " + reinterpret_cast<const char*>(glGetString(GL_VERSION)) +
      "; interlock mode " +
      InterlockModeName(render_context_->selectInterlockMode(frame));
  description += "; ANGLE_shader_pixel_local_storage(_coherent) ";
  description += caps.ANGLE_shader_pixel_local_storage ? "1" : "0";
  description += caps.ANGLE_shader_pixel_local_storage_coherent ? "/1" : "/0";
  description += ", EXT_shader_pixel_local_storage ";
  description += caps.EXT_shader_pixel_local_storage ? "1" : "0";
  description += ", KHR_blend_equation_advanced ";
  description += caps.KHR_blend_equation_advanced ? "1" : "0";
  return description;
}

rive::rcp<rive::RenderImage> RendererGL::MakeImage(uint32_t width,
                                                   uint32_t height,
                                                   const uint8_t* rgba_premul) {
  gles2::SetGLContext(gl_);
  rive::rcp<rive::gpu::Texture> texture =
      GLImpl(render_context_.get())
          ->makeImageTexture(width, height, rive::math::msb(width | height),
                             rive::GPUTextureFormat::rgba32, rgba_premul, 1, 1,
                             /*srgb=*/false, /*generateRemainingMips=*/true);
  return texture ? rive::make_rcp<rive::RiveRenderImage>(std::move(texture))
                 : nullptr;
}

RendererGL::Target::Target(uint32_t width, uint32_t height)
    : target_(rive::make_rcp<rive::gpu::TextureRenderTargetGL>(width,
                                                               height)) {
  // Top row first, like the textures Chromium samples.
  target_->setBottomUp(false);
}

RendererGL::Target::~Target() {
  if (offscreen_texture_) {
    glDeleteTextures(1, &offscreen_texture_);
  }
}

uint32_t RendererGL::Target::width() const {
  return target_->width();
}

uint32_t RendererGL::Target::height() const {
  return target_->height();
}

void RendererGL::Draw(Target& target,
                      uint32_t texture,
                      const std::function<void(rive::Renderer*)>& draw) {
  gles2::SetGLContext(gl_);
  if (!target.checked_texture_) {
    target.checked_texture_ = true;
    GLint immutable = 0;
    gl_->BindTexture(GL_TEXTURE_2D, texture);
    gl_->GetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_IMMUTABLE_FORMAT,
                           &immutable);
    if (!immutable) {
      gl_->GenTextures(1, &target.offscreen_texture_);
      gl_->BindTexture(GL_TEXTURE_2D, target.offscreen_texture_);
      gl_->TexStorage2DEXT(GL_TEXTURE_2D, 1, GL_RGBA8, target.width(),
                           target.height());
    }
    gl_->BindTexture(GL_TEXTURE_2D, 0);
  }
  rive::gpu::RenderContextGLImpl* impl = GLImpl(render_context_.get());
  // Others (e.g. SharedImage access) change GL state on this context too.
  impl->invalidateGLState();
  rive::gpu::RenderContext::FrameDescriptor frame;
  frame.renderTargetWidth = target.width();
  frame.renderTargetHeight = target.height();
  frame.loadAction = rive::gpu::LoadAction::clear;
  frame.clearColor = 0;
  render_context_->beginFrame(frame);
  target.target_->setTargetTexture(
      target.offscreen_texture_ ? target.offscreen_texture_ : texture);
  {
    rive::RiveRenderer renderer(render_context_.get());
    draw(&renderer);
  }
  render_context_->flush({.renderTarget = target.target_.get()});
  impl->unbindGLInternalResources();
  if (target.offscreen_texture_) {
    gl_->CopySubTextureCHROMIUM(target.offscreen_texture_, 0, GL_TEXTURE_2D,
                                texture, 0, 0, 0, 0, 0, target.width(),
                                target.height(), false, false, false);
  }
}

}  // namespace cobalt_rive

/*
 * Copyright 2015 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cobalt/renderer/backend/egl/texture.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <malloc.h>

#include "cobalt/renderer/backend/egl/graphics_context.h"
#include "cobalt/renderer/backend/egl/resource_context.h"
#include "cobalt/renderer/backend/egl/texture_data.h"
#include "cobalt/renderer/backend/egl/texture_data_cpu.h"
#include "cobalt/renderer/backend/egl/utils.h"

namespace cobalt {
namespace renderer {
namespace backend {

TextureEGL::TextureEGL(GraphicsContextEGL* graphics_context,
                       scoped_ptr<TextureDataEGL> texture_source_data,
                       bool bgra_supported)
    : graphics_context_(graphics_context),
      surface_info_(texture_source_data->GetSurfaceInfo()) {
  gl_handle_ =
      texture_source_data->ConvertToTexture(graphics_context_, bgra_supported);
}

TextureEGL::TextureEGL(GraphicsContextEGL* graphics_context,
                       const RawTextureMemoryEGL* data, intptr_t offset,
                       const SurfaceInfo& surface_info, int pitch_in_bytes,
                       bool bgra_supported)
    : graphics_context_(graphics_context), surface_info_(surface_info) {
  gl_handle_ = data->CreateTexture(graphics_context_, offset, surface_info_,
                                   pitch_in_bytes, bgra_supported);
}

TextureEGL::TextureEGL(
    GraphicsContextEGL* graphics_context,
    const scoped_refptr<PBufferRenderTargetEGL>& render_target)
    : graphics_context_(graphics_context) {
  GraphicsContextEGL::ScopedMakeCurrent scoped_make_current(graphics_context_);

  source_render_target_ = render_target;

  // First we create the OpenGL texture object and maintain a handle to it.
  surface_info_ = render_target->GetSurfaceInfo();
  GL_CALL(glGenTextures(1, &gl_handle_));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, gl_handle_));
  SetupInitialTextureParameters();

  // PBuffers should always be RGBA, so we enforce this here.
  GLenum gl_format = SurfaceInfoFormatToGL(surface_info_.format);
  DCHECK_EQ(GL_RGBA, gl_format);

  // This call attaches the EGL PBuffer object to the currently bound OpenGL
  // texture object, effectively allowing the PBO render target to be used
  // as a texture by referencing gl_handle_ from now on.
  EGL_CALL(eglBindTexImage(render_target->display(),
                           render_target->GetSurface(),
                           EGL_BACK_BUFFER));

  GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
}

TextureEGL::~TextureEGL() {
  GraphicsContextEGL::ScopedMakeCurrent scoped_make_current(graphics_context_);

  if (source_render_target_) {
    EGL_CALL(eglReleaseTexImage(source_render_target_->display(),
                                source_render_target_->GetSurface(),
                                EGL_BACK_BUFFER));
  }

  GL_CALL(glDeleteTextures(1, &gl_handle_));
}

const SurfaceInfo& TextureEGL::GetSurfaceInfo() const { return surface_info_; }

}  // namespace backend
}  // namespace renderer
}  // namespace cobalt

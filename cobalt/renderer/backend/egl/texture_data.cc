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

#include "cobalt/renderer/backend/egl/texture_data.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "cobalt/renderer/backend/egl/utils.h"

namespace cobalt {
namespace renderer {
namespace backend {

void SetupInitialTextureParameters() {
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
}

GLenum SurfaceInfoFormatToGL(SurfaceInfo::Format format) {
  switch (format) {
    case SurfaceInfo::kFormatRGBA8:
      return GL_RGBA;
    case SurfaceInfo::kFormatBGRA8:
      return GL_BGRA_EXT;
    case SurfaceInfo::kFormatA8:
      return GL_ALPHA;
    default:
      DLOG(FATAL) << "Unsupported incoming pixel data format.";
  }
  return GL_INVALID_ENUM;
}

}  // namespace backend
}  // namespace renderer
}  // namespace cobalt

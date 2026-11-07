// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_CANVAS_HTMLCANVAS_CANVAS_CONTEXT_CREATION_ATTRIBUTES_HELPERS_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_CANVAS_HTMLCANVAS_CANVAS_CONTEXT_CREATION_ATTRIBUTES_HELPERS_H_

#include "build/build_config.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"

namespace blink {

class CanvasContextCreationAttributesCore;
class CanvasContextCreationAttributesModule;
#if !BUILDFLAG(IS_COBALT)
class CanvasRenderingContext2DSettings;
#endif  // !BUILDFLAG(IS_COBALT)

bool ToCanvasContextCreationAttributes(
    const CanvasContextCreationAttributesModule*,
    CanvasContextCreationAttributesCore& result,
    ExceptionState& exception_state);

#if !BUILDFLAG(IS_COBALT)
CanvasRenderingContext2DSettings* ToCanvasRenderingContext2DSettings(
    const CanvasContextCreationAttributesCore&);
#endif  // !BUILDFLAG(IS_COBALT)

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_CANVAS_HTMLCANVAS_CANVAS_CONTEXT_CREATION_ATTRIBUTES_HELPERS_H_

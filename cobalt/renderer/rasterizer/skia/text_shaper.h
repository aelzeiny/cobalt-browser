/*
 * Copyright 2016 Google Inc. All Rights Reserved.
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

#ifndef COBALT_RENDERER_RASTERIZER_SKIA_TEXT_SHAPER_H_
#define COBALT_RENDERER_RASTERIZER_SKIA_TEXT_SHAPER_H_

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "base/synchronization/lock.h"
#include "cobalt/render_tree/font_provider.h"
#include "cobalt/renderer/rasterizer/skia/font.h"
#include "cobalt/renderer/rasterizer/skia/glyph_buffer.h"

#include "third_party/harfbuzz-ng/src/hb.h"
#include "third_party/harfbuzz-ng/src/hb-icu.h"
#include "third_party/icu/public/common/unicode/uscript.h"
#include "third_party/skia/include/core/SkTextBlob.h"

namespace cobalt {
namespace renderer {
namespace rasterizer {
namespace skia {

// Describes a render_tree::TextShaper using skia and HarfBuzz to shape the
// text.
class TextShaper {
 public:
  // A script run represents a segment of text that can be shaped using a single
  // SkiaFont and UScriptCode combination.
  struct ScriptRun {
    ScriptRun(SkiaFont* run_font, UScriptCode run_script,
              unsigned int run_start_index, unsigned int run_length)
        : font(run_font),
          script(run_script),
          start_index(run_start_index),
          length(run_length) {}

    SkiaFont* font;
    UScriptCode script;
    unsigned int start_index;
    unsigned int length;
  };

  typedef std::vector<ScriptRun> ScriptRuns;

  TextShaper();

  // Shapes a utf-16 text buffer using the given font provider. The shaping
  // can be simple or complex, depending on the text provided.
  // |language| is used during complex shaping by HarfBuzz in order to allow it
  // to make shaping decisions more likely to be correct for the locale.
  // If |is_rtl| is true, then the glyphs in the text buffer will be reversed.
  // Returns a newly created glyph buffer, which can be used to render the
  // shaped text.
  scoped_refptr<SkiaGlyphBuffer> CreateGlyphBuffer(
      const char16* text_buffer, size_t text_length,
      const std::string& language, bool is_rtl,
      render_tree::FontProvider* font_provider);

  // Shapes a utf-8 string using a single font. The shaping can be simple or
  // complex, depending on the text provided.
  // Returns a newly created glyph buffer, which can be used to render the
  // shaped text.
  scoped_refptr<SkiaGlyphBuffer> CreateGlyphBuffer(
      const std::string& utf8_string,
      const scoped_refptr<render_tree::Font>& font);

  // Shapes a utf-16 text buffer using the given font provider. The shaping
  // can be simple or complex, depending on the text provided.  However, a glyph
  // buffer is not created from the shaping data. It is instead only used to
  // generate the width of the data when it is shaped.
  // Returns the width of the shaped text.
  float GetTextWidth(const char16* text_buffer, size_t text_length,
                     const std::string& language, bool is_rtl,
                     render_tree::FontProvider* font_provider,
                     render_tree::FontVector* maybe_used_fonts);

 private:
  // Internal class used for tracking the vertical bounds of a text buffer
  // during shaping when bounds are requested (i.e. the passed in |maybe_bounds|
  // is non-NULL).
  class VerticalBounds {
   public:
    VerticalBounds()
        : min_y_(std::numeric_limits<float>::max()),
          max_y_(std::numeric_limits<float>::min()) {}

    void IncludeRange(float min_y, float max_y) {
      min_y_ = std::min(min_y_, min_y);
      max_y_ = std::max(max_y_, max_y);
    }

    float GetY() const { return IsValid() ? min_y_ : 0; }
    float GetHeight() const { return IsValid() ? max_y_ - min_y_ : 0; }

   private:
    bool IsValid() const { return max_y_ >= min_y_; }

    float min_y_;
    float max_y_;
  };

  // Shape text relying on SkiaFont and HarfBuzz.
  // Returns the width of the shaped text.
  // If |maybe_glyph_buffer| is non-NULL, it is populated with SkiaGlyphBuffer
  // shaping data.
  // If |maybe_bounds| is non-NULL, it is populated with the bounds of the
  // shaped text.
  // If |maybe_used_fonts| is non-NULL, it is populated with all of the fonts
  // used during shaping.
  float ShapeText(const char16* text_buffer, size_t text_length,
                  const std::string& language, bool is_rtl,
                  render_tree::FontProvider* font_provider,
                  SkTextBlobBuilder* maybe_builder, math::RectF* maybe_bounds,
                  render_tree::FontVector* maybe_used_fonts);

  // Populate a ScriptRuns object with all runs of text containing a single
  // SkiaFont and UScriptCode combination.
  // Returns false if the script run collection fails.
  bool CollectScriptRuns(const char16* text_buffer, size_t text_length,
                         render_tree::FontProvider* font_provider,
                         ScriptRuns* runs);

  // Shape a complex text run using HarfBuzz.
  void ShapeComplexRun(const char16* text_buffer, const ScriptRun& script_run,
                       const std::string& language, bool is_rtl,
                       render_tree::FontProvider* font_provider,
                       SkTextBlobBuilder* maybe_builder,
                       VerticalBounds* maybe_vertical_bounds,
                       render_tree::FontVector* maybe_used_fonts,
                       float* current_width);

  // Shape a simple text run. In the case where the direction is RTL, the text
  // will be reversed.
  void ShapeSimpleRunWithDirection(const char16* text_buffer,
                                   size_t text_length, bool is_rtl,
                                   render_tree::FontProvider* font_provider,
                                   SkTextBlobBuilder* maybe_builder,
                                   VerticalBounds* maybe_vertical_bounds,
                                   render_tree::FontVector* maybe_used_fonts,
                                   float* current_width);

  // Shape a simple text run, relying on the SkiaFont objects provided by
  // the FontProvider to determine the shaping data.
  void ShapeSimpleRun(const char16* text_buffer, size_t text_length,
                      render_tree::FontProvider* font_provider,
                      SkTextBlobBuilder* maybe_builder,
                      VerticalBounds* maybe_vertical_bounds,
                      render_tree::FontVector* maybe_used_fonts,
                      float* current_width);

  // Verifies that the glyph arrays have the required size allocated. If they do
  // not, then the arrays are re-allocated with the required size.
  void EnsureLocalGlyphArraysHaveSize(size_t size);
  // Verifies that the local text buffer has the required size allocated. If it
  // does not, then the buffer is re-allocated with the required size.
  void EnsureLocalTextBufferHasSize(size_t size);

  // Lock used during shaping to ensure it does not occur on multiple threads at
  // the same time.
  base::Lock shaping_mutex_;

  // The allocated glyph and positions data. This is retained in between shaping
  // calls to prevent constantly needing to allocate the arrays. In the case
  // where a larger array is needed than the current size, larger arrays are
  // allocated in their place.
  size_t local_glyph_array_size_;
  scoped_array<render_tree::GlyphIndex> local_glyphs_;
  scoped_array<SkScalar> local_positions_;

  // The allocated text buffer used by complex shaping when normalizing the
  // text.
  size_t local_text_buffer_size_;
  scoped_array<char16> local_text_buffer_;
};

}  // namespace skia
}  // namespace rasterizer
}  // namespace renderer
}  // namespace cobalt

#endif  // COBALT_RENDERER_RASTERIZER_SKIA_TEXT_SHAPER_H_

#if defined(WITH_RIVE_TEXT) && !defined(RIVE_NO_CORETEXT)

// #if defined(TARGET_OS_IPHONE) || defined(TARGET_OS_SIMULATOR)
// #else
#include <CoreText/CoreText.h>
#include <CoreText/CTFontManager.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>
// #endif

#include "rive/math/math_types.hpp"
#include "rive/text_engine.hpp"
#include "rive/text/font_hb.hpp"
#include "rive/text/utf.hpp"

#include "hb-coretext.h"
#include "hb-ot.h"

extern "C"
{
#include "third_party/sheenbidi/Headers/SheenBidi.h"
}

constexpr int kStdScale = 2048;
constexpr float gInvScale = 1.0f / kStdScale;

class CoreTextHBFont : public HBFont
{
private:
  CTFontRef m_ctFont;
  bool m_useSystemShaper;
  uint16_t m_weight;
  uint8_t m_width;

public:
  CoreTextHBFont(
      hb_font_t* font,
      CTFontRef ctFont,
      bool useSystemShaper,
      uint16_t weight,
      uint8_t width);

  CoreTextHBFont(hb_font_t* font,
         std::unordered_map<hb_tag_t, float> axisValues,
         std::unordered_map<hb_tag_t, uint32_t> featureValues,
         std::vector<hb_feature_t> hbFeatures,
         CTFontRef ctFont,
         bool useSystemShaper,
         uint16_t weight,
         uint8_t width);
  ~CoreTextHBFont() override;

  void shapeFallbackRun(
      rive::SimpleArrayBuilder<rive::GlyphRun>& gruns,
      const rive::Unichar text[],
      const unsigned textStart,
      const rive::TextRun& textRun,
      const rive::TextRun& originalTextRun,
      const uint32_t fallbackIndex) override;

  rive::RawPath getPath(rive::GlyphID glyph) const override;

  uint16_t getWeight() const override;

  rive::SimpleArray<rive::Paragraph> onShapeText(
    rive::Span<const rive::Unichar> text,
    rive::Span<const rive::TextRun> runs,
    int textDirectionFlag) const override;

  rive::rcp<Font> withOptions(
    rive::Span<const Coord> variableAxes,
    rive::Span<const Feature> features) const override;
};

struct HBCTFaceData
{
  CTFontRef ctFont;
};

static void hb_ct_face_data_destroy(void* userData)
{
  auto data = static_cast<HBCTFaceData*>(userData);
  if (data != nullptr)
  {
    if (data->ctFont != nullptr)
    {
      CFRelease(data->ctFont);
    }
    delete data;
  }
}

static hb_blob_t* hb_ct_reference_table(
    hb_face_t*, hb_tag_t tag, void* userData)
{
  auto data = static_cast<HBCTFaceData*>(userData);
  if (data == nullptr || data->ctFont == nullptr)
  {
    return hb_blob_get_empty();
  }

  CFDataRef table = CTFontCopyTable(
      data->ctFont,
      static_cast<CTFontTableTag>(tag),
      kCTFontTableOptionNoOptions);
  if (table == nullptr)
  {
    return hb_blob_get_empty();
  }

  const char* bytes = reinterpret_cast<const char*>(CFDataGetBytePtr(table));
  const auto length = static_cast<unsigned int>(CFDataGetLength(table));

  return hb_blob_create(
      bytes,
      length,
      HB_MEMORY_MODE_READONLY,
      (void*)table,
      [](void* blobUserData) {
        CFRelease(static_cast<CFDataRef>(blobUserData));
      });
}

static hb_font_t* hb_font_create_from_ct_tables(CTFontRef ctFont)
{
  if (ctFont == nullptr)
  {
    return nullptr;
  }

  auto* faceData = new HBCTFaceData{(CTFontRef)CFRetain(ctFont)};
  hb_face_t* face = hb_face_create_for_tables(
      hb_ct_reference_table, faceData, hb_ct_face_data_destroy);
  if (face == nullptr)
  {
    hb_ct_face_data_destroy(faceData);
    return nullptr;
  }

  const unsigned int upem = (unsigned int)CTFontGetUnitsPerEm(ctFont);
  if (upem > 0)
  {
    hb_face_set_upem(face, upem);
  }

  hb_font_t* font = hb_font_create(face);
  hb_face_destroy(face);
  return font;
}

rive::rcp<rive::Font> HBFont::FromSystem(
    void* systemFont, bool useSystemShaper, uint16_t weight, uint8_t width)
{
  CTFontRef ctFont = (CTFontRef)systemFont;
  bool isCopy = false;
  if (CTFontGetSize(ctFont) != kStdScale)
  {
    // Need the font sized at our magic scale so we can extract normalized
    // path data.
    ctFont = CTFontCreateCopyWithAttributes(
        ctFont, kStdScale, nullptr, nullptr);
    isCopy = true;
  }

  hb_font_t* font = hb_font_create_from_ct_tables(ctFont);
  if (font == nullptr)
  {
    font = hb_coretext_font_create(ctFont);
  }
  if (font)
  {
    auto riveFont = rive::rcp<rive::Font>(
        new CoreTextHBFont(font, ctFont, useSystemShaper, weight, width));
    if (isCopy)
    {
      CFRelease(ctFont);
    }
    return riveFont;
  }
  if (isCopy)
  {
    CFRelease(ctFont);
  }
  return nullptr;
}

template <size_t N, typename T> class AutoSTArray
{
  T m_storage[N];
  T* m_ptr;
  const size_t m_count;

public:
  AutoSTArray(size_t n) : m_count(n)
  {
    m_ptr = m_storage;
    if (n > N)
    {
      m_ptr = new T[n];
    }
  }
  ~AutoSTArray()
  {
    if (m_ptr != m_storage)
    {
      delete[] m_ptr;
    }
  }

  T* data() const { return m_ptr; }

  T& operator[](size_t index)
  {
    assert(index < m_count);
    return m_ptr[index];
  }
};
template <typename T> class AutoCF
{
  T m_obj;

public:
  AutoCF(T obj = nullptr) : m_obj(obj) {}
  AutoCF(const AutoCF& other)
  {
    if (other.m_obj)
    {
      CFRetain(other.m_obj);
    }
    m_obj = other.m_obj;
  }
  AutoCF(AutoCF&& other)
  {
    m_obj = other.m_obj;
    other.m_obj = nullptr;
  }
  ~AutoCF()
  {
    if (m_obj)
    {
      CFRelease(m_obj);
    }
  }

  AutoCF& operator=(const AutoCF& other)
  {
    if (m_obj != other.m_obj)
    {
      if (other.m_obj)
      {
        CFRetain(other.m_obj);
      }
      if (m_obj)
      {
        CFRelease(m_obj);
      }
      m_obj = other.m_obj;
    }
    return *this;
  }

  void reset(T obj)
  {
    if (obj != m_obj)
    {
      if (m_obj)
      {
        CFRelease(m_obj);
      }
      m_obj = obj;
    }
  }

  operator T() const { return m_obj; }
  operator bool() const { return m_obj != nullptr; }
  T get() const { return m_obj; }
};

struct AutoUTF16
{
  std::vector<uint16_t> array;

  AutoUTF16(const rive::Unichar uni[], int count)
  {
    array.reserve(count);
    for (int i = 0; i < count; ++i)
    {
      uint16_t tmp[2];
      int n = rive::UTF::ToUTF16(uni[i], tmp);

      for (int j = 0; j < n; ++j)
      {
        array.push_back(tmp[j]);
      }
    }
  }
};

static void apply_element(void* ctx, const CGPathElement* element)
{
  auto path = (rive::RawPath*)ctx;
  const CGPoint* points = element->points;

  switch (element->type)
  {
    case kCGPathElementMoveToPoint:
      path->moveTo(
          (float)points[0].x * gInvScale, (float)-points[0].y * gInvScale);
      break;

    case kCGPathElementAddLineToPoint:
      path->lineTo(
          (float)points[0].x * gInvScale, (float)-points[0].y * gInvScale);
      break;

    case kCGPathElementAddQuadCurveToPoint:

      path->quadToCubic(
          (float)points[0].x * gInvScale,
          (float)-points[0].y * gInvScale,
          (float)points[1].x * gInvScale,
          (float)-points[1].y * gInvScale);
      break;

    case kCGPathElementAddCurveToPoint:

      path->cubicTo(
          (float)points[0].x * gInvScale,
          (float)-points[0].y * gInvScale,
          (float)points[1].x * gInvScale,
          (float)-points[1].y * gInvScale,
          (float)points[2].x * gInvScale,
          (float)-points[2].y * gInvScale);
      break;

    case kCGPathElementCloseSubpath:
      path->close();
      break;

    default:
      assert(false);
      break;
  }
}

static bool ct_extract_glyph_path(
    CTFontRef ctFont, rive::GlyphID glyph, rive::RawPath* outPath)
{
  if (ctFont == nullptr)
  {
    return false;
  }
  AutoCF<CGPathRef> cgPath = CTFontCreatePathForGlyph(ctFont, glyph, nullptr);
  if (!cgPath)
  {
    return false;
  }
  CGPathApply(cgPath.get(), outPath, apply_element);
  return true;
}

CoreTextHBFont::CoreTextHBFont(
    hb_font_t* font,
    CTFontRef ctFont,
    bool useSystemShaper,
    uint16_t weight,
    uint8_t width)
    : HBFont(font)
    , m_ctFont(ctFont ? (CTFontRef)CFRetain(ctFont) : nullptr)
    , m_useSystemShaper(useSystemShaper)
    , m_weight(weight)
    , m_width(width)
{
  m_axisValues[HB_OT_TAG_VAR_AXIS_WEIGHT] = weight;
  m_axisValues[HB_OT_TAG_VAR_AXIS_WIDTH] = width;

  hb_variation_t variation_data[2];
  variation_data[0].tag = HB_OT_TAG_VAR_AXIS_WEIGHT;
  variation_data[0].value = weight;
  variation_data[1].tag = HB_OT_TAG_VAR_AXIS_WIDTH;
  variation_data[1].value = width;
  hb_font_set_variations(font, variation_data, 2);
}

CoreTextHBFont::CoreTextHBFont(hb_font_t* font,
               std::unordered_map<hb_tag_t, float> axisValues,
               std::unordered_map<hb_tag_t, uint32_t> featureValues,
               std::vector<hb_feature_t> hbFeatures,
               CTFontRef ctFont,
               bool useSystemShaper,
               uint16_t weight,
               uint8_t width) :
  HBFont(font, axisValues, featureValues, hbFeatures),
  m_ctFont(ctFont ? (CTFontRef)CFRetain(ctFont) : nullptr),
  m_useSystemShaper(useSystemShaper),
  m_weight(weight),
  m_width(width)
{
}

CoreTextHBFont::~CoreTextHBFont()
{
  if (m_ctFont)
  {
    CFRelease(m_ctFont);
  }
}

void CoreTextHBFont::shapeFallbackRun(
    rive::SimpleArrayBuilder<rive::GlyphRun>& gruns,
    const rive::Unichar text[],
    const unsigned textStart,
    const rive::TextRun& textRun,
    const rive::TextRun& originalTextRun,
    const uint32_t fallbackIndex)
{
  if (!m_useSystemShaper)
  {
    HBFont::shapeFallbackRun(
        gruns, text, textStart, textRun, originalTextRun, fallbackIndex);
    return;
  }

  CTFontRef ctFont = m_ctFont;

  AutoUTF16 utf16(&text[textStart], textRun.unicharCount);
  CFIndex utf16Length = rive::math::lossless_numeric_cast<CFIndex>(
      utf16.array.size());

  assert(utf16.array.size() >= textRun.unicharCount);

  // Build mapping from UTF16 to UTF32 offsets
  std::vector<uint32_t> u16ToU32(utf16Length, 0);
  uint32_t u16_idx = 0;
  for (uint32_t i = 0; i < textRun.unicharCount; ++i)
  {
    if (u16_idx < utf16Length)
    {
      u16ToU32[u16_idx] = i;
    }
    if (text[textStart + i] > 0xFFFF)
    {
      if (u16_idx + 1 < utf16Length)
      {
        u16ToU32[u16_idx + 1] = i;
      }
      u16_idx += 2;
    }
    else
    {
      u16_idx += 1;
    }
  }

  AutoCF<CFStringRef> string = CFStringCreateWithCharactersNoCopy(
      nullptr, utf16.array.data(), utf16Length, kCFAllocatorNull);

  AutoCF<CFMutableDictionaryRef> attr = CFDictionaryCreateMutable(
      kCFAllocatorDefault,
      0,
      &kCFTypeDictionaryKeyCallBacks,
      &kCFTypeDictionaryValueCallBacks);
  CFDictionaryAddValue(attr.get(), kCTFontAttributeName, ctFont);

  AutoCF<CFMutableAttributedStringRef>
      attrString = CFAttributedStringCreateMutable(
          kCFAllocatorDefault, utf16Length);
  CFAttributedStringReplaceString(
      attrString.get(), CFRangeMake(0, 0), string.get());
  CFAttributedStringSetAttributes(
      attrString.get(), CFRangeMake(0, utf16Length), attr.get(), false);

  SInt8 forcedLevel = static_cast<SInt8>(textRun.level);
  AutoCF<CFNumberRef> level_number = CFNumberCreate(
      kCFAllocatorDefault, kCFNumberSInt8Type, &forcedLevel);
  const void* optionKeys[] = {kCTTypesetterOptionForcedEmbeddingLevel};
  const void* optionValues[] = {level_number.get()};
  AutoCF<CFDictionaryRef> options = CFDictionaryCreate(
      kCFAllocatorDefault,
      optionKeys,
      optionValues,
      1,
      &kCFTypeDictionaryKeyCallBacks,
      &kCFTypeDictionaryValueCallBacks);

  AutoCF<CTTypesetterRef>
      typesetter = CTTypesetterCreateWithAttributedStringAndOptions(
          attrString.get(), options.get());
  if (!typesetter)
  {
    return;
  }

  AutoCF<CTLineRef> line = CTTypesetterCreateLine(typesetter.get(), {0, 0});
  if (!line)
  {
    return;
  }

  std::unordered_map<CTFontRef, rive::rcp<rive::Font>> fallbackFontCache;
  CFArrayRef run_array = CTLineGetGlyphRuns(line.get());
  CFIndex runCount = CFArrayGetCount(run_array);
  for (CFIndex runIndex = 0; runIndex < runCount; runIndex++)
  {
    CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(run_array, runIndex);
    CTRunStatus status = CTRunGetStatus(run);
    bool isRunRTL = (status & kCTRunStatusRightToLeft) != 0;

    uint8_t runLevel = textRun.level;
    bool isBaseLevelRTL = (textRun.level % 2 != 0);
    if (isRunRTL != isBaseLevelRTL)
    {
      runLevel = textRun.level + 1;
    }
    bool isEvenLevel = (runLevel % 2 == 0);

    if (CFIndex count = CTRunGetGlyphCount(run))
    {
      rive::GlyphRun gr(count);
      float fontSize = CTFontGetSize(ctFont);
      const float scale = fontSize > 0 ? textRun.size / fontSize : 1.0f;
      CFDictionaryRef attributes = CTRunGetAttributes(run);
      CTFontRef runFont =
        (CTFontRef)CFDictionaryGetValue(attributes, kCTFontAttributeName);
      if (runFont && runFont != ctFont)
      {
        auto it = fallbackFontCache.find(runFont);
        if (it == fallbackFontCache.end())
        {
          fallbackFontCache[runFont] =
            HBFont::FromSystem((void*)runFont, m_useSystemShaper, m_weight, m_width);
        }
        gr.font = fallbackFontCache[runFont];
      }
      else
      {
        gr.font = textRun.font;
      }

      gr.size = textRun.size;
      gr.lineHeight = textRun.lineHeight;
      gr.letterSpacing = textRun.letterSpacing;
      gr.styleId = textRun.styleId;
      gr.level = runLevel;

      CTRunGetGlyphs(run, {0, count}, gr.glyphs.data());
      if (!isEvenLevel)
      {
        std::reverse(gr.glyphs.begin(), gr.glyphs.end());
      }

      AutoSTArray<1024, CFIndex> indices(count);
      AutoSTArray<1024, CGSize> advances(count);

      CTRunGetAdvances(run, {0, count}, advances.data());
      CTRunGetStringIndices(run, {0, count}, indices.data());

      CFIndex reverseIndex = count - 1;
      for (CFIndex i = 0; i < count; ++i)
      {
        CFIndex glyphIndex = isEvenLevel ? i : reverseIndex;
        float advance = (float)(advances[i].width * scale) +
                        textRun.letterSpacing;
        gr.xpos[glyphIndex] = gr.advances[glyphIndex] = advance;

        CFIndex u16Index = indices[i];
        uint32_t u32Offset =
            (u16Index >= 0 && (size_t)u16Index < u16ToU32.size())
                ? u16ToU32[u16Index]
                : (uint32_t)std::max<CFIndex>(0, u16Index);

        gr.textIndices[glyphIndex] =
            rive::math::lossless_numeric_cast<uint32_t>(textStart + u32Offset);

        gr.offsets[glyphIndex] = rive::Vec2D(0.0f, 0.0f);
        reverseIndex--;
      }
      gr.xpos[count] = 0;

      gruns.add(std::move(gr));
    }
  }
}

rive::RawPath CoreTextHBFont::getPath(rive::GlyphID glyph) const
{
  // When we use the system shaper (coretext) get the glyphs from there
  // too.
  if (m_useSystemShaper)
  {
    rive::RawPath ctPath;
    if (ct_extract_glyph_path(m_ctFont, glyph, &ctPath))
    {
      return ctPath;
    }
  }

  rive::RawPath rpath = HBFont::getPath(glyph);

  // We didn't use the system shaper, but harfbuzz failed to extract the
  // glyphs. Try getting them from the system.
  if (rpath.empty() && !m_useSystemShaper)
  {
    rive::RawPath ctPath;
    if (ct_extract_glyph_path(m_ctFont, glyph, &ctPath))
    {
      return ctPath;
    }
  }
  return rpath;
}

uint16_t CoreTextHBFont::getWeight() const
{
  return m_weight > 0 ? m_weight : HBFont::getWeight();
}

rive::SimpleArray<rive::Paragraph> CoreTextHBFont::onShapeText(
  rive::Span<const rive::Unichar> text,
  rive::Span<const rive::TextRun> runs,
  int textDirectionFlag) const
{
  if (!m_useSystemShaper)
  {
    return HBFont::onShapeText(text, runs, textDirectionFlag);
  }

  rive::SimpleArrayBuilder<rive::Paragraph> paragraphs;

  SBCodepointSequence codepointSequence = {SBStringEncodingUTF32,
                      (void*)text.data(),
                      text.size()};

  SBAlgorithmRef bidiAlgorithm = SBAlgorithmCreate(&codepointSequence);

  SBLevel defaultLevel;
  switch (textDirectionFlag)
  {
    case 0:
      defaultLevel = 0;
      break;
    case 1:
      defaultLevel = 1;
      break;
    default:
      defaultLevel = SBLevelDefaultLTR;
      break;
  }

  SBUInteger paragraphStart = 0;
  while (paragraphStart < text.size())
  {
    SBParagraphRef sbParagraph = SBAlgorithmCreateParagraph(bidiAlgorithm,
                                paragraphStart,
                                INT32_MAX,
                                defaultLevel);
    SBUInteger paragraphLength = SBParagraphGetLength(sbParagraph);
    if (paragraphLength == 0)
    {
      SBParagraphRelease(sbParagraph);
      break;
    }
    SBUInteger paragraphEnd = paragraphStart + paragraphLength;

    rive::Paragraph paragraph;
    rive::SimpleArrayBuilder<rive::GlyphRun> glyphRuns;

    uint32_t runTextStart = 0;
    for (const auto& textRun : runs)
    {
      uint32_t runTextEnd = runTextStart + textRun.unicharCount;

      // Check if run overlaps with paragraph
      if (runTextStart < paragraphEnd && runTextEnd > paragraphStart)
      {
        uint32_t overlapStart = std::max((uint32_t)paragraphStart, runTextStart);
        uint32_t overlapEnd = std::min((uint32_t)paragraphEnd, runTextEnd);
        uint32_t overlapLength = overlapEnd - overlapStart;

        if (overlapLength > 0)
        {
          CTFontRef ctFont = m_ctFont;

          AutoUTF16 utf16(&text[overlapStart], overlapLength);
          CFIndex utf16Length =
            rive::math::lossless_numeric_cast<CFIndex>(utf16.array.size());

          // Build mapping from UTF16 to UTF32 offsets
          std::vector<uint32_t> u16ToU32(utf16Length, 0);
          uint32_t u16_idx = 0;
          for (uint32_t i = 0; i < overlapLength; ++i)
          {
            if (u16_idx < utf16Length)
            {
              u16ToU32[u16_idx] = i;
            }
            if (text[overlapStart + i] > 0xFFFF)
            {
              if (u16_idx + 1 < utf16Length)
              {
                u16ToU32[u16_idx + 1] = i;
              }
              u16_idx += 2;
            }
            else
            {
              u16_idx += 1;
            }
          }

          AutoCF<CFStringRef> string = CFStringCreateWithCharactersNoCopy(
            nullptr, utf16.array.data(), utf16Length, kCFAllocatorNull);

          AutoCF<CFMutableDictionaryRef> attr =
            CFDictionaryCreateMutable(kCFAllocatorDefault,
                         0,
                         &kCFTypeDictionaryKeyCallBacks,
                         &kCFTypeDictionaryValueCallBacks);
          CFDictionaryAddValue(attr.get(), kCTFontAttributeName, ctFont);

          AutoCF<CFMutableAttributedStringRef> attrString =
            CFAttributedStringCreateMutable(kCFAllocatorDefault, utf16Length);
          CFAttributedStringReplaceString(
            attrString.get(), CFRangeMake(0, 0), string.get());
          CFAttributedStringSetAttributes(
            attrString.get(), CFRangeMake(0, utf16Length), attr.get(), false);

          SInt8 forcedLevel = static_cast<SInt8>(textRun.level);
          AutoCF<CFNumberRef> level_number =
            CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt8Type, &forcedLevel);
          const void* optionKeys[] = { kCTTypesetterOptionForcedEmbeddingLevel };
          const void* optionValues[] = { level_number.get() };
          AutoCF<CFDictionaryRef> options = CFDictionaryCreate(
            kCFAllocatorDefault,
            optionKeys,
            optionValues,
            1,
            &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks);

          AutoCF<CTTypesetterRef> typesetter =
            CTTypesetterCreateWithAttributedStringAndOptions(attrString.get(),
                                    options.get());
          if (!typesetter)
          {
            continue;
          }

          AutoCF<CTLineRef> line = CTTypesetterCreateLine(typesetter.get(), {0, 0});
          if (!line)
          {
            continue;
          }

          std::unordered_map<CTFontRef, rive::rcp<rive::Font>> fallbackFontCache;
          CFArrayRef run_array = CTLineGetGlyphRuns(line.get());
          CFIndex runCount = CFArrayGetCount(run_array);
          for (CFIndex runIndex = 0; runIndex < runCount; runIndex++)
          {
            CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(run_array, runIndex);
            CTRunStatus status = CTRunGetStatus(run);
            bool isRunRTL = (status & kCTRunStatusRightToLeft) != 0;

            uint8_t runLevel = textRun.level;
            bool isBaseLevelRTL = (textRun.level % 2 != 0);
            if (isRunRTL != isBaseLevelRTL)
            {
              runLevel = textRun.level + 1;
            }
            bool isEvenLevel = (runLevel % 2 == 0);

            if (CFIndex count = CTRunGetGlyphCount(run))
            {
              rive::GlyphRun gr(count);
              float fontSize = CTFontGetSize(ctFont);
              const float scale = fontSize > 0 ? textRun.size / fontSize : 1.0f;
              CFDictionaryRef attributes = CTRunGetAttributes(run);
              CTFontRef runFont = (CTFontRef)CFDictionaryGetValue(attributes, kCTFontAttributeName);
              if (runFont && runFont != ctFont) {
                auto it = fallbackFontCache.find(runFont);
                if (it == fallbackFontCache.end()) {
                  fallbackFontCache[runFont] = HBFont::FromSystem((void*)runFont, m_useSystemShaper, m_weight, m_width);
                }
                gr.font = fallbackFontCache[runFont];
              } else {
                gr.font = textRun.font;
              }

              gr.size = textRun.size;
              gr.lineHeight = textRun.lineHeight;
              gr.letterSpacing = textRun.letterSpacing;
              gr.styleId = textRun.styleId;
              gr.level = runLevel;

              CTRunGetGlyphs(run, {0, count}, gr.glyphs.data());
              if (!isEvenLevel)
              {
                std::reverse(gr.glyphs.begin(), gr.glyphs.end());
              }

              AutoSTArray<1024, CFIndex> indices(count);
              AutoSTArray<1024, CGSize> advances(count);

              CTRunGetAdvances(run, {0, count}, advances.data());
              CTRunGetStringIndices(run, {0, count}, indices.data());

              CFIndex reverseIndex = count - 1;
              for (CFIndex i = 0; i < count; ++i)
              {
                CFIndex glyphIndex = isEvenLevel ? i : reverseIndex;
                float advance =
                  (float)(advances[i].width * scale) + textRun.letterSpacing;
                gr.xpos[glyphIndex] = gr.advances[glyphIndex] = advance;
                
                CFIndex u16Index = indices[i];
                uint32_t u32Offset =
                  (u16Index >= 0 && (size_t)u16Index < u16ToU32.size())
                    ? u16ToU32[u16Index]
                    : (uint32_t)std::max<CFIndex>(0, u16Index);
                
                gr.textIndices[glyphIndex] =
                  rive::math::lossless_numeric_cast<uint32_t>(
                    overlapStart + u32Offset);

                gr.offsets[glyphIndex] = rive::Vec2D(0.0f, 0.0f);
                reverseIndex--;
              }
              gr.xpos[count] = 0;

              glyphRuns.add(std::move(gr));
            }
          }
        }
      }
      runTextStart += textRun.unicharCount;
    }

    // turn the advances we stored in xpos[] into actual x-positions
    float pos = 0;
    for (auto& gr : glyphRuns)
    {
      for (auto& xp : gr.xpos)
      {
        float adv = xp;
        xp = pos;
        pos += adv;
      }
    }

    paragraph.runs = std::move(glyphRuns);
    paragraph.level = SBParagraphGetBaseLevel(sbParagraph);
    paragraphs.add(std::move(paragraph));

    paragraphStart += paragraphLength;
    SBParagraphRelease(sbParagraph);
  }

  SBAlgorithmRelease(bidiAlgorithm);

  return std::move(paragraphs);
}

rive::rcp<rive::Font> CoreTextHBFont::withOptions(
  rive::Span<const Coord> coords,
  rive::Span<const Feature> features) const
{
  std::unordered_map<hb_tag_t, float> axisValues = m_axisValues;
  for (size_t i = 0; i < coords.size(); ++i)
  {
    axisValues[coords[i].axis] = coords[i].value;
  }

  AutoSTArray<16, hb_variation_t> vars(axisValues.size());
  size_t i = 0;
  for (auto itr = axisValues.begin(); itr != axisValues.end(); itr++)
  {
    vars[i++] = {itr->first, itr->second};
  }

  auto font = hb_font_create_sub_font(m_font);
  hb_font_set_variations(font, vars.data(), (unsigned int)axisValues.size());

  std::vector<hb_feature_t> hbFeatures;
  std::unordered_map<hb_tag_t, uint32_t> featureValues = m_featureValues;
  for (auto feature : features)
  {
    featureValues[feature.tag] = feature.value;
  }
  for (auto itr = featureValues.begin(); itr != featureValues.end(); itr++)
  {
    hbFeatures.push_back({itr->first,
               itr->second,
               HB_FEATURE_GLOBAL_START,
               HB_FEATURE_GLOBAL_END});
  }

  uint16_t weight = m_weight;
  auto weightItr = axisValues.find(HB_OT_TAG_VAR_AXIS_WEIGHT);
  if (weightItr != axisValues.end() && std::isfinite(weightItr->second) &&
    weightItr->second > 0.0f)
  {
    weight = static_cast<uint16_t>(
      std::clamp(std::round(weightItr->second), 1.0f, 1000.0f));
  }

  return rive::rcp<rive::Font>(
    new CoreTextHBFont(font, axisValues, featureValues, hbFeatures, m_ctFont, m_useSystemShaper, weight, m_width));
}

static constexpr hb_tag_t kGradAxisTag = HB_TAG('G', 'R', 'A', 'D');

static float grad_offset_for_font(const rive::Font* font)
{
  uint16_t axisCount = font->getAxisCount();
  for (uint16_t i = 0; i < axisCount; ++i)
  {
    rive::Font::Axis axis = font->getAxis(i);
    if (axis.tag == kGradAxisTag)
    {
      float grad =
        std::clamp(font->getAxisValue(kGradAxisTag), axis.min, axis.max);
      return grad - axis.def;
    }
  }
  return font->getAxisValue(kGradAxisTag);
}

static uint16_t effective_weight_for_font(const rive::Font* font)
{
  if (!font)
  {
    return 400;
  }
  float weight = font->getAxisValue(HB_OT_TAG_VAR_AXIS_WEIGHT);
  if (!std::isfinite(weight) || weight <= 0.0f)
  {
    weight = static_cast<float>(font->getWeight());
  }
  if (!std::isfinite(weight) || weight <= 0.0f)
  {
    weight = 400.0f;
  }
  // Variable fonts like Roboto Flex use the GRAD (Grade) axis to thicken
  // strokes in weight-equivalent units without changing advance width
  // (for example, wght=600 with GRAD=115 vs. GRAD=0). System fallback fonts
  // lack a GRAD axis, so fold the GRAD delta relative to the axis default
  // into the requested fallback weight.
  float gradOffset = grad_offset_for_font(font);
  if (!std::isfinite(gradOffset))
  {
    gradOffset = 0.0f;
  }
  return static_cast<uint16_t>(
    std::clamp(std::round(weight + gradOffset), 100.0f, 900.0f));
}

static CFStringRef ct_ui_usage_for_weight(uint16_t weight)
{
  if (weight < 150)
  {
    return CFSTR("CTFontThinUsage");
  }
  if (weight < 250)
  {
    return CFSTR("CTFontUltraLightUsage");
  }
  if (weight < 350)
  {
    return CFSTR("CTFontLightUsage");
  }
  if (weight < 450)
  {
    return CFSTR("CTFontRegularUsage");
  }
  if (weight < 550)
  {
    return CFSTR("CTFontMediumUsage");
  }
  if (weight < 650)
  {
    return CFSTR("CTFontDemiUsage");
  }
  if (weight < 750)
  {
    return CFSTR("CTFontBoldUsage");
  }
  if (weight < 850)
  {
    return CFSTR("CTFontHeavyUsage");
  }
  return CFSTR("CTFontBlackUsage");
}

// CTFontCreateForString only preserves weight across its CJK fallback cascade
// when the base CTFont carries NSCTFontUIUsageAttribute; a named font like
// Helvetica or a numeric weight trait falls back to Regular.
static CTFontRef ct_create_system_font_for_weight(uint16_t weight, CGFloat size)
{
  CFStringRef usage = ct_ui_usage_for_weight(weight);
  const void* keys[] = {CFSTR("NSCTFontUIUsageAttribute")};
  const void* values[] = {usage};
  AutoCF<CFDictionaryRef> attributes = CFDictionaryCreate(
    kCFAllocatorDefault,
    keys,
    values,
    1,
    &kCFTypeDictionaryKeyCallBacks,
    &kCFTypeDictionaryValueCallBacks);
  if (attributes)
  {
    AutoCF<CTFontDescriptorRef> descriptor =
      CTFontDescriptorCreateWithAttributes(attributes.get());
    if (descriptor)
    {
      CTFontRef font =
        CTFontCreateWithFontDescriptor(descriptor.get(), size, nullptr);
      if (font)
      {
        return font;
      }
    }
  }
  return CTFontCreateWithName(CFSTR("Helvetica"), size, nullptr);
}

static rive::rcp<rive::Font> AppleStyleFallbackProc(const rive::Unichar missing,
                          const uint32_t fallbackIndex,
                          const rive::Font* font)
{
  // 1. Convert Unichar to CFString
  UniChar characters[2];
  size_t charCount = 0;
  if (missing <= 0xFFFF)
  {
    characters[0] = (UniChar)missing;
    charCount = 1;
  }
  else
  {
    characters[0] = (UniChar)(0xD800 + ((missing - 0x10000) >> 10));
    characters[1] = (UniChar)(0xDC00 + ((missing - 0x10000) & 0x3FF));
    charCount = 2;
  }
  AutoCF<CFStringRef> str =
    CFStringCreateWithCharacters(kCFAllocatorDefault, characters, charCount);
  if (!str)
  {
    return nullptr;
  }

  uint16_t weight = effective_weight_for_font(font);

  // 2. Find the system fallback font for this specific character
  AutoCF<CTFontRef> baseFont = ct_create_system_font_for_weight(weight, 16.0);
  if (!baseFont)
  {
    return nullptr;
  }
  AutoCF<CTFontRef> fallbackFont =
    CTFontCreateForString(baseFont.get(), str.get(), CFRangeMake(0, charCount));
  if (!fallbackFont)
  {
    return nullptr;
  }

  // 3. Wrap the resolved system fallback font in our CoreTextHBFont
  return HBFont::FromSystem((void*)fallbackFont.get(), true, weight, 100);
}

struct AppleFallbackProcRegister
{
  AppleFallbackProcRegister()
  {
    rive::Font::gFallbackProc = AppleStyleFallbackProc;
  }
};

static AppleFallbackProcRegister g_AppleFallbackProcRegister;

#endif

// Copyright 2026 The Cobalt Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/rive/html_rive_element.h"

#include <optional>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "cc/paint/paint_op_buffer.h"
#include "cc/paint/record_paint_canvas.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/html/rive/layout_rive.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_array_buffer.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColor.h"

namespace blink {

namespace {

base::FilePath GetTestDataPath(const char* filename) {
  base::FilePath src_root;
  base::PathService::Get(base::DIR_SRC_TEST_DATA_ROOT, &src_root);
  return src_root.AppendASCII("third_party/blink/renderer/core/html/rive/testdata")
      .AppendASCII(filename);
}

size_t CountNonTransparentPixels(const SkBitmap& bitmap) {
  size_t count = 0;
  for (int y = 0; y < bitmap.height(); ++y) {
    for (int x = 0; x < bitmap.width(); ++x) {
      if (SkColorGetA(bitmap.getColor(x, y)) > 0) {
        ++count;
      }
    }
  }
  return count;
}

}  // namespace

class HTMLRiveElementTest : public PageTestBase {};

TEST_F(HTMLRiveElementTest, CreatesRivePlayerElementAndLayoutRive) {
  SetBodyInnerHTML(R"HTML(
    <rive-player id="player" fit="cover" alignment="top-left"></rive-player>
  )HTML");
  UpdateAllLifecyclePhasesForTest();

  Element* elem = GetElementById("player");
  ASSERT_NE(elem, nullptr);
  auto* rive_elem = static_cast<HTMLRiveElement*>(elem);
  EXPECT_EQ(rive_elem->fit(), "cover");
  EXPECT_EQ(rive_elem->alignment(), "top-left");
  EXPECT_TRUE(rive_elem->autoplay());
  EXPECT_FALSE(rive_elem->loaded());

  LayoutObject* layout_object = rive_elem->GetLayoutObject();
  ASSERT_NE(layout_object, nullptr);
  EXPECT_STREQ(layout_object->GetName(), "LayoutRive");
}

TEST_F(HTMLRiveElementTest, LoadsAndRendersLikeButtonRiv) {
  SetBodyInnerHTML(R"HTML(
    <rive-player id="player" style="width: 200px; height: 200px;"></rive-player>
  )HTML");
  UpdateAllLifecyclePhasesForTest();

  auto* rive_elem = static_cast<HTMLRiveElement*>(GetElementById("player"));
  ASSERT_NE(rive_elem, nullptr);

  std::optional<std::vector<uint8_t>> bytes =
      base::ReadFileToBytes(GetTestDataPath("like_button.riv"));
  ASSERT_TRUE(bytes.has_value()) << "Failed to read like_button.riv";

  DOMArrayBuffer* buffer = DOMArrayBuffer::Create(*bytes);
  ASSERT_TRUE(rive_elem->loadFromBuffer(buffer));
  EXPECT_TRUE(rive_elem->loaded());
  EXPECT_GT(rive_elem->artboardWidth(), 0.0);
  EXPECT_GT(rive_elem->artboardHeight(), 0.0);
  EXPECT_GE(rive_elem->artboardNames().size(), 1u);
  EXPECT_GE(rive_elem->stateMachineNames().size(), 1u);

  rive_elem->advance(0.016);
  UpdateAllLifecyclePhasesForTest();

  cc::RecordPaintCanvas record_canvas;
  rive_elem->PaintRive(&record_canvas, gfx::SizeF(200.0f, 200.0f));
  cc::PaintRecord paint_record = record_canvas.ReleaseAsRecord();
  EXPECT_GT(paint_record.size(), 0u);

  SkBitmap bitmap;
  bitmap.allocN32Pixels(200, 200);
  bitmap.eraseColor(SK_ColorTRANSPARENT);
  SkCanvas sk_canvas(bitmap);
  paint_record.Playback(&sk_canvas);

  EXPECT_GT(CountNonTransparentPixels(bitmap), 100u);
}

TEST_F(HTMLRiveElementTest, LoadsAndRendersListsDemoRiv) {
  SetBodyInnerHTML(R"HTML(
    <rive-player id="player" style="width: 400px; height: 300px;"></rive-player>
  )HTML");
  UpdateAllLifecyclePhasesForTest();

  auto* rive_elem = static_cast<HTMLRiveElement*>(GetElementById("player"));
  ASSERT_NE(rive_elem, nullptr);

  std::optional<std::vector<uint8_t>> bytes =
      base::ReadFileToBytes(GetTestDataPath("lists_demo.riv"));
  ASSERT_TRUE(bytes.has_value()) << "Failed to read lists_demo.riv";

  DOMArrayBuffer* buffer = DOMArrayBuffer::Create(*bytes);
  ASSERT_TRUE(rive_elem->loadFromBuffer(buffer));
  EXPECT_TRUE(rive_elem->loaded());
  EXPECT_GT(rive_elem->artboardWidth(), 0.0);
  EXPECT_GT(rive_elem->artboardHeight(), 0.0);

  rive_elem->advance(0.016);

  cc::RecordPaintCanvas record_canvas;
  rive_elem->PaintRive(&record_canvas, gfx::SizeF(400.0f, 300.0f));
  cc::PaintRecord paint_record = record_canvas.ReleaseAsRecord();
  EXPECT_GT(paint_record.size(), 0u);

  SkBitmap bitmap;
  bitmap.allocN32Pixels(400, 300);
  bitmap.eraseColor(SK_ColorTRANSPARENT);
  SkCanvas sk_canvas(bitmap);
  paint_record.Playback(&sk_canvas);

  EXPECT_GT(CountNonTransparentPixels(bitmap), 500u);
}

}  // namespace blink

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
#include "third_party/blink/renderer/bindings/core/v8/v8_keyboard_event_init.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/event_type_names.h"
#include "third_party/blink/renderer/core/events/keyboard_event.h"
#include "third_party/blink/renderer/core/execution_context/agent.h"
#include "third_party/blink/renderer/core/html/rive/layout_rive.h"
#include "third_party/blink/renderer/core/testing/page_test_base.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_array_buffer.h"
#include "third_party/blink/renderer/platform/scheduler/public/event_loop.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColor.h"

namespace blink {

namespace {

base::FilePath GetTestDataPath(const char* filename) {
  base::FilePath src_root;
  base::PathService::Get(base::DIR_SRC_TEST_DATA_ROOT, &src_root);
  return src_root
      .AppendASCII("third_party/blink/renderer/core/html/rive/testdata")
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

class HTMLRiveElementFocusTest : public HTMLRiveElementTest {
 protected:
  // Loads focus_nodes_list_order.riv: four focusable buttons stacked
  // vertically.
  HTMLRiveElement* LoadFocusFixture() {
    SetBodyInnerHTML(R"HTML(
      <button id="before">before</button>
      <rive-player id="player" style="width: 500px; height: 500px;">
      </rive-player>
      <button id="after">after</button>
    )HTML");
    UpdateAllLifecyclePhasesForTest();
    auto* rive_elem = static_cast<HTMLRiveElement*>(GetElementById("player"));
    std::optional<std::vector<uint8_t>> bytes =
        base::ReadFileToBytes(GetTestDataPath("focus_nodes_list_order.riv"));
    EXPECT_TRUE(bytes.has_value());
    EXPECT_TRUE(rive_elem->loadFromBuffer(DOMArrayBuffer::Create(*bytes)));
    UpdateAllLifecyclePhasesForTest();
    return rive_elem;
  }

  // Dispatches a trusted keydown and returns whether the player handled it.
  bool PressKey(HTMLRiveElement* rive_elem, const char* key) {
    auto* init = KeyboardEventInit::Create();
    init->setKey(key);
    init->setBubbles(true);
    init->setCancelable(true);
    auto* event =
        MakeGarbageCollected<KeyboardEvent>(event_type_names::kKeydown, init);
    event->SetTrusted(true);
    rive_elem->DispatchEvent(*event);
    return event->DefaultHandled();
  }

  // Moves in `direction` until it fails and returns how many moves succeeded.
  int CountMoves(HTMLRiveElement* rive_elem, const char* direction) {
    int moves = 0;
    while (moves < 10 && rive_elem->moveFocus(direction)) {
      ++moves;
    }
    return moves;
  }

  void RunMicrotasks() {
    GetDocument().GetAgent().event_loop()->PerformMicrotaskCheckpoint();
  }
};

TEST_F(HTMLRiveElementFocusTest, NotFocusableWithoutFocusNodes) {
  SetBodyInnerHTML(R"HTML(<rive-player id="player"></rive-player>)HTML");
  UpdateAllLifecyclePhasesForTest();
  auto* rive_elem = static_cast<HTMLRiveElement*>(GetElementById("player"));
  EXPECT_FALSE(rive_elem->hasFocusNodes());
  EXPECT_FALSE(rive_elem->IsFocusable());
  EXPECT_FALSE(rive_elem->moveFocus("next"));
}

TEST_F(HTMLRiveElementFocusTest, FocusableWhenFileHasFocusNodes) {
  HTMLRiveElement* rive_elem = LoadFocusFixture();
  EXPECT_TRUE(rive_elem->hasFocusNodes());
  EXPECT_TRUE(rive_elem->IsFocusable());
  EXPECT_EQ(rive_elem->tabIndex(), 0);
  EXPECT_FALSE(rive_elem->hasRiveFocus());
}

TEST_F(HTMLRiveElementFocusTest, DomFocusEntersAndBlurClears) {
  HTMLRiveElement* rive_elem = LoadFocusFixture();
  rive_elem->Focus();
  EXPECT_TRUE(rive_elem->IsFocused());
  // Entry waits for the end of the current task.
  EXPECT_FALSE(rive_elem->hasRiveFocus());
  RunMicrotasks();
  EXPECT_TRUE(rive_elem->hasRiveFocus());
  // Default entry is the first node, which is the top button.
  EXPECT_FALSE(rive_elem->moveFocus("up"));

  GetElementById("after")->Focus();
  EXPECT_FALSE(rive_elem->hasRiveFocus());
}

TEST_F(HTMLRiveElementFocusTest, DirectionalEntryStartsAtNearEdge) {
  HTMLRiveElement* rive_elem = LoadFocusFixture();

  // Coming up from below lands on the bottom button.
  EXPECT_TRUE(rive_elem->moveFocus("up"));
  EXPECT_FALSE(rive_elem->moveFocus("down"));
  EXPECT_EQ(CountMoves(rive_elem, "up"), 3);

  // Coming down from above lands on the top button.
  rive_elem->clearFocus();
  EXPECT_TRUE(rive_elem->moveFocus("down"));
  EXPECT_FALSE(rive_elem->moveFocus("up"));
  EXPECT_EQ(CountMoves(rive_elem, "down"), 3);
}

TEST_F(HTMLRiveElementFocusTest, ScriptCanPickEntryEdgeAfterFocus) {
  HTMLRiveElement* rive_elem = LoadFocusFixture();
  // What a page's D-pad handler does when moving up into the player.
  rive_elem->Focus();
  EXPECT_TRUE(rive_elem->moveFocus("up"));
  RunMicrotasks();
  // Still on the bottom button: the deferred default entry didn't override it.
  EXPECT_FALSE(rive_elem->moveFocus("down"));
  EXPECT_EQ(CountMoves(rive_elem, "up"), 3);
}

TEST_F(HTMLRiveElementFocusTest, DirectionalMovesStopAtEdges) {
  HTMLRiveElement* rive_elem = LoadFocusFixture();
  EXPECT_FALSE(rive_elem->moveFocus("sideways"));
  EXPECT_FALSE(rive_elem->hasRiveFocus());

  // The first directional move enters the artboard.
  EXPECT_TRUE(rive_elem->moveFocus("down"));
  EXPECT_TRUE(rive_elem->hasRiveFocus());

  // Four stacked buttons: three moves from one end to the other.
  CountMoves(rive_elem, "up");
  EXPECT_EQ(CountMoves(rive_elem, "down"), 3);
  EXPECT_EQ(CountMoves(rive_elem, "up"), 3);
  EXPECT_FALSE(rive_elem->moveFocus("left"));
  EXPECT_TRUE(rive_elem->hasRiveFocus());

  rive_elem->clearFocus();
  EXPECT_FALSE(rive_elem->hasRiveFocus());
}

TEST_F(HTMLRiveElementFocusTest, ArrowKeysNavigateAndReleaseAtEdges) {
  HTMLRiveElement* rive_elem = LoadFocusFixture();
  rive_elem->Focus();
  RunMicrotasks();
  ASSERT_TRUE(rive_elem->hasRiveFocus());

  // Go to the top, then walk down with the arrow key.
  CountMoves(rive_elem, "up");
  EXPECT_FALSE(PressKey(rive_elem, "ArrowUp"));
  EXPECT_TRUE(PressKey(rive_elem, "ArrowDown"));
  EXPECT_TRUE(PressKey(rive_elem, "ArrowDown"));
  EXPECT_TRUE(PressKey(rive_elem, "ArrowDown"));
  // At the bottom edge the key is left for the page to handle.
  EXPECT_FALSE(PressKey(rive_elem, "ArrowDown"));
  EXPECT_TRUE(rive_elem->hasRiveFocus());

  // Keys with no meaning to the file are not swallowed.
  EXPECT_FALSE(PressKey(rive_elem, "F5"));
}

TEST_F(HTMLRiveElementFocusTest, MovingFocusChangesRenderedFrame) {
  HTMLRiveElement* rive_elem = LoadFocusFixture();
  auto render = [&]() {
    rive_elem->advance(0.5);
    cc::RecordPaintCanvas record_canvas;
    rive_elem->PaintRive(&record_canvas, gfx::SizeF(500.0f, 500.0f));
    SkBitmap bitmap;
    bitmap.allocN32Pixels(500, 500);
    bitmap.eraseColor(SK_ColorTRANSPARENT);
    SkCanvas sk_canvas(bitmap);
    record_canvas.ReleaseAsRecord().Playback(&sk_canvas);
    return bitmap;
  };
  auto differing_pixels = [](const SkBitmap& a, const SkBitmap& b) {
    size_t count = 0;
    for (int y = 0; y < a.height(); ++y) {
      for (int x = 0; x < a.width(); ++x) {
        if (a.getColor(x, y) != b.getColor(x, y)) {
          ++count;
        }
      }
    }
    return count;
  };

  SkBitmap unfocused = render();
  ASSERT_TRUE(rive_elem->moveFocus("next"));
  SkBitmap first = render();
  ASSERT_TRUE(rive_elem->moveFocus("next"));
  SkBitmap second = render();
  EXPECT_GT(differing_pixels(unfocused, first), 0u);
  EXPECT_GT(differing_pixels(first, second), 0u);
}

}  // namespace blink

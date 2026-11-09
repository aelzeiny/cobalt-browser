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
  // The player gets its own composited layer by default (UA stylesheet).
  EXPECT_TRUE(layout_object->StyleRef().HasWillChangeTransformHint());
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

// focus_like.riv: a thumbs-up whose "Like" layer animates motion only and
// whose "Focus" layer animates colour only (white unfocused, black focused).
// Generated with the Rive CLI from RML.
TEST_F(HTMLRiveElementTest, FocusColorSwapsWithoutDisturbingLikeAnimation) {
  SetBodyInnerHTML(R"HTML(
    <rive-player id="a" style="width: 96px; height: 96px;"></rive-player>
    <rive-player id="b" style="width: 96px; height: 96px;"></rive-player>
  )HTML");
  UpdateAllLifecyclePhasesForTest();
  std::optional<std::vector<uint8_t>> bytes =
      base::ReadFileToBytes(GetTestDataPath("focus_like.riv"));
  ASSERT_TRUE(bytes.has_value());
  auto* a = static_cast<HTMLRiveElement*>(GetElementById("a"));
  auto* b = static_cast<HTMLRiveElement*>(GetElementById("b"));
  ASSERT_TRUE(a->loadFromBuffer(DOMArrayBuffer::Create(*bytes)));
  ASSERT_TRUE(b->loadFromBuffer(DOMArrayBuffer::Create(*bytes)));

  auto render = [](HTMLRiveElement* elem) {
    cc::RecordPaintCanvas record_canvas;
    elem->PaintRive(&record_canvas, gfx::SizeF(96.0f, 96.0f));
    SkBitmap bitmap;
    bitmap.allocN32Pixels(96, 96);
    bitmap.eraseColor(SK_ColorTRANSPARENT);
    SkCanvas sk_canvas(bitmap);
    record_canvas.ReleaseAsRecord().Playback(&sk_canvas);
    return bitmap;
  };
  // Counts fully opaque pixels that are white-ish and black-ish.
  auto count_colors = [](const SkBitmap& bitmap, size_t* white, size_t* black) {
    *white = *black = 0;
    for (int y = 0; y < bitmap.height(); ++y) {
      for (int x = 0; x < bitmap.width(); ++x) {
        SkColor c = bitmap.getColor(x, y);
        if (SkColorGetA(c) != 255) {
          continue;
        }
        if (SkColorGetR(c) > 230 && SkColorGetG(c) > 230 &&
            SkColorGetB(c) > 230) {
          ++*white;
        } else if (SkColorGetR(c) < 40 && SkColorGetG(c) < 40 &&
                   SkColorGetB(c) < 40) {
          ++*black;
        }
      }
    }
  };
  auto same_shape = [](const SkBitmap& x, const SkBitmap& y) {
    for (int j = 0; j < x.height(); ++j) {
      for (int i = 0; i < x.width(); ++i) {
        if (SkColorGetA(x.getColor(i, j)) != SkColorGetA(y.getColor(i, j))) {
          return false;
        }
      }
    }
    return true;
  };
  auto step = [&](int frames) {
    for (int i = 0; i < frames; ++i) {
      a->advance(1.0 / 60);
      b->advance(1.0 / 60);
    }
  };

  size_t white, black;
  step(1);
  SkBitmap idle = render(a);
  count_colors(idle, &white, &black);
  EXPECT_GT(white, 0u);
  EXPECT_EQ(black, 0u);

  // Start liking both, then focus only `b` partway through the animation.
  ASSERT_TRUE(a->setViewModelBoolean("liked", true));
  ASSERT_TRUE(b->setViewModelBoolean("liked", true));
  step(5);
  ASSERT_TRUE(b->setViewModelBoolean("focused", true));
  step(8);

  SkBitmap a_mid = render(a);
  SkBitmap b_mid = render(b);
  // Still animating: the pose differs from idle.
  EXPECT_FALSE(same_shape(idle, a_mid));
  // Focus changed only the colour: the shapes match pixel for pixel.
  EXPECT_TRUE(same_shape(a_mid, b_mid));
  count_colors(a_mid, &white, &black);
  EXPECT_GT(white, 0u);
  EXPECT_EQ(black, 0u);
  count_colors(b_mid, &white, &black);
  EXPECT_GT(black, 0u);
  EXPECT_EQ(white, 0u);

  // Unfocus after the animation settles: back to white, same liked pose.
  step(120);
  ASSERT_TRUE(b->setViewModelBoolean("focused", false));
  step(10);
  SkBitmap a_end = render(a);
  SkBitmap b_end = render(b);
  EXPECT_TRUE(same_shape(a_end, b_end));
  count_colors(b_end, &white, &black);
  EXPECT_GT(white, 0u);
  EXPECT_EQ(black, 0u);
}

// The icon sits idle (no frames scheduled) most of the time. A focus change
// must show the new colour on the very first frame, which advances by zero
// because the frame clock restarts, or it flickers against the page.
TEST_F(HTMLRiveElementTest, FocusColorAppliesOnFirstFrameWhenIdle) {
  SetBodyInnerHTML(R"HTML(
    <rive-player id="icon" style="width: 96px; height: 96px;"></rive-player>
  )HTML");
  UpdateAllLifecyclePhasesForTest();
  std::optional<std::vector<uint8_t>> bytes =
      base::ReadFileToBytes(GetTestDataPath("focus_like.riv"));
  ASSERT_TRUE(bytes.has_value());
  auto* icon = static_cast<HTMLRiveElement*>(GetElementById("icon"));
  ASSERT_TRUE(icon->loadFromBuffer(DOMArrayBuffer::Create(*bytes)));
  icon->advance(1.0);

  auto opaque_colors = [&](size_t* white, size_t* black) {
    cc::RecordPaintCanvas record_canvas;
    icon->PaintRive(&record_canvas, gfx::SizeF(96.0f, 96.0f));
    SkBitmap bitmap;
    bitmap.allocN32Pixels(96, 96);
    bitmap.eraseColor(SK_ColorTRANSPARENT);
    SkCanvas sk_canvas(bitmap);
    record_canvas.ReleaseAsRecord().Playback(&sk_canvas);
    *white = *black = 0;
    for (int y = 0; y < 96; ++y) {
      for (int x = 0; x < 96; ++x) {
        SkColor c = bitmap.getColor(x, y);
        if (SkColorGetA(c) != 255) {
          continue;
        }
        if (SkColorGetR(c) > 230) {
          ++*white;
        } else if (SkColorGetR(c) < 40) {
          ++*black;
        }
      }
    }
  };

  size_t white, black;
  ASSERT_TRUE(icon->setViewModelBoolean("focused", true));
  icon->OnAnimationFrame(1000.0);  // First frame after idle: zero elapsed.
  opaque_colors(&white, &black);
  EXPECT_GT(black, 0u);
  EXPECT_EQ(white, 0u);

  ASSERT_TRUE(icon->setViewModelBoolean("focused", false));
  icon->OnAnimationFrame(2000.0);
  opaque_colors(&white, &black);
  EXPECT_GT(white, 0u);
  EXPECT_EQ(black, 0u);
}

class HTMLRiveElementFocusTest : public HTMLRiveElementTest {
 protected:
  // Loads focus_nodes_list_order.riv: four focusable buttons stacked
  // vertically.
  HTMLRiveElement* LoadFocusFixture(
      const char* filename = "focus_nodes_list_order.riv") {
    SetBodyInnerHTML(R"HTML(
      <button id="before">before</button>
      <rive-player id="player" style="width: 500px; height: 500px;">
      </rive-player>
      <button id="after">after</button>
    )HTML");
    UpdateAllLifecyclePhasesForTest();
    auto* rive_elem = static_cast<HTMLRiveElement*>(GetElementById("player"));
    std::optional<std::vector<uint8_t>> bytes =
        base::ReadFileToBytes(GetTestDataPath(filename));
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

// focus_grid.riv: a 3x2 grid of cards that highlight while focused and fire
// "select-<n>" on Enter. Generated with the Rive CLI from RML.
TEST_F(HTMLRiveElementFocusTest, GridNavigatesInTwoDimensionsAndSelects) {
  HTMLRiveElement* rive_elem = LoadFocusFixture("focus_grid.riv");
  ASSERT_TRUE(rive_elem->hasFocusNodes());

  // Entering from above lands in the top row; walk the grid's edges.
  EXPECT_TRUE(rive_elem->moveFocus("down"));
  EXPECT_FALSE(rive_elem->moveFocus("up"));
  CountMoves(rive_elem, "left");
  EXPECT_EQ(CountMoves(rive_elem, "right"), 2);
  EXPECT_EQ(CountMoves(rive_elem, "down"), 1);
  EXPECT_EQ(CountMoves(rive_elem, "left"), 2);
  EXPECT_EQ(CountMoves(rive_elem, "up"), 1);

  // Now on the top-left card (card 0). Enter goes to the focused card.
  rive_elem->Focus();
  EXPECT_TRUE(PressKey(rive_elem, "Enter"));
  rive_elem->advance(0.016);
  EXPECT_EQ(rive_elem->lastEventName(), "select-0");

  EXPECT_TRUE(rive_elem->moveFocus("right"));
  EXPECT_TRUE(PressKey(rive_elem, "Enter"));
  rive_elem->advance(0.016);
  EXPECT_EQ(rive_elem->lastEventName(), "select-1");

  // Entering from below lands in the bottom row, on its leftmost card.
  rive_elem->clearFocus();
  EXPECT_TRUE(rive_elem->moveFocus("up"));
  EXPECT_FALSE(rive_elem->moveFocus("down"));
  EXPECT_FALSE(rive_elem->moveFocus("left"));
  EXPECT_TRUE(PressKey(rive_elem, "Enter"));
  EXPECT_EQ(rive_elem->lastEventName(), "select-3");
}

}  // namespace blink

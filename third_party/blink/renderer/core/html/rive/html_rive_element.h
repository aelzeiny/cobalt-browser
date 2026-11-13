// Copyright 2026 The Cobalt Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_RIVE_HTML_RIVE_ELEMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_RIVE_HTML_RIVE_ELEMENT_H_

#include <map>
#include <memory>
#include <string>

#include "base/containers/span.h"
#include "base/memory/raw_ptr.h"
#include "cc/paint/paint_canvas.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/core/loader/threadable_loader.h"
#include "third_party/blink/renderer/core/loader/threadable_loader_client.h"
#include "third_party/blink/renderer/core/page/page_visibility_observer.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_array_buffer.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_hash_map.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextra-semi"
#pragma clang diagnostic ignored "-Wignored-qualifiers"
#pragma clang diagnostic ignored "-Wshadow"
#include "third_party/rive/include/rive/animation/linear_animation_instance.hpp"
#include "third_party/rive/include/rive/animation/state_machine_instance.hpp"
#include "third_party/rive/include/rive/artboard.hpp"
#include "third_party/rive/include/rive/file.hpp"
#include "third_party/rive/include/rive/layout.hpp"
#include "third_party/rive/include/rive/refcnt.hpp"
#include "third_party/rive/include/rive/renderer.hpp"
#include "third_party/rive/include/rive/viewmodel/runtime/viewmodel_instance_runtime.hpp"
#pragma clang diagnostic pop
#include "ui/gfx/geometry/size_f.h"

namespace cc {
class Layer;
}  // namespace cc

namespace gfx {
class Size;
}  // namespace gfx

namespace rive {
class ArtboardComponentList;
}  // namespace rive

namespace blink {

class KeyboardEvent;
class ImageResourceContent;
class RiveGpuContext;
class RiveGpuSurface;
class RiveImageRequest;

class CORE_EXPORT HTMLRiveElement final : public HTMLElement,
                                          public ThreadableLoaderClient,
                                          public PageVisibilityObserver {
  DEFINE_WRAPPERTYPEINFO();

 public:
  explicit HTMLRiveElement(Document& document);
  ~HTMLRiveElement() override;

  // IDL attributes
  String src() const;
  void setSrc(const String& value);

  String artboard() const;
  void setArtboard(const String& value);

  String stateMachine() const;
  void setStateMachine(const String& value);

  String animation() const;
  void setAnimation(const String& value);

  String fit() const;
  void setFit(const String& value);

  String alignment() const;
  void setAlignment(const String& value);

  String viewModel() const;
  void setViewModel(const String& value);

  bool autoplay() const;
  void setAutoplay(bool value);

  bool paused() const { return paused_; }
  bool animating() const { return raf_callback_id_ != 0; }
  bool loaded() const { return artboard_ != nullptr; }
  double artboardWidth() const;
  double artboardHeight() const;
  String lastEventName() const { return last_event_name_; }

  Vector<String> artboardNames() const;
  Vector<String> stateMachineNames() const;
  Vector<String> animationNames() const;
  Vector<String> inputNames() const;

  // IDL playback controls
  void play();
  void pause();
  void stop();
  bool advance(double seconds);

  bool loadFromBuffer(DOMArrayBuffer* data);

  // IDL State Machine inputs
  bool setBooleanInput(const String& name, bool value);
  bool getBooleanInput(const String& name) const;
  bool setNumberInput(const String& name, double value);
  double getNumberInput(const String& name) const;
  bool fireTrigger(const String& name);

  // IDL Text runs
  bool setTextRun(const String& name, const String& value);
  String getTextRun(const String& name) const;

  // IDL ViewModel data binding
  bool setViewModelNumber(const String& path, double value);
  double getViewModelNumber(const String& path) const;
  bool setViewModelBoolean(const String& path, bool value);
  bool getViewModelBoolean(const String& path) const;
  bool setViewModelString(const String& path, const String& value);
  String getViewModelString(const String& path) const;
  bool setViewModelColor(const String& path, int32_t argb);
  int32_t getViewModelColor(const String& path) const;
  bool setViewModelEnum(const String& path, const String& value);
  String getViewModelEnum(const String& path) const;
  bool fireViewModelTrigger(const String& path);

  // IDL ViewModel lists
  bool setListItems(const String& list_path,
                    const String& view_model,
                    const Vector<Vector<std::pair<String, String>>>& items);
  bool appendListItems(const String& list_path,
                       const String& view_model,
                       const Vector<Vector<std::pair<String, String>>>& items);
  uint32_t listItemCount(const String& list_path) const;
  bool focusListItem(const String& list_path, uint32_t index);
  bool scrollToListItem(const String& list_path, uint32_t index);
  String getListItemValue(const String& list_path,
                          uint32_t index,
                          const String& property) const;
  bool setListItemValue(const String& list_path,
                        uint32_t index,
                        const String& property,
                        const String& value);

  // Called by RiveImageRequest when a list item's image URL has loaded.
  void OnListImageLoaded(const String& url, ImageResourceContent* content);

  // IDL Focus and navigation
  bool hasFocusNodes() const;
  bool hasRiveFocus() const;
  bool moveFocus(const String& direction);
  void clearFocus();

  // Called by LayoutRive
  gfx::SizeF ArtboardSize() const;
  // With Rive Renderer: draws into the player's own layer and returns it;
  // null when painting with Skia (PaintRive) instead.
  cc::Layer* PaintRiveToLayer(const gfx::Size& box_size);
  void PaintRive(cc::PaintCanvas* canvas, const gfx::SizeF& box_size);

  // Called by RiveFrameCallback
  void OnAnimationFrame(double high_res_now_ms);

  // Whether the loaded file draws with Rive Renderer rather than Skia.
  bool UsesRiveRendererForTesting() const { return uses_rive_renderer_; }

  void Trace(Visitor* visitor) const override;

 private:
  // Element / Node overrides
  void ParseAttribute(const AttributeModificationParams& params) override;
  LayoutObject* CreateLayoutObject(const ComputedStyle& style) override;
  InsertionNotificationRequest InsertedInto(
      ContainerNode& insertion_point) override;
  void RemovedFrom(ContainerNode& insertion_point) override;
  void DidMoveToNewDocument(Document& old_document) override;

  // PageVisibilityObserver: GPU state may be gone after the page was hidden.
  void PageVisibilityChanged() override;
  void DefaultEventHandler(Event& event) override;
  FocusableState SupportsFocus(UpdateBehavior update_behavior) const override;
  int DefaultTabIndex() const override;
  void SetFocused(bool received, mojom::blink::FocusType type) override;

  // ThreadableLoaderClient overrides
  void DidReceiveData(base::span<const char> data) override;
  void DidFinishLoading(uint64_t identifier) override;
  void DidFail(uint64_t identifier, const ResourceError& error) override;
  void DidFailRedirectCheck(uint64_t identifier) override;

  void StartLoadingSrc(const String& url_string);
  void CancelLoader();
  bool LoadFromBytes(base::span<const uint8_t> bytes);
  void InstantiateArtboardAndScene();
  bool AdvanceInternal(float elapsed_seconds);
  void ScheduleAnimationIfNeeded();
  void CancelScheduledAnimation();
  void InvalidateRivePaint();
  void DispatchReportedRiveEvents();
  void OnViewModelValuesChanged();
  bool AddListItems(const String& list_path,
                    const String& view_model,
                    const Vector<Vector<std::pair<String, String>>>& items,
                    bool replace);
  void AlignFocusedToScrollStart();
  rive::ArtboardComponentList* FindArtboardList() const;
  void SetListItemValue(rive::ViewModelInstanceRuntime* instance,
                        const String& property,
                        const String& value);
  bool HandleKeyboardEvent(const KeyboardEvent& event);
  bool EnterRiveFocusFromEdge(rive::Direction direction);
  void EnterRiveFocusIfNeeded(mojom::blink::FocusType type);
  void OnRiveFocusChanged();

  rive::Fit GetRiveFit() const;
  rive::Alignment GetRiveAlignment() const;
  void DrawArtboard(rive::Renderer* renderer, const gfx::SizeF& box_size);
  void ScheduleGpuReload();
  void ReloadOntoNewGpuContext();
  bool MapOffsetToArtboard(float offset_x,
                           float offset_y,
                           rive::Vec2D* out_point) const;

  Member<ThreadableLoader> loader_;
  Vector<uint8_t> pending_bytes_;
  String loaded_url_;

  rive::rcp<rive::File> rive_file_;
  // The file's paths and paints are made for one backend, picked at load.
  bool uses_rive_renderer_ = false;
  // The Rive Renderer context the file was loaded onto, and the file, to
  // reload it onto a new context if that one is lost.
  raw_ptr<RiveGpuContext> gpu_context_ = nullptr;
  Vector<uint8_t> loaded_bytes_;
  bool gpu_reload_scheduled_ = false;
  std::unique_ptr<RiveGpuSurface> gpu_surface_;
  std::unique_ptr<rive::ArtboardInstance> artboard_;
  std::unique_ptr<rive::StateMachineInstance> state_machine_;
  std::unique_ptr<rive::LinearAnimationInstance> animation_instance_;
  rive::rcp<rive::ViewModelInstanceRuntime> view_model_instance_;

  bool paused_ = false;
  int raf_callback_id_ = 0;
  double last_frame_time_ms_ = 0.0;
  String last_event_name_;
  size_t reported_events_seen_ = 0;

  // Image URLs used by list items: in-flight or finished fetches, and the
  // Rive images made from finished ones.
  HeapHashMap<String, Member<RiveImageRequest>> image_requests_;
  std::map<std::string, rive::rcp<rive::RenderImage>> image_cache_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_RIVE_HTML_RIVE_ELEMENT_H_

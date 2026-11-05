// Copyright 2026 The Cobalt Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/rive/html_rive_element.h"

#include <algorithm>
#include <utility>

#include "services/network/public/mojom/fetch_api.mojom-blink.h"
#include "third_party/blink/public/mojom/fetch/fetch_api_request.mojom-blink.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/frame_request_callback_collection.h"
#include "third_party/blink/renderer/core/events/mouse_event.h"
#include "third_party/blink/renderer/core/events/pointer_event.h"
#include "third_party/blink/renderer/core/event_type_names.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/html/rive/layout_rive.h"
#include "third_party/blink/renderer/core/html/rive/rive_paint_canvas_renderer.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/platform/graphics/paint_invalidation_reason.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_loader_options.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextra-semi"
#pragma clang diagnostic ignored "-Wignored-qualifiers"
#pragma clang diagnostic ignored "-Wshadow"
#include "third_party/rive/include/rive/animation/state_machine_input_instance.hpp"
#include "third_party/rive/include/rive/event.hpp"
#include "third_party/rive/include/rive/event_report.hpp"
#include "third_party/rive/include/rive/math/aabb.hpp"
#include "third_party/rive/include/rive/math/mat2d.hpp"
#include "third_party/rive/include/rive/math/vec2d.hpp"
#include "third_party/rive/include/rive/text/text_value_run.hpp"
#include "third_party/rive/include/rive/viewmodel/runtime/viewmodel_runtime.hpp"
#pragma clang diagnostic pop

namespace blink {

namespace {

class RiveFrameCallback final : public FrameCallback {
 public:
  explicit RiveFrameCallback(HTMLRiveElement* element) : element_(element) {}
  ~RiveFrameCallback() override = default;

  void Invoke(double high_res_now_ms) override {
    if (element_) {
      element_->OnAnimationFrame(high_res_now_ms);
    }
  }

  void Trace(Visitor* visitor) const override {
    visitor->Trace(element_);
    FrameCallback::Trace(visitor);
  }

 private:
  Member<HTMLRiveElement> element_;
};

const QualifiedName& ArtboardAttrName() {
  DEFINE_STATIC_LOCAL(QualifiedName, attr,
                      (g_null_atom, AtomicString("artboard"), g_null_atom));
  return attr;
}

const QualifiedName& StateMachineAttrName() {
  DEFINE_STATIC_LOCAL(
      QualifiedName, attr,
      (g_null_atom, AtomicString("state-machine"), g_null_atom));
  return attr;
}

const QualifiedName& AnimationAttrName() {
  DEFINE_STATIC_LOCAL(QualifiedName, attr,
                      (g_null_atom, AtomicString("animation"), g_null_atom));
  return attr;
}

const QualifiedName& FitAttrName() {
  DEFINE_STATIC_LOCAL(QualifiedName, attr,
                      (g_null_atom, AtomicString("fit"), g_null_atom));
  return attr;
}

const QualifiedName& AlignmentAttrName() {
  DEFINE_STATIC_LOCAL(QualifiedName, attr,
                      (g_null_atom, AtomicString("alignment"), g_null_atom));
  return attr;
}

const QualifiedName& ViewModelAttrName() {
  DEFINE_STATIC_LOCAL(QualifiedName, attr,
                      (g_null_atom, AtomicString("view-model"), g_null_atom));
  return attr;
}

}  // namespace

HTMLRiveElement::HTMLRiveElement(Document& document)
    : HTMLElement(html_names::kRivePlayerTag, document, kCreateHTMLElement) {}

HTMLRiveElement::~HTMLRiveElement() = default;

String HTMLRiveElement::src() const {
  return FastGetAttribute(html_names::kSrcAttr);
}

void HTMLRiveElement::setSrc(const String& value) {
  setAttribute(html_names::kSrcAttr, AtomicString(value));
}

String HTMLRiveElement::artboard() const {
  return FastGetAttribute(ArtboardAttrName());
}

void HTMLRiveElement::setArtboard(const String& value) {
  setAttribute(ArtboardAttrName(), AtomicString(value));
}

String HTMLRiveElement::stateMachine() const {
  return FastGetAttribute(StateMachineAttrName());
}

void HTMLRiveElement::setStateMachine(const String& value) {
  setAttribute(StateMachineAttrName(), AtomicString(value));
}

String HTMLRiveElement::animation() const {
  return FastGetAttribute(AnimationAttrName());
}

void HTMLRiveElement::setAnimation(const String& value) {
  setAttribute(AnimationAttrName(), AtomicString(value));
}

String HTMLRiveElement::fit() const {
  return FastGetAttribute(FitAttrName());
}

void HTMLRiveElement::setFit(const String& value) {
  setAttribute(FitAttrName(), AtomicString(value));
}

String HTMLRiveElement::alignment() const {
  return FastGetAttribute(AlignmentAttrName());
}

void HTMLRiveElement::setAlignment(const String& value) {
  setAttribute(AlignmentAttrName(), AtomicString(value));
}

String HTMLRiveElement::viewModel() const {
  return FastGetAttribute(ViewModelAttrName());
}

void HTMLRiveElement::setViewModel(const String& value) {
  setAttribute(ViewModelAttrName(), AtomicString(value));
}

bool HTMLRiveElement::autoplay() const {
  if (FastHasAttribute(html_names::kAutoplayAttr)) {
    const AtomicString& val = FastGetAttribute(html_names::kAutoplayAttr);
    return !EqualIgnoringASCIICase(val, "false");
  }
  return true;
}

void HTMLRiveElement::setAutoplay(bool value) {
  SetBooleanAttribute(html_names::kAutoplayAttr, value);
}

double HTMLRiveElement::artboardWidth() const {
  return artboard_ ? artboard_->width() : 0.0;
}

double HTMLRiveElement::artboardHeight() const {
  return artboard_ ? artboard_->height() : 0.0;
}

Vector<String> HTMLRiveElement::artboardNames() const {
  Vector<String> names;
  if (!rive_file_) {
    return names;
  }
  const size_t count = rive_file_->artboardCount();
  names.ReserveInitialCapacity(static_cast<wtf_size_t>(count));
  for (size_t i = 0; i < count; ++i) {
    names.push_back(String::FromUTF8(rive_file_->artboardNameAt(i)));
  }
  return names;
}

Vector<String> HTMLRiveElement::stateMachineNames() const {
  Vector<String> names;
  if (!artboard_) {
    return names;
  }
  const size_t count = artboard_->stateMachineCount();
  names.ReserveInitialCapacity(static_cast<wtf_size_t>(count));
  for (size_t i = 0; i < count; ++i) {
    names.push_back(String::FromUTF8(artboard_->stateMachineNameAt(i)));
  }
  return names;
}

Vector<String> HTMLRiveElement::animationNames() const {
  Vector<String> names;
  if (!artboard_) {
    return names;
  }
  const size_t count = artboard_->animationCount();
  names.ReserveInitialCapacity(static_cast<wtf_size_t>(count));
  for (size_t i = 0; i < count; ++i) {
    names.push_back(String::FromUTF8(artboard_->animationNameAt(i)));
  }
  return names;
}

Vector<String> HTMLRiveElement::inputNames() const {
  Vector<String> names;
  if (!state_machine_) {
    return names;
  }
  const size_t count = state_machine_->inputCount();
  names.ReserveInitialCapacity(static_cast<wtf_size_t>(count));
  for (size_t i = 0; i < count; ++i) {
    if (const rive::SMIInput* input = state_machine_->input(i)) {
      names.push_back(String::FromUTF8(input->name()));
    }
  }
  return names;
}

void HTMLRiveElement::play() {
  paused_ = false;
  last_frame_time_ms_ = 0.0;
  if (state_machine_) {
    state_machine_->markNeedsAdvance();
  }
  ScheduleAnimationIfNeeded();
}

void HTMLRiveElement::pause() {
  paused_ = true;
  CancelScheduledAnimation();
}

void HTMLRiveElement::stop() {
  paused_ = true;
  CancelScheduledAnimation();
  if (rive_file_) {
    InstantiateArtboardAndScene();
  }
}

bool HTMLRiveElement::advance(double seconds) {
  if (!artboard_) {
    return false;
  }
  bool keep_going = AdvanceInternal(static_cast<float>(seconds));
  InvalidateRivePaint();
  return keep_going;
}

bool HTMLRiveElement::loadFromBuffer(DOMArrayBuffer* data) {
  if (!data || data->ByteLength() == 0) {
    return false;
  }
  CancelLoader();
  return LoadFromBytes(data->ByteSpan());
}

bool HTMLRiveElement::setBooleanInput(const String& name, bool value) {
  if (!state_machine_) {
    return false;
  }
  rive::SMIBool* input = state_machine_->getBool(name.Utf8());
  if (!input) {
    return false;
  }
  input->value(value);
  ScheduleAnimationIfNeeded();
  return true;
}

bool HTMLRiveElement::getBooleanInput(const String& name) const {
  if (!state_machine_) {
    return false;
  }
  const rive::SMIBool* input = state_machine_->getBool(name.Utf8());
  return input ? input->value() : false;
}

bool HTMLRiveElement::setNumberInput(const String& name, double value) {
  if (!state_machine_) {
    return false;
  }
  rive::SMINumber* input = state_machine_->getNumber(name.Utf8());
  if (!input) {
    return false;
  }
  input->value(static_cast<float>(value));
  ScheduleAnimationIfNeeded();
  return true;
}

double HTMLRiveElement::getNumberInput(const String& name) const {
  if (!state_machine_) {
    return 0.0;
  }
  const rive::SMINumber* input = state_machine_->getNumber(name.Utf8());
  return input ? input->value() : 0.0;
}

bool HTMLRiveElement::fireTrigger(const String& name) {
  if (!state_machine_) {
    return false;
  }
  rive::SMITrigger* trigger = state_machine_->getTrigger(name.Utf8());
  if (!trigger) {
    return false;
  }
  trigger->fire();
  ScheduleAnimationIfNeeded();
  return true;
}

bool HTMLRiveElement::setTextRun(const String& name, const String& value) {
  if (!artboard_) {
    return false;
  }
  rive::TextValueRun* run = artboard_->find<rive::TextValueRun>(name.Utf8());
  if (!run) {
    return false;
  }
  run->text(value.Utf8());
  if (state_machine_) {
    state_machine_->markNeedsAdvance();
  }
  ScheduleAnimationIfNeeded();
  InvalidateRivePaint();
  return true;
}

String HTMLRiveElement::getTextRun(const String& name) const {
  if (!artboard_) {
    return g_empty_string;
  }
  rive::TextValueRun* run = artboard_->find<rive::TextValueRun>(name.Utf8());
  return run ? String::FromUTF8(run->text()) : g_empty_string;
}

bool HTMLRiveElement::setViewModelNumber(const String& path, double value) {
  if (!view_model_instance_) {
    return false;
  }
  auto* prop = view_model_instance_->propertyNumber(path.Utf8());
  if (!prop) {
    return false;
  }
  prop->value(static_cast<float>(value));
  if (state_machine_) {
    state_machine_->markNeedsAdvance();
  }
  ScheduleAnimationIfNeeded();
  InvalidateRivePaint();
  return true;
}

double HTMLRiveElement::getViewModelNumber(const String& path) const {
  if (!view_model_instance_) {
    return 0.0;
  }
  auto* prop = view_model_instance_->propertyNumber(path.Utf8());
  return prop ? prop->value() : 0.0;
}

bool HTMLRiveElement::setViewModelBoolean(const String& path, bool value) {
  if (!view_model_instance_) {
    return false;
  }
  auto* prop = view_model_instance_->propertyBoolean(path.Utf8());
  if (!prop) {
    return false;
  }
  prop->value(value);
  if (state_machine_) {
    state_machine_->markNeedsAdvance();
  }
  ScheduleAnimationIfNeeded();
  InvalidateRivePaint();
  return true;
}

bool HTMLRiveElement::getViewModelBoolean(const String& path) const {
  if (!view_model_instance_) {
    return false;
  }
  auto* prop = view_model_instance_->propertyBoolean(path.Utf8());
  return prop ? prop->value() : false;
}

bool HTMLRiveElement::setViewModelString(const String& path,
                                         const String& value) {
  if (!view_model_instance_) {
    return false;
  }
  auto* prop = view_model_instance_->propertyString(path.Utf8());
  if (!prop) {
    return false;
  }
  prop->value(value.Utf8());
  if (state_machine_) {
    state_machine_->markNeedsAdvance();
  }
  ScheduleAnimationIfNeeded();
  InvalidateRivePaint();
  return true;
}

String HTMLRiveElement::getViewModelString(const String& path) const {
  if (!view_model_instance_) {
    return g_empty_string;
  }
  auto* prop = view_model_instance_->propertyString(path.Utf8());
  return prop ? String::FromUTF8(prop->value()) : g_empty_string;
}

bool HTMLRiveElement::setViewModelColor(const String& path, int32_t argb) {
  if (!view_model_instance_) {
    return false;
  }
  auto* prop = view_model_instance_->propertyColor(path.Utf8());
  if (!prop) {
    return false;
  }
  prop->value(argb);
  if (state_machine_) {
    state_machine_->markNeedsAdvance();
  }
  ScheduleAnimationIfNeeded();
  InvalidateRivePaint();
  return true;
}

int32_t HTMLRiveElement::getViewModelColor(const String& path) const {
  if (!view_model_instance_) {
    return 0;
  }
  auto* prop = view_model_instance_->propertyColor(path.Utf8());
  return prop ? prop->value() : 0;
}

bool HTMLRiveElement::setViewModelEnum(const String& path,
                                       const String& value) {
  if (!view_model_instance_) {
    return false;
  }
  auto* prop = view_model_instance_->propertyEnum(path.Utf8());
  if (!prop) {
    return false;
  }
  prop->value(value.Utf8());
  if (state_machine_) {
    state_machine_->markNeedsAdvance();
  }
  ScheduleAnimationIfNeeded();
  InvalidateRivePaint();
  return true;
}

String HTMLRiveElement::getViewModelEnum(const String& path) const {
  if (!view_model_instance_) {
    return g_empty_string;
  }
  auto* prop = view_model_instance_->propertyEnum(path.Utf8());
  return prop ? String::FromUTF8(prop->value()) : g_empty_string;
}

bool HTMLRiveElement::fireViewModelTrigger(const String& path) {
  if (!view_model_instance_) {
    return false;
  }
  auto* prop = view_model_instance_->propertyTrigger(path.Utf8());
  if (!prop) {
    return false;
  }
  prop->trigger();
  if (state_machine_) {
    state_machine_->markNeedsAdvance();
  }
  ScheduleAnimationIfNeeded();
  InvalidateRivePaint();
  return true;
}

gfx::SizeF HTMLRiveElement::ArtboardSize() const {
  if (artboard_ && artboard_->width() > 0 && artboard_->height() > 0) {
    return gfx::SizeF(artboard_->width(), artboard_->height());
  }
  return gfx::SizeF(LayoutReplaced::kDefaultWidth,
                    LayoutReplaced::kDefaultHeight);
}

void HTMLRiveElement::PaintRive(cc::PaintCanvas* canvas,
                                const gfx::SizeF& box_size) {
  if (!canvas || !artboard_ || box_size.IsEmpty()) {
    return;
  }

  RivePaintCanvasRenderer renderer(canvas);
  renderer.save();
  const rive::AABB frame(0.0f, 0.0f, box_size.width(), box_size.height());
  renderer.align(GetRiveFit(), GetRiveAlignment(), frame, artboard_->bounds());
  artboard_->draw(&renderer);
  renderer.restore();
}

void HTMLRiveElement::OnAnimationFrame(double high_res_now_ms) {
  raf_callback_id_ = 0;
  if (paused_ || !artboard_ || !isConnected()) {
    return;
  }

  float elapsed_seconds = 0.0f;
  if (last_frame_time_ms_ > 0.0 && high_res_now_ms > last_frame_time_ms_) {
    elapsed_seconds =
        static_cast<float>((high_res_now_ms - last_frame_time_ms_) / 1000.0);
    elapsed_seconds = std::clamp(elapsed_seconds, 0.0f, 0.1f);
  }
  last_frame_time_ms_ = high_res_now_ms;

  bool keep_going = AdvanceInternal(elapsed_seconds);
  InvalidateRivePaint();

  if (keep_going || (state_machine_ && state_machine_->needsAdvance())) {
    ScheduleAnimationIfNeeded();
  } else {
    last_frame_time_ms_ = 0.0;
  }
}

void HTMLRiveElement::ParseAttribute(
    const AttributeModificationParams& params) {
  if (params.name == html_names::kSrcAttr) {
    if (isConnected()) {
      StartLoadingSrc(params.new_value);
    }
  } else if (params.name == ArtboardAttrName() ||
             params.name == StateMachineAttrName() ||
             params.name == AnimationAttrName() ||
             params.name == ViewModelAttrName()) {
    if (rive_file_) {
      InstantiateArtboardAndScene();
    }
  } else if (params.name == FitAttrName() ||
             params.name == AlignmentAttrName()) {
    InvalidateRivePaint();
  } else if (params.name == html_names::kAutoplayAttr) {
    if (autoplay() && !paused_) {
      ScheduleAnimationIfNeeded();
    }
  } else {
    HTMLElement::ParseAttribute(params);
  }
}

LayoutObject* HTMLRiveElement::CreateLayoutObject(const ComputedStyle& style) {
  return MakeGarbageCollected<LayoutRive>(this);
}

Node::InsertionNotificationRequest HTMLRiveElement::InsertedInto(
    ContainerNode& insertion_point) {
  HTMLElement::InsertedInto(insertion_point);
  if (insertion_point.isConnected()) {
    const String current_src = src();
    if (!current_src.empty() && current_src != loaded_url_) {
      StartLoadingSrc(current_src);
    } else if (artboard_ && autoplay() && !paused_) {
      ScheduleAnimationIfNeeded();
    }
  }
  return kInsertionDone;
}

void HTMLRiveElement::RemovedFrom(ContainerNode& insertion_point) {
  CancelScheduledAnimation();
  CancelLoader();
  HTMLElement::RemovedFrom(insertion_point);
}

void HTMLRiveElement::DefaultEventHandler(Event& event) {
  if (state_machine_ && event.IsMouseEvent()) {
    auto* mouse_event = To<MouseEvent>(&event);
    rive::Vec2D artboard_point;
    if (MapOffsetToArtboard(static_cast<float>(mouse_event->offsetX()),
                            static_cast<float>(mouse_event->offsetY()),
                            &artboard_point)) {
      int pointer_id = 0;
      if (event.IsPointerEvent()) {
        pointer_id = To<PointerEvent>(&event)->pointerId();
      }
      const AtomicString& type = event.type();
      bool handled = false;
      if (type == event_type_names::kPointerdown ||
          type == event_type_names::kMousedown) {
        state_machine_->pointerDown(artboard_point, pointer_id);
        handled = true;
      } else if (type == event_type_names::kPointermove ||
                 type == event_type_names::kMousemove) {
        state_machine_->pointerMove(artboard_point, /*timeStamp=*/0.0f,
                                    pointer_id);
        handled = true;
      } else if (type == event_type_names::kPointerup ||
                 type == event_type_names::kMouseup) {
        state_machine_->pointerUp(artboard_point, pointer_id);
        handled = true;
      } else if (type == event_type_names::kPointerout ||
                 type == event_type_names::kPointerleave ||
                 type == event_type_names::kMouseout ||
                 type == event_type_names::kMouseleave) {
        state_machine_->pointerExit(artboard_point, pointer_id);
        handled = true;
      }
      if (handled) {
        ScheduleAnimationIfNeeded();
        InvalidateRivePaint();
      }
    }
  }
  HTMLElement::DefaultEventHandler(event);
}

void HTMLRiveElement::DidReceiveData(base::span<const char> data) {
  pending_bytes_.AppendSpan(base::as_bytes(data));
}

void HTMLRiveElement::DidFinishLoading(uint64_t identifier) {
  loader_ = nullptr;
  Vector<uint8_t> bytes = std::move(pending_bytes_);
  pending_bytes_.clear();
  if (LoadFromBytes(bytes)) {
    DispatchEvent(*Event::Create(event_type_names::kLoad));
  } else {
    DispatchEvent(*Event::Create(event_type_names::kError));
  }
}

void HTMLRiveElement::DidFail(uint64_t identifier,
                              const ResourceError& error) {
  loader_ = nullptr;
  pending_bytes_.clear();
  if (!error.IsCancellation()) {
    DispatchEvent(*Event::Create(event_type_names::kError));
  }
}

void HTMLRiveElement::DidFailRedirectCheck(uint64_t identifier) {
  loader_ = nullptr;
  pending_bytes_.clear();
  DispatchEvent(*Event::Create(event_type_names::kError));
}

void HTMLRiveElement::StartLoadingSrc(const String& url_string) {
  CancelLoader();
  if (url_string.empty() || !GetExecutionContext()) {
    return;
  }

  KURL url = GetDocument().CompleteURL(url_string);
  if (!url.IsValid()) {
    DispatchEvent(*Event::Create(event_type_names::kError));
    return;
  }

  loaded_url_ = url_string;
  ResourceRequest request(url);
  request.SetMode(network::mojom::RequestMode::kCors);
  request.SetTargetAddressSpace(network::mojom::IPAddressSpace::kUnknown);
  request.SetCredentialsMode(network::mojom::CredentialsMode::kSameOrigin);
  request.SetRequestContext(mojom::blink::RequestContextType::FETCH);

  ResourceLoaderOptions options(GetExecutionContext()->GetCurrentWorld());
  loader_ = MakeGarbageCollected<ThreadableLoader>(*GetExecutionContext(), this,
                                                   options);
  loader_->Start(std::move(request));
}

void HTMLRiveElement::CancelLoader() {
  if (loader_) {
    loader_->Cancel();
    loader_ = nullptr;
  }
  pending_bytes_.clear();
}

bool HTMLRiveElement::LoadFromBytes(base::span<const uint8_t> bytes) {
  CancelScheduledAnimation();
  state_machine_.reset();
  animation_instance_.reset();
  view_model_instance_.reset();
  artboard_.reset();
  rive_file_.reset();

  if (bytes.empty()) {
    return false;
  }

  rive::ImportResult result = rive::ImportResult::malformed;
  rive_file_ = rive::File::import(
      rive::Span<const uint8_t>(bytes.data(), bytes.size()),
      &RivePaintCanvasFactory::Instance(), &result);
  if (!rive_file_ || result != rive::ImportResult::success) {
    rive_file_.reset();
    return false;
  }

  InstantiateArtboardAndScene();
  return artboard_ != nullptr;
}

void HTMLRiveElement::InstantiateArtboardAndScene() {
  CancelScheduledAnimation();
  state_machine_.reset();
  animation_instance_.reset();
  view_model_instance_.reset();
  artboard_.reset();

  if (!rive_file_) {
    return;
  }

  const String requested_artboard = artboard();
  if (!requested_artboard.empty()) {
    artboard_ = rive_file_->artboardNamed(requested_artboard.Utf8());
  }
  if (!artboard_) {
    artboard_ = rive_file_->artboardDefault();
  }
  if (!artboard_) {
    artboard_ = rive_file_->artboardAt(0);
  }
  if (!artboard_) {
    return;
  }

  const String requested_sm = stateMachine();
  const String requested_anim = animation();

  if (!requested_sm.empty()) {
    state_machine_ = artboard_->stateMachineNamed(requested_sm.Utf8());
  } else if (!requested_anim.empty()) {
    animation_instance_ = artboard_->animationNamed(requested_anim.Utf8());
  }

  if (!state_machine_ && !animation_instance_) {
    state_machine_ = artboard_->defaultStateMachine();
    if (!state_machine_ && artboard_->stateMachineCount() > 0) {
      state_machine_ = artboard_->stateMachineAt(0);
    }
    if (!state_machine_ && artboard_->animationCount() > 0) {
      animation_instance_ = artboard_->animationAt(0);
    }
  }

  // Bind ViewModel instance if available in the .riv file.
  rive::ViewModelRuntime* vm_runtime = nullptr;
  const String requested_vm = viewModel();
  if (!requested_vm.empty()) {
    vm_runtime = rive_file_->viewModelByName(requested_vm.Utf8());
  }
  if (!vm_runtime) {
    vm_runtime = rive_file_->defaultArtboardViewModel(artboard_.get());
  }
  if (vm_runtime) {
    view_model_instance_ = vm_runtime->createDefaultInstance();
    if (!view_model_instance_) {
      view_model_instance_ = vm_runtime->createInstance();
    }
    if (view_model_instance_ && view_model_instance_->instance()) {
      if (state_machine_) {
        state_machine_->bindViewModelInstance(view_model_instance_->instance());
      } else {
        artboard_->bindViewModelInstance(view_model_instance_->instance());
      }
    }
  }

  // Advance by 0s to apply the initial frame pose.
  AdvanceInternal(0.0f);

  if (auto* layout_rive = static_cast<LayoutRive*>(GetLayoutObject())) {
    layout_rive->IntrinsicSizeChanged();
  }
  InvalidateRivePaint();

  if (autoplay() && !paused_) {
    ScheduleAnimationIfNeeded();
  }
}

bool HTMLRiveElement::AdvanceInternal(float elapsed_seconds) {
  if (!artboard_) {
    return false;
  }

  if (state_machine_) {
    bool keep_going = state_machine_->advanceAndApply(elapsed_seconds);
    const size_t event_count = state_machine_->reportedEventCount();
    for (size_t i = 0; i < event_count; ++i) {
      const rive::EventReport report = state_machine_->reportedEventAt(i);
      if (report.event()) {
        last_event_name_ = String::FromUTF8(report.event()->name());
        DispatchEvent(*Event::CreateBubble(AtomicString("riveevent")));
      }
    }
    return keep_going;
  }

  if (animation_instance_) {
    bool keep_going = animation_instance_->advance(elapsed_seconds);
    animation_instance_->apply();
    artboard_->advance(elapsed_seconds);
    return keep_going;
  }

  return artboard_->advance(elapsed_seconds);
}

void HTMLRiveElement::ScheduleAnimationIfNeeded() {
  if (paused_ || !artboard_ || !isConnected() || raf_callback_id_ != 0) {
    return;
  }
  raf_callback_id_ = GetDocument().RequestAnimationFrame(
      MakeGarbageCollected<RiveFrameCallback>(this));
}

void HTMLRiveElement::CancelScheduledAnimation() {
  if (raf_callback_id_ != 0) {
    GetDocument().CancelAnimationFrame(raf_callback_id_);
    raf_callback_id_ = 0;
  }
  last_frame_time_ms_ = 0.0;
}

void HTMLRiveElement::InvalidateRivePaint() {
  if (LayoutObject* layout_object = GetLayoutObject()) {
    layout_object->SetShouldDoFullPaintInvalidationWithoutLayoutChange(
        PaintInvalidationReason::kImage);
  }
}

rive::Fit HTMLRiveElement::GetRiveFit() const {
  const String f = fit();
  if (EqualIgnoringASCIICase(f, "fill")) {
    return rive::Fit::fill;
  }
  if (EqualIgnoringASCIICase(f, "cover")) {
    return rive::Fit::cover;
  }
  if (EqualIgnoringASCIICase(f, "fitwidth") ||
      EqualIgnoringASCIICase(f, "fit-width")) {
    return rive::Fit::fitWidth;
  }
  if (EqualIgnoringASCIICase(f, "fitheight") ||
      EqualIgnoringASCIICase(f, "fit-height")) {
    return rive::Fit::fitHeight;
  }
  if (EqualIgnoringASCIICase(f, "none")) {
    return rive::Fit::none;
  }
  if (EqualIgnoringASCIICase(f, "scaledown") ||
      EqualIgnoringASCIICase(f, "scale-down")) {
    return rive::Fit::scaleDown;
  }
  if (EqualIgnoringASCIICase(f, "layout")) {
    return rive::Fit::layout;
  }
  return rive::Fit::contain;
}

rive::Alignment HTMLRiveElement::GetRiveAlignment() const {
  const String a = alignment();
  if (EqualIgnoringASCIICase(a, "topleft") ||
      EqualIgnoringASCIICase(a, "top-left")) {
    return rive::Alignment::topLeft;
  }
  if (EqualIgnoringASCIICase(a, "topcenter") ||
      EqualIgnoringASCIICase(a, "top-center")) {
    return rive::Alignment::topCenter;
  }
  if (EqualIgnoringASCIICase(a, "topright") ||
      EqualIgnoringASCIICase(a, "top-right")) {
    return rive::Alignment::topRight;
  }
  if (EqualIgnoringASCIICase(a, "centerleft") ||
      EqualIgnoringASCIICase(a, "center-left")) {
    return rive::Alignment::centerLeft;
  }
  if (EqualIgnoringASCIICase(a, "centerright") ||
      EqualIgnoringASCIICase(a, "center-right")) {
    return rive::Alignment::centerRight;
  }
  if (EqualIgnoringASCIICase(a, "bottomleft") ||
      EqualIgnoringASCIICase(a, "bottom-left")) {
    return rive::Alignment::bottomLeft;
  }
  if (EqualIgnoringASCIICase(a, "bottomcenter") ||
      EqualIgnoringASCIICase(a, "bottom-center")) {
    return rive::Alignment::bottomCenter;
  }
  if (EqualIgnoringASCIICase(a, "bottomright") ||
      EqualIgnoringASCIICase(a, "bottom-right")) {
    return rive::Alignment::bottomRight;
  }
  return rive::Alignment::center;
}

bool HTMLRiveElement::MapOffsetToArtboard(float offset_x,
                                          float offset_y,
                                          rive::Vec2D* out_point) const {
  if (!artboard_ || !out_point) {
    return false;
  }
  auto* layout_rive = static_cast<const LayoutRive*>(GetLayoutObject());
  if (!layout_rive) {
    return false;
  }
  PhysicalRect content_rect = layout_rive->ReplacedContentRect();
  if (content_rect.IsEmpty()) {
    return false;
  }

  const rive::AABB frame(0.0f, 0.0f, content_rect.Width().ToFloat(),
                         content_rect.Height().ToFloat());
  const rive::Mat2D align_matrix = rive::computeAlignment(
      GetRiveFit(), GetRiveAlignment(), frame, artboard_->bounds());
  rive::Mat2D inverse;
  if (!align_matrix.invert(&inverse)) {
    return false;
  }
  const float local_x = offset_x - content_rect.X().ToFloat();
  const float local_y = offset_y - content_rect.Y().ToFloat();
  *out_point = inverse * rive::Vec2D(local_x, local_y);
  return true;
}

void HTMLRiveElement::Trace(Visitor* visitor) const {
  visitor->Trace(loader_);
  HTMLElement::Trace(visitor);
}

}  // namespace blink

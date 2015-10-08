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

#include "cobalt/dom/event_target.h"

#include "cobalt/base/polymorphic_downcast.h"
#include "cobalt/dom/dom_exception.h"
#include "cobalt/dom/testing/mock_event_listener.h"
#include "cobalt/script/testing/mock_exception_state.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cobalt {
namespace dom {
namespace {

using ::testing::AllOf;
using ::testing::Eq;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;
using ::testing::Pointee;
using ::testing::Property;
using ::testing::SaveArg;
using ::testing::StrictMock;
using ::testing::_;
using script::testing::MockExceptionState;
using testing::FakeScriptObject;
using testing::MockEventListener;

void ExpectHandleEventCall(const MockEventListener* listener,
                           const scoped_refptr<Event>& event,
                           const scoped_refptr<EventTarget>& target) {
  // Note that we must pass the raw pointer to avoid reference counting issue.
  EXPECT_CALL(
      *listener,
      HandleEvent(AllOf(
          Eq(event.get()), Pointee(Property(&Event::target, Eq(target.get()))),
          Pointee(Property(&Event::current_target, Eq(target.get()))),
          Pointee(Property(&Event::event_phase, Eq(Event::kAtTarget))),
          Pointee(Property(&Event::IsBeingDispatched, Eq(true))))))
      .RetiresOnSaturation();
}

void ExpectHandleEventCall(const MockEventListener* listener,
                           const scoped_refptr<Event>& event,
                           const scoped_refptr<EventTarget>& target,
                           void (Event::*function)(void)) {
  // Note that we must pass the raw pointer to avoid reference counting issue.
  EXPECT_CALL(
      *listener,
      HandleEvent(AllOf(
          Eq(event.get()), Pointee(Property(&Event::target, Eq(target.get()))),
          Pointee(Property(&Event::current_target, Eq(target.get()))),
          Pointee(Property(&Event::event_phase, Eq(Event::kAtTarget))),
          Pointee(Property(&Event::IsBeingDispatched, Eq(true))))))
      .WillOnce(InvokeWithoutArgs(event.get(), function))
      .RetiresOnSaturation();
}

void ExpectNoHandleEventCall(const MockEventListener* listener) {
  EXPECT_CALL(*listener, HandleEvent(_)).Times(0);
}

void DispatchEventOnCurrentTarget(const scoped_refptr<Event>& event) {
  StrictMock<MockExceptionState> exception_state;
  scoped_refptr<script::ScriptException> exception;

  EXPECT_TRUE(event->IsBeingDispatched());

  EXPECT_CALL(exception_state, SetException(_))
      .WillOnce(SaveArg<0>(&exception));
  event->current_target()->DispatchEvent(event, &exception_state);

  ASSERT_TRUE(exception);
  EXPECT_EQ(DOMException::kInvalidStateErr,
            base::polymorphic_downcast<DOMException*>(exception.get())->code());
}

TEST(EventTargetTest, SingleEventListenerFired) {
  StrictMock<MockExceptionState> exception_state;
  scoped_refptr<EventTarget> event_target = new EventTarget;
  scoped_refptr<Event> event = new Event("fired");
  scoped_ptr<MockEventListener> event_listener =
      MockEventListener::CreateAsNonAttribute();

  ExpectHandleEventCall(event_listener.get(), event, event_target);
  event_target->AddEventListener("fired",
                                 FakeScriptObject(event_listener.get()), false);
  EXPECT_TRUE(event_target->DispatchEvent(event, &exception_state));
}

TEST(EventTargetTest, SingleEventListenerNotFired) {
  StrictMock<MockExceptionState> exception_state;
  scoped_refptr<EventTarget> event_target = new EventTarget;
  scoped_refptr<Event> event = new Event("fired");
  scoped_ptr<MockEventListener> event_listener =
      MockEventListener::CreateAsNonAttribute();

  ExpectNoHandleEventCall(event_listener.get());
  event_target->AddEventListener("notfired",
                                 FakeScriptObject(event_listener.get()), false);
  EXPECT_TRUE(event_target->DispatchEvent(event, &exception_state));
}

// Test if multiple event listeners of different event types can be added and
// fired properly.
TEST(EventTargetTest, MultipleEventListeners) {
  StrictMock<MockExceptionState> exception_state;
  scoped_refptr<EventTarget> event_target = new EventTarget;
  scoped_refptr<Event> event = new Event("fired");
  scoped_ptr<MockEventListener> event_listener_fired_1 =
      MockEventListener::CreateAsNonAttribute();
  scoped_ptr<MockEventListener> event_listener_fired_2 =
      MockEventListener::CreateAsNonAttribute();
  scoped_ptr<MockEventListener> event_listener_not_fired =
      MockEventListener::CreateAsNonAttribute();

  InSequence in_sequence;
  ExpectHandleEventCall(event_listener_fired_1.get(), event, event_target);
  ExpectHandleEventCall(event_listener_fired_2.get(), event, event_target);
  ExpectNoHandleEventCall(event_listener_not_fired.get());

  event_target->AddEventListener(
      "fired", FakeScriptObject(event_listener_fired_1.get()), false);
  event_target->AddEventListener(
      "notfired", FakeScriptObject(event_listener_not_fired.get()), false);
  event_target->AddEventListener(
      "fired", FakeScriptObject(event_listener_fired_2.get()), true);
  EXPECT_TRUE(event_target->DispatchEvent(event, &exception_state));
}

// Test if event listener can be added and later removed.
TEST(EventTargetTest, AddRemoveEventListener) {
  StrictMock<MockExceptionState> exception_state;
  scoped_refptr<EventTarget> event_target = new EventTarget;
  scoped_refptr<Event> event = new Event("fired");
  scoped_ptr<MockEventListener> event_listener =
      MockEventListener::CreateAsNonAttribute();

  ExpectHandleEventCall(event_listener.get(), event, event_target);
  event_target->AddEventListener("fired",
                                 FakeScriptObject(event_listener.get()), false);
  EXPECT_TRUE(event_target->DispatchEvent(event, &exception_state));

  ExpectNoHandleEventCall(event_listener.get());
  event_target->RemoveEventListener(
      "fired", FakeScriptObject(event_listener.get()), false);
  EXPECT_TRUE(event_target->DispatchEvent(event, &exception_state));

  ExpectHandleEventCall(event_listener.get(), event, event_target);
  event_target->AddEventListener("fired",
                                 FakeScriptObject(event_listener.get()), false);
  EXPECT_TRUE(event_target->DispatchEvent(event, &exception_state));
}

// Test if attribute event listener works.
TEST(EventTargetTest, AttributeListener) {
  StrictMock<MockExceptionState> exception_state;
  scoped_refptr<EventTarget> event_target = new EventTarget;
  scoped_refptr<Event> event = new Event("fired");
  scoped_ptr<MockEventListener> non_attribute_event_listener =
      MockEventListener::CreateAsNonAttribute();
  scoped_ptr<MockEventListener> attribute_event_listener_1 =
      MockEventListener::CreateAsAttribute();
  scoped_ptr<MockEventListener> attribute_event_listener_2 =
      MockEventListener::CreateAsAttribute();

  event_target->AddEventListener(
      "fired", FakeScriptObject(non_attribute_event_listener.get()), false);

  ExpectHandleEventCall(non_attribute_event_listener.get(), event,
                        event_target);
  ExpectHandleEventCall(attribute_event_listener_1.get(), event, event_target);
  event_target->SetAttributeEventListener(
      "fired", FakeScriptObject(attribute_event_listener_1.get()));
  EXPECT_TRUE(event_target->DispatchEvent(event, &exception_state));

  ExpectHandleEventCall(non_attribute_event_listener.get(), event,
                        event_target);
  ExpectNoHandleEventCall(attribute_event_listener_1.get());
  ExpectHandleEventCall(attribute_event_listener_2.get(), event, event_target);
  event_target->SetAttributeEventListener(
      "fired", FakeScriptObject(attribute_event_listener_2.get()));
  EXPECT_TRUE(event_target->DispatchEvent(event, &exception_state));

  ExpectHandleEventCall(non_attribute_event_listener.get(), event,
                        event_target);
  ExpectNoHandleEventCall(attribute_event_listener_1.get());
  ExpectNoHandleEventCall(attribute_event_listener_2.get());
  event_target->SetAttributeEventListener("fired", FakeScriptObject(NULL));
  EXPECT_TRUE(event_target->DispatchEvent(event, &exception_state));
}

// Test if one event listener can be used by multiple events.
TEST(EventTargetTest, EventListenerReuse) {
  StrictMock<MockExceptionState> exception_state;
  scoped_refptr<EventTarget> event_target = new EventTarget;
  scoped_refptr<Event> event_1 = new Event("fired_1");
  scoped_refptr<Event> event_2 = new Event("fired_2");
  scoped_ptr<MockEventListener> event_listener =
      MockEventListener::CreateAsNonAttribute();

  ExpectHandleEventCall(event_listener.get(), event_1, event_target);
  ExpectHandleEventCall(event_listener.get(), event_2, event_target);
  event_target->AddEventListener("fired_1",
                                 FakeScriptObject(event_listener.get()), false);
  event_target->AddEventListener("fired_2",
                                 FakeScriptObject(event_listener.get()), false);
  EXPECT_TRUE(event_target->DispatchEvent(event_1, &exception_state));
  EXPECT_TRUE(event_target->DispatchEvent(event_2, &exception_state));

  ExpectHandleEventCall(event_listener.get(), event_1, event_target);
  event_target->RemoveEventListener(
      "fired_2", FakeScriptObject(event_listener.get()), false);
  EXPECT_TRUE(event_target->DispatchEvent(event_1, &exception_state));
  EXPECT_TRUE(event_target->DispatchEvent(event_2, &exception_state));

  ExpectHandleEventCall(event_listener.get(), event_1, event_target);
  // The capture flag is not the same so the event will not be removed.
  event_target->RemoveEventListener(
      "fired_1", FakeScriptObject(event_listener.get()), true);
  EXPECT_TRUE(event_target->DispatchEvent(event_1, &exception_state));
  EXPECT_TRUE(event_target->DispatchEvent(event_2, &exception_state));

  ExpectNoHandleEventCall(event_listener.get());
  event_target->RemoveEventListener(
      "fired_1", FakeScriptObject(event_listener.get()), false);
  EXPECT_TRUE(event_target->DispatchEvent(event_1, &exception_state));
  EXPECT_TRUE(event_target->DispatchEvent(event_2, &exception_state));
}

TEST(EventTargetTest, StopPropagation) {
  StrictMock<MockExceptionState> exception_state;
  scoped_refptr<EventTarget> event_target = new EventTarget;
  scoped_refptr<Event> event = new Event("fired");
  scoped_ptr<MockEventListener> event_listener_fired_1 =
      MockEventListener::CreateAsNonAttribute();
  scoped_ptr<MockEventListener> event_listener_fired_2 =
      MockEventListener::CreateAsNonAttribute();

  InSequence in_sequence;
  ExpectHandleEventCall(event_listener_fired_1.get(), event, event_target,
                        &Event::StopPropagation);
  ExpectHandleEventCall(event_listener_fired_2.get(), event, event_target);

  event_target->AddEventListener(
      "fired", FakeScriptObject(event_listener_fired_1.get()), false);
  event_target->AddEventListener(
      "fired", FakeScriptObject(event_listener_fired_2.get()), true);
  EXPECT_TRUE(event_target->DispatchEvent(event, &exception_state));
}

TEST(EventTargetTest, StopImmediatePropagation) {
  StrictMock<MockExceptionState> exception_state;
  scoped_refptr<EventTarget> event_target = new EventTarget;
  scoped_refptr<Event> event = new Event("fired");
  scoped_ptr<MockEventListener> event_listener_fired_1 =
      MockEventListener::CreateAsNonAttribute();
  scoped_ptr<MockEventListener> event_listener_fired_2 =
      MockEventListener::CreateAsNonAttribute();

  ExpectHandleEventCall(event_listener_fired_1.get(), event, event_target,
                        &Event::StopImmediatePropagation);
  ExpectNoHandleEventCall(event_listener_fired_2.get());

  event_target->AddEventListener(
      "fired", FakeScriptObject(event_listener_fired_1.get()), false);
  event_target->AddEventListener(
      "fired", FakeScriptObject(event_listener_fired_2.get()), true);
  EXPECT_TRUE(event_target->DispatchEvent(event, &exception_state));
}

TEST(EventTargetTest, PreventDefault) {
  StrictMock<MockExceptionState> exception_state;
  scoped_refptr<Event> event;
  scoped_refptr<EventTarget> event_target = new EventTarget;
  scoped_ptr<MockEventListener> event_listener_fired =
      MockEventListener::CreateAsNonAttribute();

  event_target->AddEventListener(
      "fired", FakeScriptObject(event_listener_fired.get()), false);
  event = new Event("fired", Event::kNotBubbles, Event::kNotCancelable);
  ExpectHandleEventCall(event_listener_fired.get(), event, event_target,
                        &Event::PreventDefault);
  EXPECT_TRUE(event_target->DispatchEvent(event, &exception_state));

  event = new Event("fired", Event::kNotBubbles, Event::kCancelable);
  ExpectHandleEventCall(event_listener_fired.get(), event, event_target,
                        &Event::PreventDefault);
  EXPECT_FALSE(event_target->DispatchEvent(event, &exception_state));
}

TEST(EventTargetTest, RaiseException) {
  StrictMock<MockExceptionState> exception_state;
  scoped_refptr<script::ScriptException> exception;
  scoped_refptr<EventTarget> event_target = new EventTarget;
  scoped_refptr<Event> event;
  scoped_ptr<MockEventListener> event_listener =
      MockEventListener::CreateAsNonAttribute();

  EXPECT_CALL(exception_state, SetException(_))
      .WillOnce(SaveArg<0>(&exception));
  // Dispatch a NULL event.
  event_target->DispatchEvent(NULL, &exception_state);
  ASSERT_TRUE(exception);
  EXPECT_EQ(DOMException::kInvalidStateErr,
            base::polymorphic_downcast<DOMException*>(exception.get())->code());
  exception = NULL;

  EXPECT_CALL(exception_state, SetException(_))
      .WillOnce(SaveArg<0>(&exception));
  // Dispatch an uninitialized event.
  event_target->DispatchEvent(new Event(Event::Uninitialized),
                              &exception_state);
  ASSERT_TRUE(exception);
  EXPECT_EQ(DOMException::kInvalidStateErr,
            base::polymorphic_downcast<DOMException*>(exception.get())->code());
  exception = NULL;

  event_target->AddEventListener("fired",
                                 FakeScriptObject(event_listener.get()), false);
  event = new Event("fired", Event::kNotBubbles, Event::kNotCancelable);
  // Dispatch event again when it is being dispatched.
  EXPECT_CALL(*event_listener, HandleEvent(_))
      .WillOnce(Invoke(DispatchEventOnCurrentTarget));
  EXPECT_TRUE(event_target->DispatchEvent(event, &exception_state));
}

}  // namespace
}  // namespace dom
}  // namespace cobalt

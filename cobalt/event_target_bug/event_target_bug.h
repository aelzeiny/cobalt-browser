// Copyright 2023 The Cobalt Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef COBALT_EVENT_TARGET_BUG_EVENT_TARGET_BUG_H_
#define COBALT_EVENT_TARGET_BUG_EVENT_TARGET_BUG_H_

#include <memory>
#include <vector>

#include "base/memory/singleton.h"
#include "base/timer/timer.h"
#include "cobalt/script/environment_settings.h"
#include "cobalt/script/javascript_engine.h"
#include "cobalt/script/wrappable.h"
#include "cobalt/web/context.h"
#include "cobalt/web/environment_settings.h"
#include "cobalt/web/environment_settings_helper.h"
#include "cobalt/web/event_target.h"
#include "starboard/common/log.h"

namespace cobalt {
namespace event_target_bug {

/**
 * Bug explained:
 * `EventTargetBug` is a class that is bound to the window but never Garbage
 * Collected. This is because a global singleton maintains a scoped_refptr to
 * it. However, the event listeners added to this class ARE garbage collected.
 * This causes an error whenever a bug is fired.
 *
 */
class EventTargetBug : public web::EventTarget {
 public:
  explicit EventTargetBug(script::EnvironmentSettings* settings);

  ~EventTargetBug() {
    // WILL NEVER HAPPEN THANKS TO scoped_refptr on factory class.
    SB_LOG(INFO) << "[EventTargetBug] This should never be destroyed";
  }

  void FireEvent() {
    SB_LOG(INFO) << "[EventTargetBug] Firing event";
    DispatchEvent(new web::Event("event"));
  }

  void AsyncFireEvent(const int milliseconds) {
    SB_LOG(INFO) << "[EventTargetBug] Async firing event";
    target_timer_->Start(
        FROM_HERE, base::TimeDelta::FromMilliseconds(milliseconds),
        base::Bind(&EventTargetBug::FireEvent, base::Unretained(this)));
  }

  static void ForceGarbageCollection(script::EnvironmentSettings* settings) {
    SB_LOG(INFO) << "[EventTargetBug] Forcing Garbage Collection on JS Engine";
    web::get_context(settings)->javascript_engine()->CollectGarbage();
  }

  DEFINE_WRAPPABLE_TYPE(EventTargetBug);

 private:
  std::unique_ptr<base::OneShotTimer> target_timer_;
};

class SomeGlobalObjectThatMaintainsARefToEventTargetBug {
 public:
  static SomeGlobalObjectThatMaintainsARefToEventTargetBug* GetInstance() {
    return base::Singleton<
        SomeGlobalObjectThatMaintainsARefToEventTargetBug>::get();
  }

  void AddEventTargetBug(const scoped_refptr<EventTargetBug>& event_target_bug);

 private:
  friend struct base::DefaultSingletonTraits<
      SomeGlobalObjectThatMaintainsARefToEventTargetBug>;

  // This will keep all declared `EventTargetBug` objects alive.
  std::vector<scoped_refptr<EventTargetBug>> objs_;
};

}  // namespace event_target_bug
}  // namespace cobalt

#endif  // COBALT_EVENT_TARGET_BUG_EVENT_TARGET_BUG_H_

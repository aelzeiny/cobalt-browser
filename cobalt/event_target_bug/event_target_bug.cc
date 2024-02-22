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

#include "cobalt/event_target_bug/event_target_bug.h"

namespace cobalt {
namespace event_target_bug {
void SomeGlobalObjectThatMaintainsARefToEventTargetBug::AddEventTargetBug(
    const scoped_refptr<EventTargetBug>& event_target_bug) {
  objs_.push_back(event_target_bug);
}

EventTargetBug::EventTargetBug(script::EnvironmentSettings* settings)
    : web::EventTarget(settings), target_timer_(new base::OneShotTimer()) {
  SB_LOG(INFO) << "[EventTargetBug] Created";

  // Add to Singleton to keep alive.
  SomeGlobalObjectThatMaintainsARefToEventTargetBug::GetInstance()
      ->AddEventTargetBug(this);
}

}  // namespace event_target_bug
}  // namespace cobalt

// Copyright 2026 The Cobalt Authors. All Rights Reserved.
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

#include "cobalt/browser/idle_memory_purger.h"

#include "base/allocator/partition_allocator/src/partition_alloc/memory_reclaimer.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/discardable_memory_allocator.h"
#include "base/memory/memory_pressure_listener.h"

namespace cobalt {

IdleMemoryPurger::IdleMemoryPurger(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
  // Start the countdown immediately: with no interaction yet the app is
  // input-quiescent from the observer's point of view.
  RestartIdleTimer();
}

IdleMemoryPurger::~IdleMemoryPurger() = default;

void IdleMemoryPurger::DidGetUserInteraction(
    const blink::WebInputEvent& event) {
  // Fired by WebContentsImpl for key down, mouse down, touch start and
  // scroll-begin (this covers TV remote key presses, which arrive as key
  // events). Any interaction pushes the quiescence deadline out.
  RestartIdleTimer();
}

void IdleMemoryPurger::MediaStartedPlaying(const MediaPlayerInfo& media_info,
                                           const content::MediaPlayerId& id) {
  playing_media_.insert(id);
}

void IdleMemoryPurger::MediaStoppedPlaying(
    const MediaPlayerInfo& media_info,
    const content::MediaPlayerId& id,
    WebContentsObserver::MediaStoppedReason reason) {
  playing_media_.erase(id);
}

void IdleMemoryPurger::WebContentsDestroyed() {
  idle_timer_.Stop();
  playing_media_.clear();
}

void IdleMemoryPurger::RestartIdleTimer() {
  idle_timer_.Start(FROM_HERE, kIdleThreshold, this,
                    &IdleMemoryPurger::OnIdleTimerFired);
}

void IdleMemoryPurger::OnIdleTimerFired() {
  // Keep the countdown alive regardless of the outcome below, so a long park
  // is re-checked (and re-swept) every kIdleThreshold.
  RestartIdleTimer();

  if (!playing_media_.empty()) {
    // Media is actively playing (typically fullscreen playback). Purging now
    // could cause visible jank or re-decodes, and playback sessions receive
    // input rarely, so wait for the next check instead.
    return;
  }

  const base::TimeTicks now = base::TimeTicks::Now();
  if (!last_purge_time_.is_null() &&
      (now - last_purge_time_) < kMinTimeBetweenPurges) {
    return;
  }
  last_purge_time_ = now;

  PurgeNow();
}

void IdleMemoryPurger::PurgeNow() {
  ++purge_count_;
  LOG(INFO) << "Idle memory purge #" << purge_count_ << ": no user input for "
            << kIdleThreshold
            << " and no active media playback; notifying moderate memory "
               "pressure, reclaiming PartitionAlloc and releasing free "
               "discardable memory.";

  // Drives every in-process MemoryPressureListener: Blink cache purging and
  // V8 MemoryPressureNotification(kModerate) via RenderThreadImpl (Cobalt
  // runs single-process on Starboard, so the renderer listeners are in this
  // process), cc caches, and the service-side DiscardableSharedMemoryManager
  // (which purges to half its memory limit on MODERATE).
  base::MemoryPressureListener::NotifyMemoryPressure(
      base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_MODERATE);

  // Return freed-but-retained PartitionAlloc memory to the OS. Same idiom as
  // AppEventRunner::OnLowMemory(). Pressure listeners above are notified
  // asynchronously, so memory they free is decommitted by a later periodic
  // reclaim or by the next sweep; that is fine for a recurring sweep.
  ::partition_alloc::MemoryReclaimer::Instance()->ReclaimAll();

  // Release purged-and-free spans held by the discardable memory allocator.
  // In single-process mode the process-wide instance is the renderer's
  // ClientDiscardableSharedMemoryManager, which only releases its freelist on
  // CRITICAL pressure, so do it explicitly here. ReleaseFreeMemory() is
  // lock-protected and callable from any thread.
  if (base::DiscardableMemoryAllocator::HasInstance()) {
    base::DiscardableMemoryAllocator::GetInstance()->ReleaseFreeMemory();
  }
}

}  // namespace cobalt

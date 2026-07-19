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

#ifndef COBALT_BROWSER_IDLE_MEMORY_PURGER_H_
#define COBALT_BROWSER_IDLE_MEMORY_PURGER_H_

#include "base/containers/flat_set.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "content/public/browser/media_player_id.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class WebContents;
}  // namespace content

namespace cobalt {

// Purges process-wide memory caches after the user has been quiescent for a
// while ("parked" the app): no user input for at least |kIdleThreshold| and no
// media player currently playing in the observed WebContents.
//
// Rationale: on Linux/Starboard no OS memory-pressure source ever fires, so
// caches (Blink, V8, cc, discardable) fill up and are never asked to evict,
// and the live heap drifts upward over multi-hour sessions. This class makes
// that release deterministic: whenever the user parks, the process is swept
// back toward its baseline. Sweeps are rate-limited and skipped while media
// is playing so they never cost user-visible jank.
//
// Lives on the browser UI thread, which is where WebContentsObserver methods
// are dispatched and where the sweep runs.
class IdleMemoryPurger : public content::WebContentsObserver {
 public:
  // Time with no qualifying user input (key/mouse/touch/scroll-begin) before
  // the app is considered parked. While parked, the check re-fires at this
  // cadence, so long parks are swept repeatedly.
  static constexpr base::TimeDelta kIdleThreshold = base::Minutes(5);

  // Minimum spacing between two sweeps. With the current kIdleThreshold this
  // holds by construction (the timer cannot fire twice within the threshold);
  // it is kept as an explicit guard on the purge path.
  static constexpr base::TimeDelta kMinTimeBetweenPurges = base::Minutes(5);

  explicit IdleMemoryPurger(content::WebContents* web_contents);

  IdleMemoryPurger(const IdleMemoryPurger&) = delete;
  IdleMemoryPurger& operator=(const IdleMemoryPurger&) = delete;

  ~IdleMemoryPurger() override;

  // content::WebContentsObserver:
  void DidGetUserInteraction(const blink::WebInputEvent& event) override;
  void MediaStartedPlaying(const MediaPlayerInfo& media_info,
                           const content::MediaPlayerId& id) override;
  void MediaStoppedPlaying(
      const MediaPlayerInfo& media_info,
      const content::MediaPlayerId& id,
      WebContentsObserver::MediaStoppedReason reason) override;
  void WebContentsDestroyed() override;

 private:
  void RestartIdleTimer();
  void OnIdleTimerFired();
  void PurgeNow();

  // Ids of media players currently playing in the observed WebContents.
  // MediaStartedPlaying() is always paired with MediaStoppedPlaying(), which
  // is when ids are released (see WebContentsObserver documentation).
  base::flat_set<content::MediaPlayerId> playing_media_;

  base::TimeTicks last_purge_time_;
  int purge_count_ = 0;

  base::OneShotTimer idle_timer_;
};

}  // namespace cobalt

#endif  // COBALT_BROWSER_IDLE_MEMORY_PURGER_H_

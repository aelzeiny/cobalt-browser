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

#ifndef DOM_TEST_RUNNER_H_
#define DOM_TEST_RUNNER_H_

#if defined(ENABLE_TEST_RUNNER)

#include "base/callback.h"
#include "cobalt/script/wrappable.h"

namespace cobalt {
namespace dom {

// TestRunner is a utility class for JavaScript to communicate with
// Cobalt-driver applications (eg. layout tests). The name of TestRunner is
// analogous to the object by the same name inside of WebKit.
class TestRunner : public script::Wrappable {
 public:
  TestRunner();

  // These methods are used in applications that wait for the onload event to
  // which they react by taking a measurement. The function |WaitUntilDone| can
  // be called to indicate that the system should not take a measurement at the
  // onload event, and instead wait for a call to |NotifyDone| to take the
  // measurement. (eg. CSS background-image and animation related tests use
  // |WaitUntilDone| to block the snapshot taken when the onload event
  // triggered, and wait for a |NotifyDone| call to take the snapshot.)
  void NotifyDone();
  void WaitUntilDone();

  void set_notify_done_callback(const base::Closure& notify_done_callback) {
    notify_done_callback_ = notify_done_callback;
  }

  bool should_wait() { return should_wait_; }

  DEFINE_WRAPPABLE_TYPE(TestRunner);

 private:
  ~TestRunner() {}

  bool should_wait_;
  base::Closure notify_done_callback_;

  DISALLOW_COPY_AND_ASSIGN(TestRunner);
};

}  // namespace dom
}  // namespace cobalt

#endif  // ENABLE_TEST_RUNNER

#endif  // DOM_TEST_RUNNER_H_

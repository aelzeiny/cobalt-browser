// Copyright 2025 The Cobalt Authors. All Rights Reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_CHANGES_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_CHANGES_H_

#include <functional>
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace cobalt {
namespace h5vcc {
namespace idom {

// A change buffer to queue up changes to be applied to the DOM.
class Changes {
 public:
  // Queues a change to be applied.
  template <typename... Args>
  void Queue(std::function<void(Args...)> fn, Args... args);

  // Flushes the queue, applying all changes.
  void Flush();

 private:
  WTF::Vector<std::function<void()>> buffer_;
  wtf_size_t buffer_start_ = 0;
};

template <typename... Args>
void Changes::Queue(std::function<void(Args...)> fn, Args... args) {
  buffer_.push_back(std::bind(fn, std::forward<Args>(args)...));
}

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_CHANGES_H_

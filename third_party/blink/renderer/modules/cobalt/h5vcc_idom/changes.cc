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

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/changes.h"

namespace cobalt {
namespace h5vcc {
namespace idom {

void Changes::Flush() {
  // A change may cause this function to be called re-entrantly. Keep track of
  // the portion of the buffer we are consuming. Updates the start pointer so
  // that the next call knows where to start from.
  const wtf_size_t start = buffer_start_;
  const wtf_size_t end = buffer_.size();

  buffer_start_ = end;

  for (wtf_size_t i = start; i < end; ++i) {
    buffer_[i]();
  }

  buffer_start_ = start;
  buffer_.Shrink(start);
}

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

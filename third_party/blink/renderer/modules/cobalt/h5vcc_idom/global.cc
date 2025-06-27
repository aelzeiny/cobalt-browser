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

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/global.h"

namespace cobalt {
namespace h5vcc {
namespace idom {

namespace {
WTF::String* g_key_attribute_name = nullptr;
}  // namespace

const WTF::String& KeyAttributeName() {
  if (!g_key_attribute_name) {
    g_key_attribute_name = new WTF::String("key");
  }
  return *g_key_attribute_name;
}

void SetKeyAttributeName(const WTF::String& name) {
  if (!g_key_attribute_name) {
    g_key_attribute_name = new WTF::String(name);
  } else {
    *g_key_attribute_name = name;
  }
}

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

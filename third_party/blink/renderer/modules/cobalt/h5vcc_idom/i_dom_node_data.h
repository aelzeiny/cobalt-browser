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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_I_DOM_NODE_DATA_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_I_DOM_NODE_DATA_H_

#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace cobalt {
namespace h5vcc {
namespace idom {
class NodeData;
}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

namespace blink {

class IDomNodeData final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  explicit IDomNodeData(cobalt::h5vcc::idom::NodeData* node_data);

  String nameOrCtor() const;
  String key() const;
  String text() const;
  bool staticsApplied() const;
  bool alwaysDiffAttributes() const;

  void Trace(Visitor*) const override;

 private:
  cobalt::h5vcc::idom::NodeData* node_data_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_I_DOM_NODE_DATA_H_

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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_CONTEXT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_CONTEXT_H_

#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/i_dom_notification.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_vector.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"

namespace cobalt {
namespace h5vcc {
namespace idom {

// A context object keeps track of the state of a patch.
class Context final : public blink::GarbageCollected<Context> {
 public:
  explicit Context(blink::Node* node);

  void MarkCreated(blink::Node* node);
  void MarkDeleted(blink::Node* node);
  void NotifyChanges(blink::IDomNotification* notifications);

  void Trace(blink::Visitor* visitor) const;

 private:
  blink::HeapVector<blink::Member<blink::Node>> created_;
  blink::HeapVector<blink::Member<blink::Node>> deleted_;
  blink::Member<blink::Node> node_;
  bool tracking_disabled_;
};

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_CONTEXT_H_

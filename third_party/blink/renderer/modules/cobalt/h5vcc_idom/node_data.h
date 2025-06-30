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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_NODE_DATA_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_NODE_DATA_H_

#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_hash_map.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {
class Element;
}

namespace cobalt {
namespace h5vcc {
namespace idom {

// Keeps track of information needed to perform diffs for a given DOM node.
class NodeData final : public blink::GarbageCollected<NodeData> {
 public:
  NodeData(const WTF::String& name_or_ctor, const WTF::String& key);

  const WTF::String& GetNameOrCtor() const { return name_or_ctor_; }
  const WTF::String& GetKey() const { return key_; }

  const WTF::String& GetText() const { return text_; }
  void SetText(const WTF::String& text) { text_ = text; }

  bool StaticsApplied() const { return statics_applied_; }
  void SetStaticsApplied(bool applied) { statics_applied_ = applied; }

  bool AlwaysDiffAttributes() const { return always_diff_attributes_; }
  void SetAlwaysDiffAttributes(bool always_diff) {
    always_diff_attributes_ = always_diff;
  }

  bool HasEmptyAttrsArr() const;
  WTF::Vector<WTF::String>& GetAttrsArr(wtf_size_t length);

  void Trace(blink::Visitor* visitor) const;

 private:
  // The nodeName or constructor for the Node.
  WTF::String name_or_ctor_;
  // The key used to identify this node.
  WTF::String key_;
  // The previous text value, for Text nodes.
  WTF::String text_;

  // An array of attribute name/value pairs.
  WTF::Vector<WTF::String> attrs_arr_;

  // Whether or not the statics have been applied for the node yet.
  bool statics_applied_ = false;
  bool always_diff_attributes_ = false;
};

// Using a map to associate NodeData with a Node, as we cannot add fields to
// Node directly. This map will be managed by H5vccIdom.
using NodeDataMap =
    blink::HeapHashMap<blink::WeakMember<blink::Node>, blink::Member<NodeData>>;

// Initializes a NodeData object for a Node.
NodeData* InitData(NodeDataMap& data_map,
                   blink::Node* node,
                   const WTF::String& name_or_ctor,
                   const WTF::String& key);

// Retrieves the NodeData object for a Node, creating it if necessary.
NodeData* GetData(NodeDataMap& data_map,
                  blink::Node* node,
                  const WTF::String& fallback_key = WTF::String());

// Gets the key for a Node.
WTF::String GetKey(NodeDataMap& data_map, blink::Node* node);

// Imports node and its subtree, initializing caches.
void ImportNode(NodeDataMap& data_map, blink::Node* node);

// Checks if the NodeData already exists.
bool IsDataInitialized(NodeDataMap& data_map, blink::Node* node);

// Clears all caches from a node and all of its children.
void ClearCache(NodeDataMap& data_map, blink::Node* node);

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_NODE_DATA_H_

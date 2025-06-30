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

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/node_data.h"

#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/global.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"

namespace cobalt {
namespace h5vcc {
namespace idom {

namespace {

void RecordAttributes(blink::Element* element, NodeData* data) {
  if (!element->hasAttributes()) {
    return;
  }
  const auto& attributes = element->Attributes();
  auto& attrs_arr = data->GetAttrsArr(attributes.size() * 2);
  attrs_arr.clear();
  for (const auto& attr : attributes) {
    attrs_arr.push_back(attr.GetName().ToString());
    attrs_arr.push_back(attr.Value());
  }
}

NodeData* ImportSingleNode(NodeDataMap& data_map,
                           blink::Node* node,
                           const WTF::String& fallback_key) {
  // OPTIMIZED: Use single lookup instead of double lookup
  auto it = data_map.find(node);
  if (it != data_map.end()) {
    return it->value;
  }

  WTF::String node_name = node->IsElementNode()
                              ? blink::To<blink::Element>(node)->localName()
                              : node->nodeName();
  WTF::String key;
  if (node->IsElementNode()) {
    blink::Element* element = blink::To<blink::Element>(node);
    const WTF::AtomicString key_attr_name(KeyAttributeName());
    if (!key_attr_name.IsNull() && element->hasAttribute(key_attr_name)) {
      key = element->getAttribute(key_attr_name);
    } else if (!fallback_key.empty()) {
      key = fallback_key;
    }
    // Otherwise, key remains empty (null in JS)
  }

  NodeData* data = InitData(data_map, node, node_name, key);

  if (node->IsElementNode()) {
    RecordAttributes(blink::To<blink::Element>(node), data);
  }

  return data;
}

}  // namespace

NodeData::NodeData(const WTF::String& name_or_ctor, const WTF::String& key)
    : name_or_ctor_(name_or_ctor), key_(key) {}

bool NodeData::HasEmptyAttrsArr() const {
  return attrs_arr_.empty();
}

WTF::Vector<WTF::String>& NodeData::GetAttrsArr(wtf_size_t length) {
  if (attrs_arr_.capacity() < length) {
    attrs_arr_.reserve(length);
  }
  return attrs_arr_;
}

void NodeData::Trace(blink::Visitor* visitor) const {}

NodeData* InitData(NodeDataMap& data_map,
                   blink::Node* node,
                   const WTF::String& name_or_ctor,
                   const WTF::String& key) {
  NodeData* data = blink::MakeGarbageCollected<NodeData>(name_or_ctor, key);
  data_map.Set(node, data);
  return data;
}

NodeData* GetData(NodeDataMap& data_map,
                  blink::Node* node,
                  const WTF::String& fallback_key) {
  // OPTIMIZED: Fast path for already initialized data
  auto it = data_map.find(node);
  if (it != data_map.end()) {
    return it->value;
  }

  // Fallback to slower initialization path only when needed
  return ImportSingleNode(data_map, node, fallback_key);
}

WTF::String GetKey(NodeDataMap& data_map, blink::Node* node) {
  return GetData(data_map, node)->GetKey();
}

void ImportNode(NodeDataMap& data_map, blink::Node* node) {
  ImportSingleNode(data_map, node, WTF::String());
  for (blink::Node* child = node->firstChild(); child;
       child = child->nextSibling()) {
    ImportNode(data_map, child);
  }
}

bool IsDataInitialized(NodeDataMap& data_map, blink::Node* node) {
  // OPTIMIZED: Use faster find() instead of Contains() to avoid double lookup
  return data_map.find(node) != data_map.end();
}

void ClearCache(NodeDataMap& data_map, blink::Node* node) {
  data_map.erase(node);
  for (blink::Node* child = node->firstChild(); child;
       child = child->nextSibling()) {
    ClearCache(data_map, child);
  }
}

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

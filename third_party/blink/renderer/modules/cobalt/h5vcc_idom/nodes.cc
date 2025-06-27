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

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/nodes.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/text.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/node_data.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"

namespace cobalt {
namespace h5vcc {
namespace idom {

namespace {

WTF::AtomicString GetNamespaceForTag(const WTF::AtomicString& tag,
                                     blink::Node* parent) {
  if (tag == "svg") {
    return WTF::AtomicString("http://www.w3.org/2000/svg");
  }
  if (tag == "math") {
    return WTF::AtomicString("http://www.w3.org/1998/Math/MathML");
  }
  if (parent == nullptr) {
    return WTF::AtomicString();
  }
  if (parent->IsElementNode() &&
      blink::To<blink::Element>(parent)->localName() == "foreignObject") {
    return WTF::AtomicString();
  }
  if (parent->IsElementNode()) {
    return blink::To<blink::Element>(parent)->namespaceURI();
  }
  return WTF::AtomicString();
}

}  // namespace

blink::Element* CreateElement(NodeDataMap& data_map,
                              blink::Document* doc,
                              blink::Node* parent,
                              const WTF::AtomicString& name_or_ctor,
                              const WTF::String& key) {
  blink::Element* el;
  // TODO: Handle function case
  // if (typeof nameOrCtor === "function") {
  //   el = new nameOrCtor();
  // } else {
  const WTF::AtomicString& namespace_uri =
      GetNamespaceForTag(name_or_ctor, parent);
  if (!namespace_uri.IsNull()) {
    el = doc->createElementNS(namespace_uri, name_or_ctor, ASSERT_NO_EXCEPTION);
  } else {
    el = doc->CreateElementForBinding(name_or_ctor);
  }
  // }

  InitData(data_map, el, name_or_ctor, key);
  return el;
}

blink::Text* CreateText(NodeDataMap& data_map, blink::Document* doc) {
  blink::Text* node = doc->createTextNode(WTF::String());
  InitData(data_map, node, "#text", WTF::String());
  return node;
}

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

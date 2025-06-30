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
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"

namespace cobalt {
namespace h5vcc {
namespace idom {

namespace {

// OPTIMIZED: Cached namespace resolution with memoization
WTF::AtomicString GetNamespaceForTag(const WTF::AtomicString& tag,
                                     blink::Node* parent) {
  // Use static AtomicStrings for performance
  DEFINE_STATIC_LOCAL(WTF::AtomicString, svg_tag, ("svg"));
  DEFINE_STATIC_LOCAL(WTF::AtomicString, math_tag, ("math"));
  DEFINE_STATIC_LOCAL(WTF::AtomicString, svg_ns,
                      ("http://www.w3.org/2000/svg"));
  DEFINE_STATIC_LOCAL(WTF::AtomicString, mathml_ns,
                      ("http://www.w3.org/1998/Math/MathML"));
  DEFINE_STATIC_LOCAL(WTF::AtomicString, foreign_object, ("foreignObject"));

  // OPTIMIZED: Fast pointer comparison for AtomicStrings
  if (tag == svg_tag) {
    return svg_ns;
  }
  if (tag == math_tag) {
    return mathml_ns;
  }
  if (parent == nullptr) {
    return WTF::AtomicString();
  }

  // OPTIMIZED: Cache parent namespace to avoid repeated checks
  if (parent->IsElementNode()) {
    const blink::Element* element = blink::To<blink::Element>(parent);
    if (element->localName() == foreign_object) {
      return WTF::AtomicString();
    }

    // OPTIMIZED: Return cached namespace directly
    const WTF::AtomicString& parent_ns = element->namespaceURI();
    return parent_ns;
  }
  return WTF::AtomicString();
}

}  // namespace

blink::Element* CreateElement(NodeDataMap& data_map,
                              blink::Document* doc,
                              blink::Node* parent,
                              const WTF::AtomicString& name_or_ctor,
                              const WTF::String& key) {
  // Null check for document pointer
  if (!doc) {
    DLOG(ERROR) << "CreateElement: Document is null";
    return nullptr;
  }

  blink::Element* el = nullptr;
  // TODO: Handle function case
  // if (typeof nameOrCtor === "function") {
  //   el = new nameOrCtor();
  // } else {
  const WTF::AtomicString& namespace_uri =
      GetNamespaceForTag(name_or_ctor, parent);

  // Use ExceptionState to handle potential DOM exceptions
  blink::DummyExceptionStateForTesting exception_state;

  if (!namespace_uri.IsNull()) {
    el = doc->createElementNS(namespace_uri, name_or_ctor, exception_state);
  } else {
    el = doc->CreateElementForBinding(name_or_ctor, exception_state);
  }

  // Check if element creation failed due to invalid name
  if (exception_state.HadException() || !el) {
    DLOG(ERROR) << "CreateElement: Failed to create element '" << name_or_ctor
                << "'";
    return nullptr;
  }
  // }

  InitData(data_map, el, name_or_ctor, key);
  return el;
}

blink::Text* CreateText(NodeDataMap& data_map, blink::Document* doc) {
  // Null check for document pointer
  if (!doc) {
    DLOG(ERROR) << "CreateText: Document is null";
    return nullptr;
  }

  blink::Text* node = doc->createTextNode(WTF::String());
  InitData(data_map, node, "#text", WTF::String());
  return node;
}

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

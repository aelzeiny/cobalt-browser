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

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/native_virtual_elements.h"

#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/core.h"
#include "third_party/blink/renderer/modules/cobalt/h5vcc_idom/node_data.h"

namespace cobalt {
namespace h5vcc {
namespace idom {

ElementBuilder& ElementBuilder::Id(const WTF::String& id) {
  DEFINE_STATIC_LOCAL(WTF::AtomicString, id_attr, ("id"));
  return Attr(id_attr, id);
}

ElementBuilder& ElementBuilder::Class(const WTF::String& class_name) {
  DEFINE_STATIC_LOCAL(WTF::AtomicString, class_attr, ("class"));
  return Attr(class_attr, class_name);
}

ElementBuilder& ElementBuilder::Style(const WTF::String& style) {
  DEFINE_STATIC_LOCAL(WTF::AtomicString, style_attr, ("style"));
  return Attr(style_attr, style);
}

// Convenience methods for common HTML attributes
ElementBuilder& ElementBuilder::Href(const WTF::String& href) {
  DEFINE_STATIC_LOCAL(WTF::AtomicString, href_attr, ("href"));
  return Attr(href_attr, href);
}

ElementBuilder& ElementBuilder::Src(const WTF::String& src) {
  DEFINE_STATIC_LOCAL(WTF::AtomicString, src_attr, ("src"));
  return Attr(src_attr, src);
}

ElementBuilder& ElementBuilder::Type(const WTF::String& type) {
  DEFINE_STATIC_LOCAL(WTF::AtomicString, type_attr, ("type"));
  return Attr(type_attr, type);
}

ElementBuilder& ElementBuilder::Value(const WTF::String& value) {
  DEFINE_STATIC_LOCAL(WTF::AtomicString, value_attr, ("value"));
  return Attr(value_attr, value);
}

ElementBuilder& ElementBuilder::Disabled(bool disabled) {
  DEFINE_STATIC_LOCAL(WTF::AtomicString, disabled_attr, ("disabled"));
  if (disabled) {
    return Attr(disabled_attr, WTF::String(""));
  } else {
    // Remove attribute if not disabled
    // Note: We'd need to track removed attributes for this to work properly
    // For now, just don't add it
    return *this;
  }
}

// Fast common element creators
blink::Element* CreateDiv(NodeDataMap& data_map,
                          const WTF::String& key = WTF::String()) {
  DEFINE_STATIC_LOCAL(WTF::AtomicString, div_tag, ("div"));
  return NativeVirtualElements::ElementOpen(data_map, div_tag, key);
}

blink::Element* CreateSpan(NodeDataMap& data_map,
                           const WTF::String& key = WTF::String()) {
  DEFINE_STATIC_LOCAL(WTF::AtomicString, span_tag, ("span"));
  return NativeVirtualElements::ElementOpen(data_map, span_tag, key);
}

blink::Element* CreateP(NodeDataMap& data_map,
                        const WTF::String& key = WTF::String()) {
  DEFINE_STATIC_LOCAL(WTF::AtomicString, p_tag, ("p"));
  return NativeVirtualElements::ElementOpen(data_map, p_tag, key);
}

blink::Element* CreateA(NodeDataMap& data_map,
                        const WTF::String& href,
                        const WTF::String& key = WTF::String()) {
  DEFINE_STATIC_LOCAL(WTF::AtomicString, a_tag, ("a"));
  DEFINE_STATIC_LOCAL(WTF::AtomicString, href_attr, ("href"));

  AttributeList attrs;
  if (!href.empty()) {
    attrs.emplace_back(href_attr, AttributeValue(href));
  }

  return NativeVirtualElements::ElementOpen(data_map, a_tag, key,
                                            AttributeList(), attrs);
}

blink::Element* CreateImg(NodeDataMap& data_map,
                          const WTF::String& src,
                          const WTF::String& key = WTF::String()) {
  DEFINE_STATIC_LOCAL(WTF::AtomicString, img_tag, ("img"));
  DEFINE_STATIC_LOCAL(WTF::AtomicString, src_attr, ("src"));

  AttributeList attrs;
  if (!src.empty()) {
    attrs.emplace_back(src_attr, AttributeValue(src));
  }

  return NativeVirtualElements::ElementVoid(data_map, img_tag, key,
                                            AttributeList(), attrs);
}

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

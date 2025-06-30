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

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_ATTRIBUTE_VALUE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_ATTRIBUTE_VALUE_H_

#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace cobalt {
namespace h5vcc {
namespace idom {

// Native C++ attribute value storage to eliminate V8 conversions
class AttributeValue {
 public:
  enum Type { kString, kNumber, kBoolean, kNull };

  // Constructors for different types
  AttributeValue() : type_(kNull) {}
  explicit AttributeValue(const WTF::String& value)
      : type_(kString), string_value_(value) {}
  explicit AttributeValue(const WTF::AtomicString& value)
      : type_(kString), string_value_(value.GetString()) {}
  explicit AttributeValue(double value)
      : type_(kNumber), number_value_(value) {}
  explicit AttributeValue(bool value)
      : type_(kBoolean), boolean_value_(value) {}

  // Type accessors
  Type GetType() const { return type_; }
  bool IsString() const { return type_ == kString; }
  bool IsNumber() const { return type_ == kNumber; }
  bool IsBoolean() const { return type_ == kBoolean; }
  bool IsNull() const { return type_ == kNull; }

  // Value accessors
  const WTF::String& AsString() const { return string_value_; }
  double AsNumber() const { return number_value_; }
  bool AsBoolean() const { return boolean_value_; }

  // Convert to string for DOM attribute setting
  WTF::String ToString() const {
    switch (type_) {
      case kString:
        return string_value_;
      case kNumber:
        return WTF::String::Number(number_value_);
      case kBoolean:
        return boolean_value_ ? "true" : "false";
      case kNull:
        return WTF::String();
    }
    return WTF::String();
  }

  // Convert to AtomicString for efficient DOM operations
  WTF::AtomicString ToAtomicString() const {
    return WTF::AtomicString(ToString());
  }

 private:
  Type type_;
  WTF::String string_value_;
  double number_value_ = 0.0;
  bool boolean_value_ = false;
};

// Attribute name-value pair for efficient storage
struct AttributePair {
  WTF::AtomicString name;
  AttributeValue value;

  AttributePair(const WTF::AtomicString& n, const AttributeValue& v)
      : name(n), value(v) {}
};

// Native C++ container for attributes (replaces HeapVector<ScriptValue>)
using AttributeList = WTF::Vector<AttributePair>;

// Builder for attributes without V8 overhead
class AttributeBuilder {
 public:
  void Clear() { attributes_.clear(); }

  void AddAttribute(const WTF::AtomicString& name,
                    const AttributeValue& value) {
    attributes_.emplace_back(name, value);
  }

  void AddAttribute(const WTF::AtomicString& name, const WTF::String& value) {
    attributes_.emplace_back(name, AttributeValue(value));
  }

  void AddAttribute(const WTF::AtomicString& name, double value) {
    attributes_.emplace_back(name, AttributeValue(value));
  }

  void AddAttribute(const WTF::AtomicString& name, bool value) {
    attributes_.emplace_back(name, AttributeValue(value));
  }

  const AttributeList& GetAttributes() const { return attributes_; }
  AttributeList& GetMutableAttributes() { return attributes_; }

  size_t Size() const { return attributes_.size(); }
  bool IsEmpty() const { return attributes_.empty(); }

 private:
  AttributeList attributes_;
};

}  // namespace idom
}  // namespace h5vcc
}  // namespace cobalt

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_COBALT_H5VCC_IDOM_ATTRIBUTE_VALUE_H_

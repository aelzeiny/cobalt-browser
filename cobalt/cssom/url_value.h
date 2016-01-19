/*
 * Copyright 2015 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CSSOM_URL_VALUE_H_
#define CSSOM_URL_VALUE_H_

#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "cobalt/base/polymorphic_equatable.h"
#include "cobalt/cssom/property_value.h"
#include "googleurl/src/gurl.h"

namespace cobalt {
namespace cssom {

class PropertyValueVisitor;

// A URL is a pointer to a resource and corresponds to the URI token in the
// grammar. It is generated by CSS parser when parsing URL values.
//  https://www.w3.org/TR/css3-values/#urls
class URLValue : public PropertyValue {
 public:
  explicit URLValue(const std::string& url);

  void Accept(PropertyValueVisitor* visitor) OVERRIDE;

  const std::string& value() const { return url_; }

  std::string ToString() const OVERRIDE { return "url(" + url_ + ")"; }

  GURL Resolve(const GURL& base_url) const;

  bool is_absolute() const { return is_absolute_; }

  bool operator==(const URLValue& other) const { return url_ == other.url_; }

  DEFINE_POLYMORPHIC_EQUATABLE_TYPE(URLValue);

 private:
  ~URLValue() OVERRIDE {}

  const std::string url_;
  const bool is_absolute_;

  DISALLOW_COPY_AND_ASSIGN(URLValue);
};

}  // namespace cssom
}  // namespace cobalt

#endif  // CSSOM_URL_VALUE_H_

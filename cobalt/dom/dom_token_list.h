/*
 * Copyright 2014 Google Inc. All Rights Reserved.
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

#ifndef COBALT_DOM_DOM_TOKEN_LIST_H_
#define COBALT_DOM_DOM_TOKEN_LIST_H_

#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "cobalt/base/token.h"
#include "cobalt/script/wrappable.h"

namespace cobalt {
namespace dom {

class Element;

// The DOMTokenList interface represents a set of space-separated tokens.
//   https://www.w3.org/TR/2014/WD-dom-20140710/#interface-domtokenlist
class DOMTokenList : public script::Wrappable {
 public:
  DOMTokenList(Element* element, const std::string& attr_name);

  // Web API: DOMTokenList
  //
  unsigned int length() const;

  base::optional<std::string> Item(unsigned int index) const;
  bool Contains(const std::string& token) const;
  void Add(const std::vector<std::string>& tokens);
  void Remove(const std::vector<std::string>& tokens);
  std::string AnonymousStringifier() const;

  // Custom, not in any spec.

  // This is a variation of Item that should only be called in cases where NULL
  // is not needed for invalid indices.
  base::Token NonNullItem(unsigned int index) const;

  // This is a variation of Contains that should only be called in cases where
  // the token has already been validated.
  bool ContainsValid(base::Token valid_token) const;

  DEFINE_WRAPPABLE_TYPE(DOMTokenList);

 private:
  ~DOMTokenList();

  // From the spec: DOMTokenList.
  //   https://www.w3.org/TR/dom/#concept-dtl-update
  void UpdateSteps() const;

  // Returns if a token is valid.
  bool IsTokenValid(const std::string& token) const;
  // Refreshes the cache if it is not up to date.
  void MaybeRefresh() const;

  // The associated element.
  base::WeakPtr<Element> element_;
  // Name of the associated attribute.
  std::string attr_name_;
  // Node generation of the element when tokens is updated.
  mutable uint32_t element_node_generation_;
  // Cached tokens.
  mutable std::vector<base::Token> tokens_;

  DISALLOW_COPY_AND_ASSIGN(DOMTokenList);
};

}  // namespace dom
}  // namespace cobalt

#endif  // COBALT_DOM_DOM_TOKEN_LIST_H_

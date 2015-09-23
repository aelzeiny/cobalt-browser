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

#include "cobalt/cssom/css_property_definitions.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace cobalt {
namespace cssom {

TEST(CSSStylePropertyDefinitionsTest,
     PropertiesHaveALowerCaseCSSIdentifierName) {
  for (int key = 0; key < kMaxEveryPropertyKey + 1; ++key) {
    const char *name = GetPropertyName(static_cast<PropertyKey>(key));
    ASSERT_TRUE(name);
    do {
      ASSERT_TRUE((*name >= 'a' && *name <= 'z') || *name == '-');
    } while (*++name);
  }
}

TEST(CSSStylePropertyDefinitionsTest, LongHandPropertiesHaveAnInitialValue) {
  for (int key = 0; key < kMaxLonghandPropertyKey + 1; ++key) {
    const scoped_refptr<PropertyValue> &initial_value =
        GetPropertyInitialValue(static_cast<PropertyKey>(key));
    ASSERT_TRUE(initial_value);
  }
}

TEST(CSSStylePropertyDefinitionsTest, LongHandPropertiesKeyLookup) {
  for (int key = 0; key < kMaxLonghandPropertyKey + 1; ++key) {
    const char *name = GetPropertyName(static_cast<PropertyKey>(key));
    ASSERT_TRUE(name);
    PropertyKey looked_up_key = GetLonghandPropertyKey(name);
    ASSERT_NE(looked_up_key, kNoneProperty);
    ASSERT_EQ(key, looked_up_key);
  }
}

}  // namespace cssom
}  // namespace cobalt

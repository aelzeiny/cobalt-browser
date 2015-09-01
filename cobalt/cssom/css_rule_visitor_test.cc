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

#include "cobalt/cssom/css_rule_visitor.h"

#include "cobalt/cssom/css_media_rule.h"
#include "cobalt/cssom/css_style_rule.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cobalt {
namespace cssom {

class MockCSSRuleVisitor : public CSSRuleVisitor {
 public:
  MOCK_METHOD1(VisitCSSStyleRule, void(CSSStyleRule* css_style_rule));
  MOCK_METHOD1(VisitCSSMediaRule, void(CSSMediaRule* css_media_rule));
};

TEST(CSSRuleVisitorTest, VisitsCSSStyleRule) {
  scoped_refptr<CSSStyleRule> css_style_rule = new CSSStyleRule();
  MockCSSRuleVisitor mock_visitor;
  EXPECT_CALL(mock_visitor, VisitCSSStyleRule(css_style_rule.get()));
  css_style_rule->Accept(&mock_visitor);
}

TEST(CSSRuleVisitorTest, VisitsCSSMediaRule) {
  scoped_refptr<CSSMediaRule> rule = new CSSMediaRule();
  MockCSSRuleVisitor mock_visitor;
  EXPECT_CALL(mock_visitor, VisitCSSMediaRule(rule.get()));
  rule->Accept(&mock_visitor);
}

}  // namespace cssom
}  // namespace cobalt

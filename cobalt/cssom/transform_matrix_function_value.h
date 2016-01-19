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

#ifndef CSSOM_TRANSFORM_MATRIX_FUNCTION_VALUE_H_
#define CSSOM_TRANSFORM_MATRIX_FUNCTION_VALUE_H_

#include <string>

#include "base/compiler_specific.h"
#include "cobalt/base/polymorphic_equatable.h"
#include "cobalt/cssom/property_value.h"
#include "cobalt/cssom/property_value_visitor.h"
#include "cobalt/cssom/transform_matrix.h"

namespace cobalt {
namespace cssom {

class TransformFunctionVisitor;

// The matrix function allows one to specify a 2D 2x3 affine transformation
// as a matrix.
//   https://www.w3.org/TR/css-transforms-1/#funcdef-matrix
class TransformMatrixFunctionValue : public PropertyValue {
 public:
  explicit TransformMatrixFunctionValue(const TransformMatrix& matrix);

  void Accept(PropertyValueVisitor* visitor) OVERRIDE {
    visitor->VisitTransformMatrixFunction(this);
  }

  const TransformMatrix& value() const { return value_; }

  std::string ToString() const OVERRIDE;

  bool operator==(const TransformMatrixFunctionValue& other) const {
    return value_ == other.value_;
  }

  DEFINE_POLYMORPHIC_EQUATABLE_TYPE(TransformMatrixFunctionValue);

 private:
  virtual ~TransformMatrixFunctionValue() OVERRIDE {}

  const TransformMatrix value_;
};

}  // namespace cssom
}  // namespace cobalt

#endif  // CSSOM_TRANSFORM_MATRIX_FUNCTION_VALUE_H_

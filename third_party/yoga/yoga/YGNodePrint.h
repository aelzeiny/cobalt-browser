/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifdef DEBUG

#pragma once

#ifndef THIRD_PARTY_YOGA_YOGA_YGNODEPRINT_H_
#define THIRD_PARTY_YOGA_YOGA_YGNODEPRINT_H_

#include <string>

#include <yoga/Yoga.h>

namespace facebook {
namespace yoga {

void YGNodeToString(
    std::string& str,
    YGNodeRef node,
    YGPrintOptions options,
    uint32_t level);

} // namespace yoga
} // namespace facebook

#endif

#endif  // THIRD_PARTY_YOGA_YOGA_YGNODEPRINT_H_

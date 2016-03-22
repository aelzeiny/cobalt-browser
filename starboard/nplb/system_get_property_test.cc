// Copyright 2015 Google Inc. All Rights Reserved.
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

#include <string.h>

#include "starboard/memory.h"
#include "starboard/string.h"
#include "starboard/system.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace starboard {
namespace nplb {
namespace {

// Size of appropriate value buffer.
const size_t kValueSize = 1024;

void BasicTest(SbSystemPropertyId id,
               bool expect_result,
               bool expected_result,
               int line) {
#define LOCAL_CONTEXT "Context : id=" << id << ", line=" << line;
  char value[kValueSize] = {0};
  SbMemorySet(value, 0xCD, kValueSize);
  bool result = SbSystemGetProperty(id, value, kValueSize);
  if (expect_result) {
    EXPECT_EQ(expected_result, result) << LOCAL_CONTEXT;
  }
  if (result) {
    EXPECT_NE('\xCD', value[0]) << LOCAL_CONTEXT;
    int len = SbStringGetLength(value);
    EXPECT_GT(len, 0) << LOCAL_CONTEXT;
  } else {
    EXPECT_EQ('\xCD', value[0]) << LOCAL_CONTEXT;
  }
#undef LOCAL_CONTEXT
}

TEST(SbSystemGetPropertyTest, ReturnsRequired) {
  BasicTest(kSbSystemPropertyFriendlyName, true, true, __LINE__);
  BasicTest(kSbSystemPropertyManufacturerName, true, true, __LINE__);
  BasicTest(kSbSystemPropertyModelName, true, true, __LINE__);
  BasicTest(kSbSystemPropertyPlatformName, true, true, __LINE__);
  BasicTest(kSbSystemPropertyPlatformUuid, true, true, __LINE__);
}

TEST(SbSystemGetPropertyTest, FailsGracefullyZeroBufferLength) {
  char value[kValueSize] = {0};
  EXPECT_FALSE(SbSystemGetProperty(kSbSystemPropertyFriendlyName, value, 0));
}

TEST(SbSystemGetPropertyTest, FailsGracefullyNullBuffer) {
  EXPECT_FALSE(
      SbSystemGetProperty(kSbSystemPropertyFriendlyName, NULL, kValueSize));
}

TEST(SbSystemGetPropertyTest, FailsGracefullyNullBufferAndZeroLength) {
  EXPECT_FALSE(SbSystemGetProperty(kSbSystemPropertyFriendlyName, NULL, 0));
}

TEST(SbSystemGetPropertyTest, FailsGracefullyBogusId) {
  BasicTest(static_cast<SbSystemPropertyId>(99999), true, false, __LINE__);
}

}  // namespace
}  // namespace nplb
}  // namespace starboard

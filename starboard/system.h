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

// A broad set of APIs that allow the client application to query build and
// runtime properties of the enclosing system.

#ifndef STARBOARD_SYSTEM_H_
#define STARBOARD_SYSTEM_H_

#include "starboard/configuration.h"
#include "starboard/export.h"
#include "starboard/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Enumeration of special paths that the platform can define.
typedef enum SbSystemPathId {
  // Path to where the local content files that ship with the binary are
  // available.
  kSbSystemPathContentDirectory,

  // Path to the directory that can be used as a local file cache, if
  // avaialable.
  kSbSystemPathCacheDirectory,

  // Path to the directory where logs can be written into.
  // TODO(***REMOVED***): Remove from release builds?
  kSbSystemPathLogOutputDirectory,

  // Path to the directory where screenshots should be written into.
  // TODO(***REMOVED***): Remove from release builds?
  kSbSystemPathScreenshotDirectory,

  // Path to the directory containing the root of the source tree.
  // TODO(***REMOVED***): Remove from release builds.
  kSbSystemPathSourceDirectory,

  // Path to a directory where temporary files can be written.
  // TODO(***REMOVED***): Remove from release builds?
  kSbSystemPathTempDirectory,

  // Path to a directory where test results can be written.
  // TODO(***REMOVED***): Remove from release builds?
  kSbSystemPathTestOutputDirectory,
} SbSystemPathId;

// Gets the platform-defined system path specified by |path_id|, placing it as a
// zero-terminated string into the user-allocated |out_path|, unless it is
// longer than |path_length| - 1. Returns false if |path_id| is invalid for this
// platform, or if |path_length| is too short for the given result, or if
// |out_path| is NULL. On failure, |out_path| will not be
// changed. Implementation must be thread-safe.
SB_EXPORT bool SbSystemGetPath(
    SbSystemPathId path_id,
    char *out_path,
    int path_length);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // STARBOARD_SYSTEM_H_

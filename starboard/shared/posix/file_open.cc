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

// Adapted from base/platform_file_posix.cc

#include "starboard/file.h"

#include <errno.h>
#include <fcntl.h>

#include "starboard/shared/posix/file_internal.h"

SbFile SbFileOpen(
    const char *path,
    int flags,
    bool *out_created,
    SbFileError *out_error) {
  int open_flags = 0;
  if (flags & kSbFileCreateOnly) {
    open_flags = O_CREAT | O_EXCL;
  }

  if (out_created) {
    *out_created = false;
  }

  // TODO(***REMOVED***): Bring back the DCHECKs and COMPILE_ASSERT once we have
  // Starboard Logging.
  if (flags & kSbFileCreateAlways) {
    // DCHECK(!open_flags);
    open_flags = O_CREAT | O_TRUNC;
  }

  if (flags & kSbFileOpenTruncated) {
    // DCHECK(!open_flags);
    // DCHECK(flags & kSbFileWrite);
    open_flags = O_TRUNC;
  }

  if (!open_flags && !(flags & kSbFileOpenOnly) &&
      !(flags & kSbFileOpenAlways)) {
    // NOTREACHED();
    errno = EOPNOTSUPP;
    if (out_error) {
      *out_error = kSbFileErrorFailed;
    }

    return kSbFileInvalid;
  }

  if (flags & kSbFileWrite && flags & kSbFileRead) {
    open_flags |= O_RDWR;
  } else if (flags & kSbFileWrite) {
    open_flags |= O_WRONLY;
  }

  // COMPILE_ASSERT(O_RDONLY == 0, O_RDONLY_must_equal_zero);

  int mode = S_IRUSR | S_IWUSR;
  int descriptor = HANDLE_EINTR(open(path, open_flags, mode));

  if (flags & kSbFileOpenAlways) {
    if (descriptor < 0) {
      open_flags |= O_CREAT;
      descriptor = HANDLE_EINTR(open(path, open_flags, mode));
      if (out_created && descriptor >= 0) {
        *out_created = true;
      }
    }
  }

  if (out_created && (descriptor >= 0) &&
      (flags & (kSbFileCreateAlways | kSbFileCreateOnly))) {
    *out_created = true;
  }

  if (out_error) {
    if (descriptor >= 0) {
      *out_error = kSbFileOk;
    } else {
      switch (errno) {
        case EACCES:
        case EISDIR:
        case EROFS:
        case EPERM:
          *out_error = kSbFileErrorAccessDenied;
          break;
        case ETXTBSY:
          *out_error = kSbFileErrorInUse;
          break;
        case EEXIST:
          *out_error = kSbFileErrorExists;
          break;
        case ENOENT:
          *out_error = kSbFileErrorNotFound;
          break;
        case EMFILE:
          *out_error = kSbFileErrorTooManyOpened;
          break;
        case ENOMEM:
          *out_error = kSbFileErrorNoMemory;
          break;
        case ENOSPC:
          *out_error = kSbFileErrorNoSpace;
          break;
        case ENOTDIR:
          *out_error = kSbFileErrorNotADirectory;
          break;
        default:
          *out_error = kSbFileErrorFailed;
      }
    }
  }

  if (descriptor < 0) {
    return kSbFileInvalid;
  }

  SbFile result = new SbFilePrivate();
  result->descriptor = descriptor;
  return result;
}

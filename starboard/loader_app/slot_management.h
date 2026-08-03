// Copyright 2020 The Cobalt Authors. All Rights Reserved.
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

#ifndef STARBOARD_LOADER_APP_SLOT_MANAGEMENT_H_
#define STARBOARD_LOADER_APP_SLOT_MANAGEMENT_H_

#include <string>
#include <vector>

#include "starboard/elf_loader/elf_loader_constants.h"

namespace loader_app {

// Compares the Evergreen versions v1 and v2. Returns 1 if v1 is newer than v2;
// returns -1 if v1 is older than v2; returns 0 if v1 is the same as v2, or if
// either of them is invalid.
// TODO: b/489518648 - The visiblity of this formerly private helper has been
// increased so that it can be exposed to unit tests. The tests should be
// rewritten to test the behavior via public APIs.
int CompareEvergreenVersion(const std::vector<char>& v1,
                            const std::vector<char>& v2);

// Interface for loading a library.
class LibraryLoader {
 public:
  virtual ~LibraryLoader() {}

  // Load the library with the provided full path to |library_path| and
  // |content_path|. |compression_type| specifies the compression format.
  // If |use_memory_mapped_file| is true the library would be loaded as a memory
  // mapped file. Compression and memory mapping are not compatible.
  virtual bool Load(const std::string& library_path,
                    const std::string& content_path,
                    elf_loader::CompressionType compression_type,
                    bool use_memory_mapped_file) = 0;

  // Resolve a symbol by name.
  virtual void* Resolve(const std::string& symbol) = 0;
};

// Ensures that a valid uncompressed, memory-mappable copy of the LZ4
// compressed library at |compressed_lib_path| exists at |cache_path|,
// reusing a previously written cache when it still matches the compressed
// library and (re)building it atomically otherwise. Returns true if a valid
// cache is present at |cache_path| on return. Used for loading compressed
// libraries as memory mapped files; see LoadSlotManagedLibrary() for the
// update-slot flow and loader_app.cc for the system-image flow.
bool EnsureUncompressedCache(const std::string& compressed_lib_path,
                             const std::string& cache_path);

// Load the library for the app specified by |app_key| and manage the
// current slot selection by rolling forward or back based on the slot status.
// The actual loading from the slot is performed by the |library_loader|.
// An alternative content can be used by specifying non-empty
// |alternative_content_path| with the full path to the content.
// If |use_memory_mapped_file| is true the library would be loaded as a memory
// mapped file. If the selected library is LZ4 compressed it is first
// decompressed, once per installed binary, to an uncompressed cache file in
// the slot's lib directory and the cache is memory mapped instead. If no
// uncompressed library can be obtained the library is loaded with in-memory
// decompression, as if |use_memory_mapped_file| were false.
// Returns a pointer to the |SbEventHandle| symbol in the library.
void* LoadSlotManagedLibrary(const std::string& app_key,
                             const std::string& alternative_content_path,
                             LibraryLoader* library_loader,
                             bool use_memory_mapped_file);

}  // namespace loader_app

#endif  // STARBOARD_LOADER_APP_SLOT_MANAGEMENT_H_

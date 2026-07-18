// Copyright 2021 The Cobalt Authors. All Rights Reserved.
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

#ifndef STARBOARD_ELF_LOADER_LZ4_FILE_IMPL_H_
#define STARBOARD_ELF_LOADER_LZ4_FILE_IMPL_H_

#include <stdint.h>

#include <vector>

#include "starboard/elf_loader/file_impl.h"
#include "third_party/lz4_lib/lz4frame.h"

namespace elf_loader {

// This class provides opening and reading a file compressed using LZ4 by
// streaming decompression: compressed blocks are decompressed on demand as
// ReadFromOffset() requests advance through the file, so the whole
// decompressed image is never materialized in memory. The file must be
// encoded using the LZ4 Frame Format and consist of a single frame.
class LZ4FileImpl : public FileImpl {
 public:
  LZ4FileImpl();
  ~LZ4FileImpl();

  // Opens the file specified and parses the LZ4 frame header. Decompression
  // itself is deferred until data is requested via ReadFromOffset().
  bool Open(const char* name) override;

  // Reads |size| decompressed bytes starting at decompressed offset |offset|.
  //
  // The LZ4 frame only supports sequential decompression, so reads at or
  // ahead of the current stream position decompress forward (discarding any
  // skipped bytes). Reads behind the current stream position are served from
  // a small carry buffer holding the most recently decompressed bytes; if
  // they reach further back than the carry buffer, decompression restarts
  // from the beginning of the frame. The ELF loader reads the ELF header,
  // then the program header table, then the segments in ascending file
  // offset order, with backward overlaps only from page-rounding of segment
  // offsets, so in practice the frame is decompressed exactly once.
  bool ReadFromOffset(int64_t offset, char* buffer, int size) override;

 private:
  // Carry buffer capacity. The loader's backward seeks come from
  // page-rounding of segment file offsets, so they are smaller than the
  // largest supported page size.
  static constexpr size_t kCarrySize = 64 * 1024;

  // Size of the scratch buffer used to decompress skipped-over bytes.
  static constexpr size_t kDiscardBufferSize = 64 * 1024;

  // Sentinel for |stream_position_| marking the stream state invalid.
  static constexpr uint64_t kInvalidStreamPosition = ~static_cast<uint64_t>(0);

  // Returns the size of the LZ4 frame header in bytes.
  size_t PeekHeaderSize();

  // Reads the LZ4 frame header information from the file into |frame_info|.
  // Once the header has been read, decompression of the data blocks must begin
  // |header_size| bytes into the file (i.e., just after the frame header
  // section). This function returns a hint for the number of source bytes that
  // LZ4F_decompress() expects for its first invocation.
  size_t ConsumeHeader(LZ4F_frameInfo_t* frame_info, size_t header_size);

  // Serves |size| decompressed bytes at |offset| into |buffer|, using the
  // carry buffer, restarting the frame, skipping forward, and decompressing
  // forward as needed.
  bool ReadStreaming(uint64_t offset, char* buffer, size_t size);

  // Decompresses the next |size| bytes of the stream, writing them to |dst|,
  // or discarding them if |dst| is null. Advances |stream_position_| and
  // maintains the carry buffer. Bytes past the end of the frame but within
  // |content_size_| read as zero, matching the old whole-image-vector
  // behavior of leaving unwritten bytes value-initialized.
  bool DecompressForward(char* dst, size_t size);

  // Resets the decompression context and the stream state so that the next
  // DecompressForward() call continues from the first byte of the frame.
  bool RestartStream();

  // Marks the stream state invalid after an unrecoverable decompression or
  // I/O error; the next ReadFromOffset() will attempt a full restart.
  void InvalidateStream();

  // Appends |size| newly decompressed bytes to the carry buffer, keeping only
  // the most recent kCarrySize bytes of the stream.
  void UpdateCarry(const char* data, size_t size);

  // A buffer holding the current chunk of compressed data read from the file,
  // of size |block_size_|.
  std::vector<char> compressed_data_;

  // Number of valid bytes in, and current read offset into,
  // |compressed_data_|.
  size_t compressed_buffer_size_ = 0;
  size_t compressed_buffer_offset_ = 0;

  // The size of the compressed file in bytes.
  size_t file_size_ = 0;

  // The size of the LZ4 frame header in bytes.
  size_t header_size_ = 0;

  // The maximum (un)compressed block size of the frame, from the frame header.
  size_t block_size_ = 0;

  // The offset in the compressed file of the next byte to read.
  size_t compressed_file_offset_ = 0;

  // The decompressed content size declared in the frame header; 0 until the
  // file has been opened successfully.
  uint64_t content_size_ = 0;

  // The number of decompressed bytes produced so far, i.e. the stream offset
  // of the next byte DecompressForward() will produce. Set to
  // kInvalidStreamPosition after an error to force a restart.
  uint64_t stream_position_ = 0;

  // The source size hint returned by the last LZ4F_getFrameInfo() or
  // LZ4F_decompress() call; 0 means the frame has been fully decoded.
  size_t source_bytes_hint_ = 0;

  // The last min(carry_size_, kCarrySize) decompressed bytes, i.e. stream
  // bytes [stream_position_ - carry_size_, stream_position_), stored at the
  // front of |carry_|. This serves the loader's small backward seeks, which
  // come from page-rounding of segment file offsets and are smaller than the
  // maximum page size.
  std::vector<char> carry_;
  size_t carry_size_ = 0;

  // Scratch buffer for decompressing skipped-over bytes, allocated lazily.
  std::vector<char> discard_buffer_;

  // Total time spent decompressing across all reads, reported on destruction.
  int64_t decompression_time_us_ = 0;

  // The LZ4 decompression context.
  LZ4F_dctx* lz4f_context_;
};

}  // namespace elf_loader

#endif  // STARBOARD_ELF_LOADER_LZ4_FILE_IMPL_H_

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

#include "starboard/elf_loader/lz4_file_impl.h"

#include <string.h>
#include <sys/stat.h>

#include <algorithm>
#include <vector>

#include "starboard/common/log.h"
#include "starboard/common/time.h"
#include "starboard/extension/loader_app_metrics.h"
#include "starboard/system.h"

namespace elf_loader {
using ::starboard::CurrentMonotonicTime;

LZ4FileImpl::LZ4FileImpl() {
  const LZ4F_errorCode_t lz4f_error_code =
      LZ4F_createDecompressionContext(&lz4f_context_, LZ4F_VERSION);

  if (lz4f_error_code != 0) {
    SB_LOG(ERROR) << LZ4F_getErrorName(lz4f_error_code);
    lz4f_context_ = nullptr;
  }
}

LZ4FileImpl::~LZ4FileImpl() {
  if (content_size_ > 0) {
    // Decompression is spread across the ReadFromOffset() calls made during
    // loading, so the total duration is only known once loading is done.
    SB_LOG(INFO) << "Decompression took: " << decompression_time_us_ / 1000
                 << " ms";
    auto metrics_extension =
        static_cast<const StarboardExtensionLoaderAppMetricsApi*>(
            SbSystemGetExtension(kStarboardExtensionLoaderAppMetricsName));
    if (metrics_extension &&
        strcmp(metrics_extension->name,
               kStarboardExtensionLoaderAppMetricsName) == 0 &&
        metrics_extension->version >= 2) {
      metrics_extension->SetElfDecompressionDurationMicroseconds(
          decompression_time_us_);
    }
  }

  if (!lz4f_context_) {
    return;
  }

  const LZ4F_errorCode_t lz4f_error_code =
      LZ4F_freeDecompressionContext(lz4f_context_);

  if (lz4f_error_code != 0) {
    SB_LOG(ERROR) << LZ4F_getErrorName(lz4f_error_code);
  }
}

static size_t GetBlockSize(const LZ4F_frameInfo_t* frame_info) {
  switch (frame_info->blockSizeID) {
    case LZ4F_default:
    case LZ4F_max64KB:
      return 64 * (1 << 10);
    case LZ4F_max256KB:
      return 256 * (1 << 10);
    case LZ4F_max1MB:
      return 1 * (1 << 20);
    case LZ4F_max4MB:
      return 4 * (1 << 20);
    default:
      SB_LOG(INFO) << "Got an unknown block size; continuing with 256KB";
      return 256 * (1 << 10);
  }
}

bool LZ4FileImpl::Open(const char* name) {
  SB_DCHECK(name);

  if (!lz4f_context_) {
    return false;
  }

  struct stat file_info;

  if (!FileImpl::Open(name) || fstat(file_, &file_info)) {
    return false;
  }

  size_t header_size = PeekHeaderSize();
  if (LZ4F_isError(header_size)) {
    SB_LOG(ERROR) << LZ4F_getErrorName(header_size);
    return false;
  }

  LZ4F_frameInfo_t frame_info = LZ4F_INIT_FRAMEINFO;
  size_t source_bytes_hint = ConsumeHeader(&frame_info, header_size);
  if (LZ4F_isError(source_bytes_hint)) {
    SB_LOG(ERROR) << LZ4F_getErrorName(source_bytes_hint);
    LZ4F_resetDecompressionContext(lz4f_context_);
    return false;
  }

  // We require the uncompressed data size to be set in the LZ4 frame header
  // so that ReadFromOffset() can validate requested ranges up front.
  uint64_t content_size = frame_info.contentSize;
  if (content_size <= 0) {
    SB_LOG(ERROR) << "Content size must be present in the LZ4 frame header";
    return false;
  }

  // LZ4F_decompress() expects (but does not require) to decode a specific
  // number of source bytes: the size of the current compressed block + the
  // header of the next block. We can meet this expectation often, without
  // allocating much extra space, by using a buffer of size equal to the
  // uncompressed block size.
  block_size_ = GetBlockSize(&frame_info);
  compressed_data_.resize(block_size_);
  carry_.resize(kCarrySize);

  file_size_ = file_info.st_size;
  header_size_ = header_size;
  content_size_ = content_size;
  source_bytes_hint_ = source_bytes_hint;
  compressed_file_offset_ = header_size;
  compressed_buffer_size_ = 0;
  compressed_buffer_offset_ = 0;
  stream_position_ = 0;
  carry_size_ = 0;

  return true;
}

size_t LZ4FileImpl::PeekHeaderSize() {
  std::vector<char> source_buffer(LZ4F_MIN_SIZE_TO_KNOW_HEADER_LENGTH);
  FileImpl::ReadFromOffset(0, source_buffer.data(),
                           LZ4F_MIN_SIZE_TO_KNOW_HEADER_LENGTH);
  return LZ4F_headerSize(source_buffer.data(),
                         LZ4F_MIN_SIZE_TO_KNOW_HEADER_LENGTH);
}

size_t LZ4FileImpl::ConsumeHeader(LZ4F_frameInfo_t* frame_info,
                                  size_t header_size) {
  std::vector<char> source_buffer(header_size);
  FileImpl::ReadFromOffset(0, source_buffer.data(), header_size);
  return LZ4F_getFrameInfo(lz4f_context_, frame_info, source_buffer.data(),
                           &header_size);
}

bool LZ4FileImpl::ReadStreaming(uint64_t offset, char* buffer, size_t size) {
  if (offset < stream_position_) {
    const uint64_t carry_start = stream_position_ - carry_size_;
    if (offset >= carry_start) {
      // Serve the overlap with already-produced bytes from the carry buffer.
      size_t index = static_cast<size_t>(offset - carry_start);
      size_t from_carry = std::min(size, carry_size_ - index);
      memcpy(buffer, carry_.data() + index, from_carry);
      buffer += from_carry;
      offset += from_carry;
      size -= from_carry;
      if (size == 0) {
        return true;
      }
      // The request now continues exactly at the current stream position.
    } else {
      // The request reaches further back than the carry buffer holds; the
      // frame only supports sequential decompression, so start over.
      if (!RestartStream()) {
        return false;
      }
    }
  }
  if (offset > stream_position_) {
    // Decompress and discard up to the requested offset.
    if (!DecompressForward(nullptr,
                           static_cast<size_t>(offset - stream_position_))) {
      return false;
    }
  }
  return DecompressForward(buffer, size);
}

bool LZ4FileImpl::DecompressForward(char* dst, size_t size) {
  while (size > 0) {
    if (source_bytes_hint_ == 0) {
      // The frame ended before the content size declared in its header. The
      // previous implementation decompressed into a value-initialized vector
      // sized to the declared content size, so such bytes read as zero;
      // preserve that behavior.
      while (size > 0) {
        char* out = dst;
        size_t out_size = size;
        if (!out) {
          if (discard_buffer_.empty()) {
            discard_buffer_.resize(kDiscardBufferSize);
          }
          out = discard_buffer_.data();
          out_size = std::min(size, discard_buffer_.size());
        }
        memset(out, 0, out_size);
        UpdateCarry(out, out_size);
        stream_position_ += out_size;
        if (dst) {
          dst += out_size;
        }
        size -= out_size;
      }
      return true;
    }

    if (compressed_buffer_offset_ == compressed_buffer_size_) {
      // Refill the compressed data buffer from the file.
      size_t file_remaining = compressed_file_offset_ < file_size_
                                  ? file_size_ - compressed_file_offset_
                                  : 0;
      size_t to_read =
          std::min(std::min(source_bytes_hint_, block_size_), file_remaining);
      if (to_read == 0) {
        SB_LOG(ERROR) << "LZ4 frame is truncated";
        InvalidateStream();
        return false;
      }
      if (!FileImpl::ReadFromOffset(compressed_file_offset_,
                                    compressed_data_.data(), to_read)) {
        InvalidateStream();
        return false;
      }
      compressed_file_offset_ += to_read;
      compressed_buffer_offset_ = 0;
      compressed_buffer_size_ = to_read;
    }

    char* out = dst;
    size_t out_capacity = size;
    if (!out) {
      if (discard_buffer_.empty()) {
        discard_buffer_.resize(kDiscardBufferSize);
      }
      out = discard_buffer_.data();
      out_capacity = std::min(size, discard_buffer_.size());
    }

    size_t produced = out_capacity;
    size_t consumed = compressed_buffer_size_ - compressed_buffer_offset_;
    size_t hint = LZ4F_decompress(
        lz4f_context_, out, &produced,
        compressed_data_.data() + compressed_buffer_offset_, &consumed,
        nullptr);
    if (LZ4F_isError(hint)) {
      SB_LOG(ERROR) << LZ4F_getErrorName(hint);
      InvalidateStream();
      return false;
    }
    source_bytes_hint_ = hint;
    compressed_buffer_offset_ += consumed;
    if (consumed == 0 && produced == 0 && hint != 0) {
      // Defensive: LZ4F_decompress() should always make progress when given
      // both input and output space; avoid an infinite loop if it does not.
      SB_LOG(ERROR) << "LZ4 decompression made no progress";
      InvalidateStream();
      return false;
    }
    if (produced > 0) {
      UpdateCarry(out, produced);
      stream_position_ += produced;
      if (dst) {
        dst += produced;
      }
      size -= produced;
    }
  }
  return true;
}

bool LZ4FileImpl::RestartStream() {
  LZ4F_resetDecompressionContext(lz4f_context_);

  LZ4F_frameInfo_t frame_info = LZ4F_INIT_FRAMEINFO;
  size_t source_bytes_hint = ConsumeHeader(&frame_info, header_size_);
  if (LZ4F_isError(source_bytes_hint)) {
    SB_LOG(ERROR) << LZ4F_getErrorName(source_bytes_hint);
    InvalidateStream();
    return false;
  }

  source_bytes_hint_ = source_bytes_hint;
  compressed_file_offset_ = header_size_;
  compressed_buffer_size_ = 0;
  compressed_buffer_offset_ = 0;
  stream_position_ = 0;
  carry_size_ = 0;
  return true;
}

void LZ4FileImpl::InvalidateStream() {
  LZ4F_resetDecompressionContext(lz4f_context_);
  source_bytes_hint_ = 0;
  compressed_buffer_size_ = 0;
  compressed_buffer_offset_ = 0;
  stream_position_ = kInvalidStreamPosition;
  carry_size_ = 0;
}

void LZ4FileImpl::UpdateCarry(const char* data, size_t size) {
  const size_t capacity = carry_.size();
  if (capacity == 0 || size == 0) {
    return;
  }
  if (size >= capacity) {
    memcpy(carry_.data(), data + (size - capacity), capacity);
    carry_size_ = capacity;
    return;
  }
  if (carry_size_ + size > capacity) {
    // Drop the oldest bytes to make room.
    size_t keep = capacity - size;
    memmove(carry_.data(), carry_.data() + (carry_size_ - keep), keep);
    carry_size_ = keep;
  }
  memcpy(carry_.data() + carry_size_, data, size);
  carry_size_ += size;
}

bool LZ4FileImpl::ReadFromOffset(int64_t offset, char* buffer, int size) {
  SB_DCHECK(lz4f_context_);
  if (file_ < 0) {
    return false;
  }
  if (offset < 0 || size < 0) {
    return false;
  }
  if (offset > static_cast<int64_t>(content_size_) ||
      size > static_cast<int64_t>(content_size_) - offset) {
    return false;
  }
  if (size == 0) {
    return true;
  }

  int64_t start_time_us = CurrentMonotonicTime();
  bool result = ReadStreaming(static_cast<uint64_t>(offset), buffer,
                              static_cast<size_t>(size));
  decompression_time_us_ += CurrentMonotonicTime() - start_time_us;
  return result;
}

}  // namespace elf_loader

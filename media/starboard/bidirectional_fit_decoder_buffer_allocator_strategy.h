// Copyright 2025 The Cobalt Authors. All Rights Reserved.
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

#ifndef MEDIA_STARBOARD_BIDIRECTIONAL_FIT_DECODER_BUFFER_ALLOCATOR_STRATEGY_H_
#define MEDIA_STARBOARD_BIDIRECTIONAL_FIT_DECODER_BUFFER_ALLOCATOR_STRATEGY_H_

#include <cstddef>

#include "media/starboard/decoder_buffer_allocator.h"
#include "media/starboard/starboard_memory_allocator.h"
#include "starboard/common/bidirectional_fit_reuse_allocator.h"
#include "starboard/configuration.h"

namespace media {

template <typename ReuseAllocatorBase>
class BidirectionalFitDecoderBufferAllocatorStrategy
    : public DecoderBufferAllocator::Strategy {
 public:
  static constexpr size_t kAudioAllocationIncrement = 256 * 1024;
  static constexpr size_t kMaxVideoAllocationIncrement = 1024 * 1024;

  BidirectionalFitDecoderBufferAllocatorStrategy(size_t initial_capacity,
                                                 size_t allocation_increment)
      : fallback_allocator_(/*enable_decommit=*/true,
                            /*enable_page_alignment=*/true),
        audio_allocator_(
            &fallback_allocator_,
            /*initial_capacity=*/0,
            kSmallAllocationThreshold,
            std::min(allocation_increment > 0 ? allocation_increment
                                              : kAudioAllocationIncrement,
                     kAudioAllocationIncrement),
            /*enable_decommit_on_idle=*/true,
            /*retain_blocks=*/1,
            /*conservative_decommit_blocks=*/0,
            /*aggressive_decommit_on_suspend=*/true,
            /*memset_on_reclaim=*/false,
            /*mark_as_cold_on_reclaim=*/false),
        video_allocator_(
            &fallback_allocator_,
            initial_capacity,
            kSmallAllocationThreshold,
            std::min(allocation_increment > 0 ? allocation_increment
                                              : kMaxVideoAllocationIncrement,
                     kMaxVideoAllocationIncrement),
            /*enable_decommit_on_idle=*/true,
            /*retain_blocks=*/2,
            /*conservative_decommit_blocks=*/0,
            /*aggressive_decommit_on_suspend=*/true,
            /*memset_on_reclaim=*/false,
            /*mark_as_cold_on_reclaim=*/false) {}

  // Constructs a strategy with explicit decommit configurations.
  explicit BidirectionalFitDecoderBufferAllocatorStrategy(
      const DecoderBufferAllocator::Strategy::ExperimentConfig& config)
      : fallback_allocator_(config.enable_decommit_on_idle,
                            config.allocate_with_page_alignment),
        audio_allocator_(&fallback_allocator_,
                         /*initial_capacity=*/0,
                         kSmallAllocationThreshold,
                         std::min(config.allocation_increment > 0
                                      ? config.allocation_increment
                                      : kAudioAllocationIncrement,
                                  kAudioAllocationIncrement),
                         config.enable_decommit_on_idle,
                         std::min<size_t>(config.retain_blocks, 1),
                         config.conservative_decommit_blocks,
                         config.aggressive_decommit_on_suspend,
                         config.memset_on_reclaim,
                         config.mark_as_cold_on_reclaim),
        video_allocator_(&fallback_allocator_,
                         config.initial_capacity,
                         kSmallAllocationThreshold,
                         config.allocation_increment,
                         config.enable_decommit_on_idle,
                         config.retain_blocks,
                         config.conservative_decommit_blocks,
                         config.aggressive_decommit_on_suspend,
                         config.memset_on_reclaim,
                         config.mark_as_cold_on_reclaim) {}

  void* Allocate(DemuxerStream::Type type, size_t size) override {
    if (type == DemuxerStream::AUDIO) {
      return audio_allocator_.Allocate(size);
    }
    return video_allocator_.Allocate(size);
  }
  void Free(DemuxerStream::Type type, void* p) override {
    if (type == DemuxerStream::AUDIO) {
      audio_allocator_.Free(p);
      return;
    }
    video_allocator_.Free(p);
  }

  size_t GetCapacity() const override {
    return audio_allocator_.GetCapacity() + video_allocator_.GetCapacity();
  }

  size_t GetAllocated() const override {
    return audio_allocator_.GetAllocated() + video_allocator_.GetAllocated();
  }

  void DecommitAllDecommitableBlocks() override {
    audio_allocator_.DecommitAllDecommitableBlocks();
    video_allocator_.DecommitAllDecommitableBlocks();
  }

  void TryToDecommitOneBlock(int cadence) override {
    audio_allocator_.TryToDecommitOneBlock(cadence);
    video_allocator_.TryToDecommitOneBlock(cadence);
  }

 private:
  // Used to determine if the memory allocated is large. The underlying logic
  // can be different.
  static constexpr size_t kSmallAllocationThreshold = 512;

  StarboardMemoryAllocator fallback_allocator_;
  starboard::BidirectionalFitReuseAllocator<ReuseAllocatorBase>
      audio_allocator_;
  starboard::BidirectionalFitReuseAllocator<ReuseAllocatorBase>
      video_allocator_;
};

}  // namespace media

#endif  // MEDIA_STARBOARD_BIDIRECTIONAL_FIT_DECODER_BUFFER_ALLOCATOR_STRATEGY_H_

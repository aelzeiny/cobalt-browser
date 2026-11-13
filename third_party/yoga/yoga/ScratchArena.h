/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <vector>

namespace facebook::yoga {

// rive: temporaries of a layout pass come from blocks kept between passes, so
// a steady state pass allocates nothing.
class ScratchArena {
  struct Block {
    Block* next;
    size_t capacity;
    alignas(std::max_align_t) unsigned char data[1];
  };

public:
  // Larger temporaries are few a pass and come from the heap, which reuses
  // what they free; here they would hold their space until the pass ends.
  static constexpr size_t kLargeAllocation = 16 * 1024;
  // Kept between passes, so one huge grid doesn't hold its high water mark
  // for the life of the thread.
  static constexpr size_t kKeptCapacity = 64 * 1024;

  struct Mark {
    Block* block;
    size_t used;
  };

  static ScratchArena& current() {
    static thread_local ScratchArena arena;
    return arena;
  }

  ~ScratchArena() {
    while (first_ != nullptr) {
      Block* next = first_->next;
      ::operator delete(first_);
      first_ = next;
    }
  }

  Mark mark() const { return {block_, used_}; }
  void release(Mark m) {
    block_ = m.block;
    used_ = m.used;
    if (block_ == nullptr) {
      trim();
    }
  }

  void* allocate(size_t bytes, size_t align) {
    size_t at = (used_ + align - 1) & ~(align - 1);
    if (block_ == nullptr || at + bytes > block_->capacity) {
      advance(bytes);
      at = 0;
    }
    used_ = at + bytes;
    return block_->data + at;
  }

  // Only the newest allocation goes back before its scope ends.
  void deallocate(void* p, size_t bytes) {
    auto* at = static_cast<unsigned char*>(p);
    if (block_ != nullptr && at + bytes == block_->data + used_) {
      used_ = static_cast<size_t>(at - block_->data);
    }
  }

private:
  void trim() {
    size_t kept = 0;
    Block** link = &first_;
    while (*link != nullptr && kept + (*link)->capacity <= kKeptCapacity) {
      kept += (*link)->capacity;
      link = &(*link)->next;
    }
    Block* extra = *link;
    *link = nullptr;
    while (extra != nullptr) {
      Block* next = extra->next;
      ::operator delete(extra);
      extra = next;
    }
  }

  void advance(size_t bytes) {
    Block* next = block_ != nullptr ? block_->next : first_;
    if (next == nullptr || next->capacity < bytes) {
      size_t capacity = block_ != nullptr ? block_->capacity * 2 : 16 * 1024;
      while (capacity < bytes) {
        capacity *= 2;
      }
      auto* fresh = static_cast<Block*>(
          ::operator new(offsetof(Block, data) + capacity));
      fresh->capacity = capacity;
      fresh->next = next;
      if (block_ != nullptr) {
        block_->next = fresh;
      } else {
        first_ = fresh;
      }
      next = fresh;
    }
    block_ = next;
    used_ = 0;
  }

  Block* first_ = nullptr;
  Block* block_ = nullptr;
  size_t used_ = 0;
};

class ScratchScope {
public:
  ScratchScope() : mark_(ScratchArena::current().mark()) {}
  ~ScratchScope() { ScratchArena::current().release(mark_); }
  ScratchScope(const ScratchScope&) = delete;
  ScratchScope& operator=(const ScratchScope&) = delete;

private:
  ScratchArena::Mark mark_;
};

template <typename T>
struct ScratchAllocator {
  using value_type = T;
  ScratchAllocator() = default;
  template <typename U>
  ScratchAllocator(const ScratchAllocator<U>&) {}
  T* allocate(size_t n) {
    const size_t bytes = n * sizeof(T);
    if (bytes > ScratchArena::kLargeAllocation) {
      return static_cast<T*>(::operator new(bytes));
    }
    return static_cast<T*>(
        ScratchArena::current().allocate(bytes, alignof(T)));
  }
  void deallocate(T* p, size_t n) {
    const size_t bytes = n * sizeof(T);
    if (bytes > ScratchArena::kLargeAllocation) {
      ::operator delete(p);
      return;
    }
    ScratchArena::current().deallocate(p, bytes);
  }
  template <typename U>
  bool operator==(const ScratchAllocator<U>&) const {
    return true;
  }
  template <typename U>
  bool operator!=(const ScratchAllocator<U>&) const {
    return false;
  }
};

template <typename T>
using ScratchVector = std::vector<T, ScratchAllocator<T>>;

} // namespace facebook::yoga

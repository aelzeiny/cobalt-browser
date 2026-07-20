// Copyright 2019 The Cobalt Authors. All Rights Reserved.
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

#include "starboard/elf_loader/relocations.h"

#include <memory>

#include "starboard/elf_loader/elf.h"
#include "starboard/elf_loader/file_impl.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace elf_loader {

namespace {

// Test constants used as sample data.
const Addr kTestAddress = 34;
const Sword kTestAddend = 5;

class RelocationsTest : public ::testing::Test {
 protected:
  RelocationsTest() {
    memset(buf_, 'A', sizeof(buf_));
    base_addr_ = reinterpret_cast<Addr>(&buf_);
    dynamic_table_[0].d_tag = DT_REL;

    dynamic_section_.reset(
        new DynamicSection(base_addr_, dynamic_table_, 1, 0));
    dynamic_section_->InitDynamicSection();

    exported_symbols_.reset(new ExportedSymbols());
    relocations_.reset(new Relocations(base_addr_, dynamic_section_.get(),
                                       exported_symbols_.get()));
  }
  ~RelocationsTest() {}

  void VerifySymAddress(const rel_t* rel, Addr sym_addr) {
    Addr target_addr = base_addr_ + rel->r_offset;
    Addr target_value = sym_addr;
    EXPECT_EQ(target_value, *reinterpret_cast<Addr*>(target_addr));
  }

#ifdef USE_RELA
  void VerifySymAddressPlusAddend(const rel_t* rel, Addr sym_addr) {
    Addr target_addr = base_addr_ + rel->r_offset;
    Addr target_value = sym_addr + rel->r_addend;
    EXPECT_EQ(target_value, *reinterpret_cast<Addr*>(target_addr));
  }

  void VerifyBaseAddressPlusAddend(const rel_t* rel) {
    Addr target_addr = base_addr_ + rel->r_offset;
    Addr target_value = base_addr_ + rel->r_addend;
    EXPECT_EQ(target_value, *reinterpret_cast<Addr*>(target_addr));
  }

  void VerifySymAddressPlusAddendDelta(const rel_t* rel, Addr sym_addr) {
    Addr target_addr = base_addr_ + rel->r_offset;
    Addr offset_rel = rel->r_offset + base_addr_;
    Addr target_value = sym_addr + (rel->r_addend - offset_rel);
    EXPECT_EQ(target_value, *reinterpret_cast<Addr*>(target_addr));
  }
#endif

 protected:
  std::unique_ptr<Relocations> relocations_;
  Addr base_addr_;

 private:
  char buf_[128];
  Dyn dynamic_table_[10];
  std::unique_ptr<DynamicSection> dynamic_section_;
  std::unique_ptr<ExportedSymbols> exported_symbols_;
};

#if SB_IS(ARCH_ARM)
TEST_F(RelocationsTest, R_ARM_JUMP_SLOT) {
  rel_t rel;
  rel.r_offset = 0;
  rel.r_info = R_ARM_JUMP_SLOT;
  Addr sym_addr = kTestAddress;

  // Expected relocation calculation:
  //   *target = sym_addr;
  relocations_->ApplyResolvedReloc(&rel, sym_addr);

  VerifySymAddress(&rel, sym_addr);
}

TEST_F(RelocationsTest, R_ARM_GLOB_DAT) {
  rel_t rel;
  rel.r_offset = 1;
  rel.r_info = R_ARM_GLOB_DAT;
  Addr sym_addr = kTestAddress;

  // Expected relocation calculation:
  //   *target = sym_addr;
  relocations_->ApplyResolvedReloc(&rel, sym_addr);

  VerifySymAddress(&rel, sym_addr);
}

TEST_F(RelocationsTest, R_ARM_ABS32) {
  rel_t rel;
  rel.r_offset = 2;
  rel.r_info = R_ARM_ABS32;
  Addr sym_addr = kTestAddress;

  Addr target_addr = base_addr_ + rel.r_offset;
  Addr target_value = *reinterpret_cast<Addr*>(target_addr);
  target_value += sym_addr;

  // Expected relocation calculation:
  //   *target += sym_addr;
  relocations_->ApplyResolvedReloc(&rel, sym_addr);

  EXPECT_EQ(target_value, *reinterpret_cast<Addr*>(target_addr));
}

TEST_F(RelocationsTest, R_ARM_REL32) {
  rel_t rel;
  rel.r_offset = 3;
  rel.r_info = R_ARM_REL32;
  Addr sym_addr = kTestAddress;

  Addr target_addr = base_addr_ + rel.r_offset;
  Addr target_value = *reinterpret_cast<Addr*>(target_addr);
  target_value += sym_addr - rel.r_offset;

  // Expected relocation calculation:
  //   *target += sym_addr - rel->r_offset;
  relocations_->ApplyResolvedReloc(&rel, sym_addr);

  EXPECT_EQ(target_value, *reinterpret_cast<Addr*>(target_addr));
}

TEST_F(RelocationsTest, R_ARM_RELATIVE) {
  rel_t rel;
  rel.r_offset = 4;
  rel.r_info = R_ARM_RELATIVE;
  Addr sym_addr = kTestAddress;

  Addr target_addr = base_addr_ + rel.r_offset;
  Addr target_value = *reinterpret_cast<Addr*>(target_addr);
  target_value += base_addr_;

  // Expected relocation calculation:
  //   *target += base_memory_address_;
  relocations_->ApplyResolvedReloc(&rel, sym_addr);

  EXPECT_EQ(target_value, *reinterpret_cast<Addr*>(target_addr));
}
#endif  // SB_IS(ARCH_ARM)

#if SB_IS(ARCH_ARM64) && defined(USE_RELA)
TEST_F(RelocationsTest, R_AARCH64_JUMP_SLOT) {
  rel_t rel;
  rel.r_offset = 0;
  rel.r_info = R_AARCH64_JUMP_SLOT;
  rel.r_addend = kTestAddend;
  Addr sym_addr = kTestAddress;

  // Expected relocation calculation:
  //   *target = sym_addr + addend;
  relocations_->ApplyResolvedReloc(&rel, sym_addr);

  VerifySymAddressPlusAddend(&rel, sym_addr);
}

TEST_F(RelocationsTest, R_AARCH64_GLOB_DAT) {
  rel_t rel;
  rel.r_offset = 1;
  rel.r_info = R_AARCH64_GLOB_DAT;
  rel.r_addend = kTestAddend;
  Addr sym_addr = kTestAddress;

  // Expected relocation calculation:
  //   *target = sym_addr + addend;
  relocations_->ApplyResolvedReloc(&rel, sym_addr);

  VerifySymAddressPlusAddend(&rel, sym_addr);
}

TEST_F(RelocationsTest, R_AARCH64_ABS64) {
  rel_t rel;
  rel.r_offset = 2;
  rel.r_info = R_AARCH64_ABS64;
  rel.r_addend = kTestAddend;
  Addr sym_addr = kTestAddress;

  Addr target_addr = base_addr_ + rel.r_offset;
  Addr target_value = *reinterpret_cast<Addr*>(target_addr);
  target_value += sym_addr + rel.r_addend;

  // Expected relocation calculation:
  //   *target += sym_addr + addend;
  relocations_->ApplyResolvedReloc(&rel, sym_addr);

  EXPECT_EQ(target_value, *reinterpret_cast<Addr*>(target_addr));
}

TEST_F(RelocationsTest, R_AARCH64_RELATIVE) {
  rel_t rel;
  rel.r_offset = 3;
  rel.r_info = R_AARCH64_RELATIVE;
  rel.r_addend = kTestAddend;
  Addr sym_addr = kTestAddress;

  // Expected relocation calculation:
  //   *target = base_memory_address_ + addend;
  relocations_->ApplyResolvedReloc(&rel, sym_addr);

  VerifyBaseAddressPlusAddend(&rel);
}

#endif  // SB_IS(ARCH_ARM64) && defined(USE_RELA)

#if SB_IS(ARCH_X86)
TEST_F(RelocationsTest, R_386_JMP_SLOT) {
  rel_t rel;
  rel.r_offset = 0;
  rel.r_info = R_386_JMP_SLOT;
  Addr sym_addr = kTestAddress;

  // Expected relocation calculation:
  //   *target = sym_addr;
  relocations_->ApplyResolvedReloc(&rel, sym_addr);

  VerifySymAddress(&rel, sym_addr);
}

TEST_F(RelocationsTest, R_386_GLOB_DAT) {
  rel_t rel;
  rel.r_offset = 1;
  rel.r_info = R_386_GLOB_DAT;
  Addr sym_addr = kTestAddress;

  // Expected relocation calculation:
  //   *target = sym_addr;
  relocations_->ApplyResolvedReloc(&rel, sym_addr);

  VerifySymAddress(&rel, sym_addr);
}

TEST_F(RelocationsTest, R_386_RELATIVE) {
  rel_t rel;
  rel.r_offset = 2;
  rel.r_info = R_386_RELATIVE;
  Addr sym_addr = kTestAddress;

  Addr target_addr = base_addr_ + rel.r_offset;
  Addr target_value = *reinterpret_cast<Addr*>(target_addr);
  target_value += base_addr_;

  // Expected relocation calculation:
  //   *target += base_memory_address_;
  relocations_->ApplyResolvedReloc(&rel, sym_addr);

  EXPECT_EQ(target_value, *reinterpret_cast<Addr*>(target_addr));
}

TEST_F(RelocationsTest, R_386_32) {
  rel_t rel;
  rel.r_offset = 3;
  rel.r_info = R_386_32;
  Addr sym_addr = kTestAddress;

  Addr target_addr = base_addr_ + rel.r_offset;
  Addr target_value = *reinterpret_cast<Addr*>(target_addr);
  target_value += sym_addr;

  // Expected relocation calculation:
  //   *target += sym_addr;
  relocations_->ApplyResolvedReloc(&rel, sym_addr);

  EXPECT_EQ(target_value, *reinterpret_cast<Addr*>(target_addr));
}

TEST_F(RelocationsTest, R_386_PC32) {
  rel_t rel;
  rel.r_offset = 4;
  rel.r_info = R_386_PC32;
  Addr sym_addr = kTestAddress;

  Addr target_addr = base_addr_ + rel.r_offset;
  Addr target_value = *reinterpret_cast<Addr*>(target_addr);
  Addr reloc = static_cast<Addr>(rel.r_offset + base_addr_);
  target_value += (sym_addr - reloc);

  // Expected relocation calculation:
  //   *target += (sym_addr - reloc);
  relocations_->ApplyResolvedReloc(&rel, sym_addr);

  EXPECT_EQ(target_value, *reinterpret_cast<Addr*>(target_addr));
}
#endif  // SB_IS(ARCH_X86)

#if SB_IS(ARCH_X64) && defined(USE_RELA)
TEST_F(RelocationsTest, R_X86_64_JMP_SLOT) {
  rel_t rel;
  rel.r_offset = 0;
  rel.r_info = R_X86_64_JMP_SLOT;
  rel.r_addend = kTestAddend;
  Addr sym_addr = kTestAddress;

  // Expected relocation calculation:
  //   *target = sym_addr + addend;
  relocations_->ApplyResolvedReloc(&rel, sym_addr);

  VerifySymAddressPlusAddend(&rel, sym_addr);
}

TEST_F(RelocationsTest, R_X86_64_GLOB_DAT) {
  rel_t rel;
  rel.r_offset = 1;
  rel.r_info = R_X86_64_GLOB_DAT;
  rel.r_addend = kTestAddend;
  Addr sym_addr = kTestAddress;

  // Expected relocation calculation:
  //   *target = sym_addr + addend;
  relocations_->ApplyResolvedReloc(&rel, sym_addr);

  VerifySymAddressPlusAddend(&rel, sym_addr);
}

TEST_F(RelocationsTest, R_X86_64_RELATIVE) {
  rel_t rel;
  rel.r_offset = 2;
  rel.r_info = R_X86_64_RELATIVE;
  rel.r_addend = kTestAddend;
  Addr sym_addr = kTestAddress;

  // Expected relocation calculation:
  //   *target = base_memory_address_ + addend;
  relocations_->ApplyResolvedReloc(&rel, sym_addr);

  VerifyBaseAddressPlusAddend(&rel);
}

TEST_F(RelocationsTest, R_X86_64_64) {
  rel_t rel;
  rel.r_offset = 3;
  rel.r_info = R_X86_64_64;
  rel.r_addend = kTestAddend;
  Addr sym_addr = kTestAddress;

  // Expected relocation calculation:
  //   *target = sym_addr + addend;
  relocations_->ApplyResolvedReloc(&rel, sym_addr);

  VerifySymAddressPlusAddend(&rel, sym_addr);
}

TEST_F(RelocationsTest, R_X86_64_PC32) {
  rel_t rel;
  rel.r_offset = 4;
  rel.r_info = R_X86_64_PC32;
  rel.r_addend = kTestAddend;
  Addr sym_addr = kTestAddress;

  relocations_->ApplyResolvedReloc(&rel, sym_addr);

  // Expected relocation calculation:
  //   *target = sym_addr + (addend - reloc);
  VerifySymAddressPlusAddendDelta(&rel, sym_addr);
}
#endif  // SB_IS(ARCH_X64) && defined(USE_RELA)

// Tests for the packed relative relocation (DT_RELR) decoder. The format is
// architecture independent: even entries are target addresses, odd entries
// are bitmaps covering the words after the last explicit target.
class RelrRelocationsTest : public ::testing::Test {
 protected:
  static constexpr size_t kNumWords = 64;
  static constexpr Addr kInitialValue = 0x1000;

  RelrRelocationsTest() {
    for (size_t i = 0; i < kNumWords; ++i) {
      words_[i] = kInitialValue;
    }
    base_addr_ = reinterpret_cast<Addr>(&words_[0]);
    exported_symbols_.reset(new ExportedSymbols());
  }

  // Builds the dynamic section from |relr_table| and initializes
  // |relocations_|. Returns the result of InitRelocations().
  bool Init(const Relr* relr_table, size_t relr_size, Addr relr_ent) {
    dynamic_table_[0].d_tag = DT_RELR;
    dynamic_table_[0].d_un.d_ptr =
        reinterpret_cast<Addr>(relr_table) - base_addr_;
    dynamic_table_[1].d_tag = DT_RELRSZ;
    dynamic_table_[1].d_un.d_val = relr_size;
    dynamic_table_[2].d_tag = DT_RELRENT;
    dynamic_table_[2].d_un.d_val = relr_ent;

    dynamic_section_.reset(
        new DynamicSection(base_addr_, dynamic_table_, 3, 0));
    dynamic_section_->InitDynamicSection();
    relocations_.reset(new Relocations(base_addr_, dynamic_section_.get(),
                                       exported_symbols_.get()));
    return relocations_->InitRelocations();
  }

  Addr WordOffset(size_t index) const { return index * sizeof(Addr); }

  void VerifyRelocated(size_t index) {
    EXPECT_EQ(kInitialValue + base_addr_, words_[index])
        << "word " << index << " should have been relocated";
  }

  void VerifyUntouched(size_t index) {
    EXPECT_EQ(kInitialValue, words_[index])
        << "word " << index << " should not have been relocated";
  }

  std::unique_ptr<Relocations> relocations_;
  Addr base_addr_;
  Addr words_[kNumWords];
  Dyn dynamic_table_[10];
  std::unique_ptr<DynamicSection> dynamic_section_;
  std::unique_ptr<ExportedSymbols> exported_symbols_;
};

TEST_F(RelrRelocationsTest, NoRelrTableIsNoop) {
  // InitRelocations() with no DT_RELR leaves the table unset; applying is a
  // no-op that succeeds.
  dynamic_section_.reset();
  Dyn empty_table[1];
  empty_table[0].d_tag = DT_NULL;
  DynamicSection dynamic_section(base_addr_, empty_table, 1, 0);
  dynamic_section.InitDynamicSection();
  Relocations relocations(base_addr_, &dynamic_section,
                          exported_symbols_.get());
  EXPECT_TRUE(relocations.InitRelocations());
  EXPECT_TRUE(relocations.ApplyRelrRelocations());
  VerifyUntouched(0);
}

TEST_F(RelrRelocationsTest, AddressEntryRelocatesSingleWord) {
  const Relr relr_table[] = {WordOffset(0)};
  ASSERT_TRUE(Init(relr_table, sizeof(relr_table), sizeof(Relr)));
  EXPECT_TRUE(relocations_->ApplyRelrRelocations());

  VerifyRelocated(0);
  VerifyUntouched(1);
}

TEST_F(RelrRelocationsTest, BitmapEntriesRelocateMarkedWords) {
  // Address entry applies word 0 and positions the window at word 1. The
  // first bitmap covers words 1..31: bit 1 -> word 1, bit 4 -> word 4. The
  // window then advances to word 32; the second bitmap's bit 1 -> word 32.
  const Relr relr_table[] = {
      WordOffset(0),
      static_cast<Relr>(1) | (static_cast<Relr>(1) << 1) |
          (static_cast<Relr>(1) << 4),
      static_cast<Relr>(1) | (static_cast<Relr>(1) << 1),
  };
  ASSERT_TRUE(Init(relr_table, sizeof(relr_table), sizeof(Relr)));
  EXPECT_TRUE(relocations_->ApplyRelrRelocations());

  VerifyRelocated(0);
  VerifyRelocated(1);
  VerifyRelocated(4);
  VerifyRelocated(32);
  VerifyUntouched(2);
  VerifyUntouched(3);
  VerifyUntouched(5);
  VerifyUntouched(31);
  VerifyUntouched(33);
}

TEST_F(RelrRelocationsTest, InterleavedAddressAndBitmapEntries) {
  // Address entries reset the window: word 0 explicitly, bitmap for word 2,
  // then a new address entry for word 40 and a bitmap for word 42.
  const Relr relr_table[] = {
      WordOffset(0),
      static_cast<Relr>(1) | (static_cast<Relr>(1) << 2),
      WordOffset(40),
      static_cast<Relr>(1) | (static_cast<Relr>(1) << 2),
  };
  ASSERT_TRUE(Init(relr_table, sizeof(relr_table), sizeof(Relr)));
  EXPECT_TRUE(relocations_->ApplyRelrRelocations());

  VerifyRelocated(0);
  VerifyRelocated(2);
  VerifyRelocated(40);
  VerifyRelocated(42);
  VerifyUntouched(1);
  VerifyUntouched(3);
  VerifyUntouched(41);
  VerifyUntouched(43);
}

TEST_F(RelrRelocationsTest, InvalidRelrEntSizeFailsInit) {
  const Relr relr_table[] = {WordOffset(0)};
  EXPECT_FALSE(Init(relr_table, sizeof(relr_table), sizeof(Relr) + 1));
}

TEST_F(RelrRelocationsTest, RelrEntDoesNotClobberPltGot) {
  // Regression test: DT_RELRENT used to fall through into the DT_PLTGOT
  // case and corrupt the stored PLT/GOT pointer. A table with all three
  // RELR tags must still initialize and apply cleanly.
  const Relr relr_table[] = {WordOffset(3)};
  ASSERT_TRUE(Init(relr_table, sizeof(relr_table), sizeof(Relr)));
  EXPECT_TRUE(relocations_->ApplyRelrRelocations());
  VerifyRelocated(3);
}

}  // namespace
}  // namespace elf_loader

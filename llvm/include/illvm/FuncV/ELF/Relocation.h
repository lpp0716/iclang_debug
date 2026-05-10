//===--- Relocation.h - ELF relocation -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// ELF relocation.
//
//===----------------------------------------------------------------------===/

#ifndef ILLVM_RELOCATION_H
#define ILLVM_RELOCATION_H

#include "illvm/FuncV/ELF/Reference.h"
#include "illvm/FuncV/ELF/SymbolTableSection.h"
#include "illvm/FuncV/ELF/Type.h"
#include "illvm/Support/Memory.h"

namespace illvm {
namespace funcv {
namespace elf {

class Relocation {
private:
  class ForRef {
  public:
    uint64_t symInfo;

    explicit ForRef(const uint64_t _symInfo) : symInfo(_symInfo) {}
  };

  const ForRef forRef;

  uint64_t r_offset;
  const uint64_t typeInfo;
  BPtr<const Symbol> sym;
  const int64_t r_addend;

  Relocation(const uint64_t _r_offset, const uint64_t r_info,
             const int64_t _r_addend)
      : forRef(r_info >> 32), r_offset(_r_offset), typeInfo(r_info & 0x0ff),
        r_addend(_r_addend) {}

public:
  explicit Relocation(const Elf_Rela *elf_rela)
      : Relocation(elf_rela->r_offset, elf_rela->r_info, elf_rela->r_addend) {}

  Relocation(const Relocation &) = delete;
  Relocation& operator=(const Relocation &) = delete;
  Relocation(Relocation &&) = default;

  uint64_t getROffset() const { return r_offset; }

  void initRef(const BPtr<const SymbolTableSection> &symTabSec);

  void subBaseOffset(const uint64_t baseOffset) {
    assert(r_offset >= baseOffset);
    r_offset -= baseOffset;
  }

  void addBaseOffset(const uint64_t baseOffset) {
    r_offset += baseOffset;
  }

  void write(Elf_Rela *rela) const;

  void dump(std::ostream &oss) const;

  std::string toString() const;
};

} // namespace elf
} // namespace funcv
} // namespace illvm

#endif // ILLVM_RELOCATION_H

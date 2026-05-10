//===--- RelocationSection.h - Relocation section ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// Relocation section.
//
//===----------------------------------------------------------------------===/

#ifndef ILLVM_RELOCATIONSECTION_H
#define ILLVM_RELOCATIONSECTION_H

#include "illvm/FuncV/ELF/Relocation.h"
#include "illvm/FuncV/ELF/Section.h"
#include "illvm/FuncV/ELF/SymbolTableSection.h"
#include "illvm/Support/Memory.h"

namespace illvm {
namespace funcv {
namespace elf {

class RelocationSection final : public Section {
private:
  struct ForDataRef {
    std::vector<OPtr<Relocation>> relocations;
  };

  ForDataRef forDataRef;

  std::vector<BPtr<const Relocation>> relocations;

  RelocationSection(const Elf_Shdr *shdr, const uint64_t _idx)
      : Section(SectionType::RelaTab, shdr, _idx) {}

  RelocationSection(const Elf_Shdr *shdr, const uint64_t _idx,
                    const uint8_t *data);

public:
  RelocationSection(const RelocationSection &) = delete;
  RelocationSection& operator=(const RelocationSection &) = delete;
  RelocationSection(RelocationSection &&) = default;

  static OPtr<RelocationSection> create(const Elf_Shdr *shdr,
                                        const uint64_t _idx) {
    return OPtr<RelocationSection>(new RelocationSection(shdr, _idx));
  }

  static OPtr<RelocationSection> create(const Elf_Shdr *shdr, const uint64_t _idx,
                                 const uint8_t *data) {
    return OPtr<RelocationSection>(new RelocationSection(shdr, _idx, data));
  }

  uint32_t getShInfo() const { return forHeaderRef.sh_info; }

  std::vector<OPtr<Relocation>> extractRelocations() {
    return std::move(forDataRef.relocations);
  }

  void initRef(const BPtr<const SymbolTableSection> &symTab,
               const BPtr<const Section> &targetSection) {
    link = symTab->getIdx();
    info = targetSection->getIdx();
  }

  void initRef(std::vector<BPtr<const Relocation>> &&_relocations) {
    relocations = std::move(_relocations);
  }

private:
  void layout() override;

  void writeDataImpl(uint8_t *buffer) const override;

  void dumpData(std::ostream &oss) const override;
};

} // namespace elf
} // namespace funcv
} // namespace illvm

#endif // ILLVM_RELOCATIONSECTION_H

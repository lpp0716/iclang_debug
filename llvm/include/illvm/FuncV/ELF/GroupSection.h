//===--- GroupSection.h - Group section ----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// Group section.
//
//===----------------------------------------------------------------------===/

#ifndef ILLVM_GROUPSECTION_H
#define ILLVM_GROUPSECTION_H

#include <vector>

#include "illvm/FuncV/ELF/Section.h"
#include "illvm/FuncV/ELF/StringTableSection.h"
#include "illvm/FuncV/ELF/SymbolTableSection.h"

namespace illvm {
namespace funcv {
namespace elf {

class GroupSection final : public Section {
private:
  struct ForDataRef {
    std::vector<uint32_t> secIdxes;
  };

  ForDataRef forDataRef;

  std::vector<BPtr<const Section>> groupSections;

  GroupSection(const Elf_Shdr *shdr, const uint8_t _idx, const uint8_t *data);

public:
  GroupSection(const GroupSection &) = delete;
  GroupSection& operator=(const GroupSection &) = delete;
  GroupSection(GroupSection &&) = default;

  static OPtr<GroupSection>
  create(const Elf_Shdr *shdr, const uint8_t _idx, const uint8_t *data) {
    return OPtr<GroupSection>(new GroupSection(shdr, _idx, data));
  }

  void initRef(const std::vector<OPtr<Section>> &sections,
             const BPtr<const SymbolTableSection> &symTab);

private:
  void layout() override;

  void writeDataImpl(uint8_t *buffer) const override;

  void dumpData(std::ostream &oss) const override;
};

} // namespace elf
} // namespace funcv
} // namespace illvm

#endif // ILLVM_GROUPSECTION_H

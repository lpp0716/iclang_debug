//===--- NormalSection.h - Normal ELF section -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// Text / data.
//
//===----------------------------------------------------------------------===/

#ifndef ILLVM_NORMALSECTION_H
#define ILLVM_NORMALSECTION_H

#include "illvm/FuncV/ELF/RelocationSection.h"
#include "illvm/FuncV/ELF/Section.h"
#include "illvm/FuncV/ELF/Symbol.h"

namespace illvm {
namespace funcv {
namespace elf {

class NormalSection final : public Section {
private:
  // We will create a copy of the data when writing.
  const uint8_t *data;
  std::optional<OPtr<RelocationSection>> relaSection;
  std::optional<std::vector<OPtr<Relocation>>> relocations;

  NormalSection(const Elf_Shdr *shdr, const uint64_t _idx, const uint8_t *_data)
      : Section(SectionType::Normal, shdr, _idx), data(_data) {}

public:
  NormalSection(const NormalSection &) = delete;
  NormalSection& operator=(const NormalSection &) = delete;
  NormalSection(NormalSection &&) = default;

  static OPtr<NormalSection> create(const Elf_Shdr *shdr, const uint64_t _idx,
                                    const uint8_t *_data) {
    return OPtr<NormalSection>(new NormalSection(shdr, _idx, _data));
  }

  void initRef(OPtr<RelocationSection> &&_relaSection) {
    relaSection = std::move(_relaSection);
    relocations = relaSection.value()->extractRelocations();
  }

  std::optional<OPtr<RelocationSection>> extractRelaSection();

private:
  void writeDataImpl(uint8_t *buffer) const override;
};

} // namespace elf
} // namespace funcv
} // namespace illvm

#endif // ILLVM_NORMALSECTION_H

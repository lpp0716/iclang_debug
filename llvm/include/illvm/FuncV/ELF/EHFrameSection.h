//===--- EHFrameSection.h - EHFrame section -------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// EHFrame section.
//
//===----------------------------------------------------------------------===/

#ifndef ILLVM_EHFRAMESECTION_H
#define ILLVM_EHFRAMESECTION_H

#include "illvm/FuncV/ELF/EhFrame.h"
#include "illvm/FuncV/ELF/RelocationSection.h"
#include "illvm/FuncV/ELF/Section.h"

namespace illvm {
namespace funcv {
namespace elf {

class EhFrameSection final : public Section {
private:
  std::vector<OPtr<CFI>> cfis;

  std::optional<OPtr<RelocationSection>> relaSection;

  // Return false means termination.
  static bool loadEntry(const uint8_t *&data, uint32_t &length,
                        uint64_t &extLength, uint32_t &entryFlag,
                        const uint8_t *&otherData);

  // Sort rOffsetToRela first.
  void linkRelaToCIEFDE(std::vector<OPtr<Relocation>> &&relaEntries);

  EhFrameSection(const Elf_Shdr *shdr, const uint64_t _idx,
                 const uint8_t *data);

public:
  EhFrameSection(const EhFrameSection &) = delete;
  EhFrameSection& operator=(const EhFrameSection &) = delete;
  EhFrameSection(EhFrameSection &&) = default;

  static OPtr<EhFrameSection>
  create(const Elf_Shdr *shdr, const uint64_t _idx, const uint8_t *data) {
    return OPtr<EhFrameSection>(new EhFrameSection(shdr, _idx, data));
  }

  void initRef(OPtr<RelocationSection> &&_relaSection);

  std::optional<OPtr<RelocationSection>> extractRelaSection();

private:
  void layout() override;

  void writeDataImpl(uint8_t *buffer) const override;

  void dumpData(std::ostream &oss) const override;
};

} // namespace elf
} // namespace funcv
} // namespace illvm

#endif // ILLVM_EHFRAMESECTION_H

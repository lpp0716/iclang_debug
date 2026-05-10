//===--- StringTableSection.h - String table section ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// String table section.
//
//===----------------------------------------------------------------------===/

#ifndef ILLVM_STRINGTABLESECTION_H
#define ILLVM_STRINGTABLESECTION_H

#include <string>
#include <vector>
#include <unordered_map>

#include "illvm/FuncV/ELF/Section.h"
#include "illvm/Support/Memory.h"

namespace illvm {
namespace funcv {
namespace elf {

class StringTableSection final : public Section {
private:
  struct ForDataRef {
    const char *data;
    // offset -> strRef.
    // It can only work before layout.
    std::unordered_map<uint64_t, BPtr<const StrRef>> offsetStrMap;
  };

  ForDataRef forDataRef;

  // The first string should be "".
  std::vector<OPtr<StrRef>> strs;

  StringTableSection(const Elf_Shdr *shdr, const uint8_t _idx,
                     const uint8_t *_data);

public:
  StringTableSection(const StringTableSection &) = delete;
  StringTableSection& operator=(const StringTableSection &) = delete;
  StringTableSection(StringTableSection &&) = default;

  static OPtr<StringTableSection>
  create(const Elf_Shdr *shdr, const uint8_t _idx, const uint8_t *_data) {
    return OPtr<StringTableSection>(new StringTableSection(shdr, _idx, _data));
  }

  BPtr<const StrRef> getStr(uint64_t strOff);

  BPtr<const StrRef> addStr(const std::string &str);

private:
  void layout() override;

  void writeDataImpl(uint8_t *buffer) const override;

  void dumpData(std::ostream &oss) const override;
};

} // namespace elf
} // namespace funcv
} // namespace illvm

#endif // ILLVM_STRINGTABLESECTION_H

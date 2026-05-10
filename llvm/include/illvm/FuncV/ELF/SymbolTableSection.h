//===--- SymbolTableSection.h - Symbol table section ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// Symbol table section.
//
//===----------------------------------------------------------------------===/

#ifndef ILLVM_SYMBOLTABLESECTION_H
#define ILLVM_SYMBOLTABLESECTION_H

#include "illvm/FuncV/ELF/Section.h"
#include "illvm/FuncV/ELF/StringTableSection.h"
#include "illvm/FuncV/ELF/Symbol.h"
#include "illvm/Support/Memory.h"

namespace illvm {
namespace funcv {
namespace elf {

// Empty shell, ownership is in SymbolTableSection.
class ExtSymTabSection final : public Section {
private:
  struct ForDataRef {
    std::vector<Elf_Word> indexes;
  };

  ForDataRef forDataRef;

  std::vector<BPtr<const Symbol>> symbols;

  ExtSymTabSection(const Elf_Shdr *shdr, const uint64_t _idx)
      : Section(SectionType::SymTabShNdx, shdr, _idx) {}

  ExtSymTabSection(const Elf_Shdr *shdr, const uint64_t _idx,
                     const uint8_t *data);

public:
  ExtSymTabSection(const ExtSymTabSection &) = delete;
  ExtSymTabSection& operator=(const ExtSymTabSection &) = delete;
  ExtSymTabSection(ExtSymTabSection &&) = default;

  static OPtr<ExtSymTabSection> create(const Elf_Shdr *shdr,
                                         const uint64_t _idx) {
    return OPtr<ExtSymTabSection>(new ExtSymTabSection(shdr, _idx));
  }

  static OPtr<ExtSymTabSection>
  create(const Elf_Shdr *shdr, const uint64_t _idx, const uint8_t *data) {
    return OPtr<ExtSymTabSection>(new ExtSymTabSection(shdr, _idx, data));
  }

  std::vector<Elf_Word> extractIndexes() { return std::move(forDataRef.indexes); }

  void initRef(BPtr<const IntRef> &&symTabSecIdx) {
    link = std::move(symTabSecIdx);
  }

  void initRef(std::vector<BPtr<const Symbol>> &&_symbols) {
    symbols = std::move(_symbols);
  }

private:
  void layout() override;

  void writeDataImpl(uint8_t *buffer) const override;

  void dumpData(std::ostream &oss) const override;
};

class SymbolTableSection final : public Section {
private:
  std::vector<OPtr<Symbol>> symbols;
  OPtr<IntRef> localSymNum;
  std::optional<OPtr<ExtSymTabSection>> extSymTabSection;

  SymbolTableSection(const Elf_Shdr *shdr, const uint64_t _idx,
                     const uint8_t *data);

public:
  SymbolTableSection(const SymbolTableSection &) = delete;
  SymbolTableSection& operator=(const SymbolTableSection &) = delete;
  SymbolTableSection(SymbolTableSection &&) = default;

  static OPtr<SymbolTableSection>
  create(const Elf_Shdr *shdr, const uint64_t _idx, const uint8_t *_data) {
    return OPtr<SymbolTableSection>(new SymbolTableSection(shdr, _idx, _data));
  }

  llvm::Error initRef(
      BPtr<StringTableSection> &strTabSec,
      const std::vector<OPtr<Section>> &sections,
          std::optional<OPtr<ExtSymTabSection>> &&_extSymTabSection);

  const auto &getSymbols() const { return symbols; }

private:
  void layout() override;

  bool needExtSymTabSec() const;

public:
  std::optional<OPtr<ExtSymTabSection>>
  extractExtSymTabSection(BPtr<StringTableSection> &strTabSec);

private:
  void writeDataImpl(uint8_t *buffer) const override;

  void dumpData(std::ostream &oss) const override;
};

} // namespace elf
} // namespace funcv
} // namespace illvm

#endif // ILLVM_SYMBOLTABLESECTION_H

//===--- ObjFile.h - ELF obj file ----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// Workflow:
// * Parse ELF header.
// * Parse section header table (Note: e_shnum, e_shstrndx).
// * Parse string table.
// * Parse .symtab_shndx, symbol table.
// * Parse other sections.
// * Init references.
// * Create necessary sections.
// * Update section layout.
// * Update section header and content (Note: e_shnum and e_shstrndx).
// * Write.
//
//===----------------------------------------------------------------------===/

#ifndef ILLVM_OBJFILE_H
#define ILLVM_OBJFILE_H

#include <string>
#include <vector>

#include "illvm/FuncV/ELF/Sections.h"
#include "illvm/Support/Memory.h"
#include "illvm/Support/BinFile.h"

namespace illvm {
namespace funcv {
namespace elf {

class ObjFile {
private:
  const BinFile &binFile;

  unsigned char e_ident[llvm::ELF::EI_NIDENT] = {};
  uint16_t e_type = 0;
  uint16_t e_machine = 0;
  uint32_t e_version = 0;
  uint64_t e_entry = 0;
  uint64_t e_phoff = 0;
  uint64_t e_shoff = 0;
  uint32_t e_flags = 0;
  uint16_t e_ehsize = 0;
  uint16_t e_phentsize = 0;
  uint64_t e_phnum = 0;
  uint16_t e_shentsize = 0;
  uint64_t e_shnum = 0;
  uint64_t e_shstrndx = 0;

  std::vector<OPtr<Section>> sections;

  BPtr<StringTableSection> strTabSec;
  BPtr<SymbolTableSection> symTabSec;

  std::optional<BPtr<EhFrameSection>> ehFrameSec;

  void parseHeader();

  void parseNecessarySections(const uint8_t *object,
                              const std::vector<const Elf_Shdr *> &shdrs);

  void parseOtherSections(const uint8_t *object,
                          const std::vector<const Elf_Shdr *> &shdrs);

  void parseSections();

  llvm::Error initRef();

  ObjFile() = delete;
  ObjFile(const BinFile &_binFile, llvm::Error &err);

public:
  ObjFile(const ObjFile &) = delete;
  ObjFile& operator=(const ObjFile &) = delete;
  ObjFile(ObjFile &&) = default;

  static llvm::Expected<ObjFile> create(const BinFile &_binFile) {
    llvm::Error err = llvm::Error::success();
    ObjFile objFile(_binFile, err);
    if (err) {
      return std::move(err);
    }
    return objFile;
  }

  BPtr<const SymbolTableSection> getSymTabSec() const {
    return symTabSec.constCopy();
  }

  void layout();

  void write(const std::string &outputPath) const;

  void dump(std::ostream &oss) const;

  std::string toString() const;
};

} // namespace elf
} // namespace funcv
} // namespace illvm

#endif // ILLVM_OBJFILE_H

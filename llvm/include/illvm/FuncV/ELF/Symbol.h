//===--- Symbol.h - ELF symbol -------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// ELF symbol.
//
//===----------------------------------------------------------------------===/

#ifndef ILLVM_SYMBOL_H
#define ILLVM_SYMBOL_H

#include <string>
#include <variant>
#include <vector>

#include "illvm/FuncV/ELF/Reference.h"
#include "illvm/FuncV/ELF/Section.h"
#include "illvm/FuncV/ELF/StringTableSection.h"
#include "illvm/FuncV/ELF/Type.h"
#include "illvm/Support/Memory.h"

namespace illvm {
namespace funcv {
namespace elf {

class Symbol {
private:
  class ForRef {
  public:
    uint32_t st_name;
    uint64_t st_shndx;

    ForRef(const uint32_t _st_name, const uint64_t _st_shndx)
        : st_name(_st_name), st_shndx(_st_shndx) {}
  };

  const ForRef forRef;

  // For section type, set st_name to 0.
  BPtr<const StrRef> name;
  // After checking, the symbol must be associated with an ordinary section,
  // and we will not make any modifications to the ordinary section,
  // so the value will not change either.
  // Note that it will be used to store reuse id.
  uint64_t st_value;
  uint32_t st_size;
  // The symbol's type and binding attributes.
  // #define ELF64_ST_BIND(info)          ((info) >> 4)
  // #define ELF64_ST_TYPE(info)          ((info) & 0xf)
  // #define ELF64_ST_INFO(bind, type)    (((bind)<<4)+((type)&0xf))
  uint8_t st_bind;
  uint8_t st_type;
  // A symbol's visibility.
  // #define ELF64_ST_VISIBILITY(o)       ((o)&0x3)
  uint8_t st_other;
  // If shndx holds int, it represents the special shndx,
  // (https://docs.oracle.com/cd/E26502_01/html/E26507/chapter6-94076.html#chapter6-tbl-16
  // , i.e., shndx == SHN_UNDEF || (SHN_LORESERVE <= shndx && shndx <
  // SHN_XINDEX).
  // Otherwise, it represents the target section index.
  //
  // Note that we do not consider SHN_XINDEX as a special shndx.
  // SHN_XINDEX means that the actual section header index is too large to fit
  // in this field. The actual value is contained in the associated section of
  // type SHT_SYMTAB_SHNDX.
  std::variant<uint64_t, BPtr<const Section>> shndx;

  // The index of this symbol in symbol header table.
  OPtr<IntRef> idx;

  uint8_t getStInfo() const { return (st_bind << 4) + (st_type & 0xf); }

  Symbol(const uint32_t _st_name, const uint64_t _st_value,
       const uint32_t _st_size, const uint8_t _st_info,
       const uint8_t _st_other, const uint64_t _st_shndx, const uint64_t _idx)
    : forRef(_st_name, _st_shndx), st_value(_st_value), st_size(_st_size),
      st_bind(_st_info >> 4), st_type(_st_info & 0xf), st_other(_st_other),
      idx(make_owner<IntRef>(_idx)) {}

public:
  explicit Symbol(const Elf_Sym *elf_sym, const uint64_t _idx)
      : Symbol(elf_sym->st_name, elf_sym->st_value, elf_sym->st_size,
               elf_sym->st_info, elf_sym->st_other, elf_sym->st_shndx, _idx) {}

  Symbol(const Symbol &) = delete;
  Symbol& operator=(const Symbol &) = delete;
  Symbol(Symbol &&) = default;

  std::string getName() const { return name->getValue(); }

  bool isFunc() const { return st_type == llvm::ELF::STT_FUNC; }

  bool isGlobal() const { return st_bind == llvm::ELF::STB_GLOBAL; }

  bool isWeak() const { return st_bind == llvm::ELF::STB_WEAK; }

  bool isLocal() const { return st_bind == llvm::ELF::STB_LOCAL; }

  BPtr<const IntRef> getIdx() const { return idx.constBorrow(); }

  llvm::Error
  initRef(BPtr<StringTableSection> &strTabSec,
          const std::vector<OPtr<Section>> &sections,
          const std::optional<std::vector<Elf_Word>> &indexes);

  void setReuseId(const uint64_t reuseId) { st_value = reuseId; }

  void layout(const uint64_t _idx);

  std::optional<uint64_t> getXSecIdx() const;

  void write(Elf_Sym *sym) const;

  void dump(std::ostream &oss) const;

  std::string toString() const;
};

} // namespace elf
} // namespace funcv
} // namespace illvm

#endif // ILLVM_SYMBOL_H

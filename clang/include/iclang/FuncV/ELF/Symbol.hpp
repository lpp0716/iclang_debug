//===--- Symbol.hpp - ELF symbol -----------------------------------------===//
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

#ifndef ICLANG_SYMBOL_HPP
#define ICLANG_SYMBOL_HPP

#include "iclang/FuncV/ELF/Config.hpp"
#include "iclang/FuncV/ELF/Reference.hpp"

namespace iclang {
namespace funcv {
namespace elf {

class Section;

class Symbol {
protected:
  // Update while writing, according to name.
  // For section type, we will set name to the actual string,
  // and set st_name to 0.
  uint32_t st_name;
  std::shared_ptr<StrRef> name;
  // Update while writing, according to value.
  uint64_t st_value;
  std::shared_ptr<IdxRef> value;
  uint32_t st_size;
  // The symbol's type and binding attributes.
  // #define ELF64_ST_BIND(info)          ((info) >> 4)
  // #define ELF64_ST_TYPE(info)          ((info) & 0xf)
  // #define ELF64_ST_INFO(bind, type)    (((bind)<<4)+((type)&0xf))
  unsigned char st_info;
  // A symbol's visibility.
  // #define ELF64_ST_VISIBILITY(o)       ((o)&0x3)
  unsigned char st_other;
  // If specialShndx != -1, it represents the special shndx,
  // (https://docs.oracle.com/cd/E26502_01/html/E26507/chapter6-94076.html#chapter6-tbl-16
  // , i.e., shndx == SHN_UNDEF || (SHN_LORESERVE <= shndx && shndx <
  // SHN_XINDEX) otherwise, we use secIdx to represent the target section index.
  // Note that we do not consider SHN_XINDEX as a special shndx.
  // SHN_XINDEX means that the actual section header index is too large to fit
  // in this field. The actual value is contained in the associated section of
  // type SHT_SYMTAB_SHNDX.
  //
  // Update while writing, according to specialShndx and secIdx.
  uint64_t st_shndx;
  int specialShndx;
  std::shared_ptr<IdxRef> secIdx; // May be nullptr (if specialShndx != -1).

  // The index of this symbol in symbol header table，
  // update while writing.
  std::shared_ptr<IdxRef> idx;

public:
  Symbol(const uint32_t _st_name, const std::shared_ptr<StrRef> &_name,
         const uint64_t _st_value, const std::shared_ptr<IdxRef> &_value,
         const uint32_t _st_size, const unsigned char _st_info,
         const unsigned char _st_other, const uint64_t _st_shndx,
         const int _specialShndx, const std::shared_ptr<IdxRef> &_secIdx,
         const std::shared_ptr<IdxRef> &_idx)
      : st_name(_st_name), name(_name), st_value(_st_value), value(_value),
        st_size(_st_size), st_info(_st_info), st_other(_st_other),
        st_shndx(_st_shndx), specialShndx(_specialShndx), secIdx(_secIdx),
        idx(_idx) {}

  Symbol(const llvm::object::ELF64LE::Sym *elf_sym)
      : Symbol(elf_sym->st_name, nullptr, elf_sym->st_value, nullptr,
               elf_sym->st_size, elf_sym->st_info, elf_sym->st_other,
               elf_sym->st_shndx, -1, nullptr, nullptr) {}

  uint32_t getStName() const { return st_name; }

  void setStName(const uint32_t _st_name) { st_name = _st_name; }

  std::shared_ptr<StrRef> getName() const { return name; }

  std::string getNameValue() const { return name->getValue(); }

  void setName(const std::shared_ptr<StrRef> &_name) { name = _name; }

  uint64_t getStValue() const { return st_value; }

  void setStValue(const uint64_t _st_value) { st_value = _st_value; }

  std::shared_ptr<IdxRef> getValue() const { return value; }

  uint64_t getValueValue() const { return value->getValue(); }

  void setValue(const std::shared_ptr<IdxRef> &_value) { value = _value; }

  uint32_t getStSize() const { return st_size; }

  void setStSize(const uint32_t _st_size) { st_size = _st_size; }

  unsigned char getStInfo() const { return st_info; }

  void setStInfo(const unsigned char _st_info) { st_info = _st_info; }

  // #define ELF64_ST_TYPE(info)          ((info) & 0xf)
  unsigned char getStType() const { return st_info & 0xf; }

  // #define ELF64_ST_BIND(info)          ((info) >> 4)
  unsigned char getStBind() const { return st_info >> 4; }

  unsigned char getStOther() const { return st_other; }

  void setStOther(const unsigned char _st_other) { st_other = _st_other; }

  uint64_t getStShndx() const { return st_shndx; }

  void setStShndx(const uint64_t _st_shndx) { st_shndx = _st_shndx; }

  int getSpecialShndx() const { return specialShndx; }

  void setSpecialShndx(const int _specialShndx) {
    specialShndx = _specialShndx;
  }

  std::shared_ptr<IdxRef> getSecIdx() const { return secIdx; }

  uint64_t getSecIdxValue() const { return secIdx->getValue(); }

  void setSecIdx(const std::shared_ptr<IdxRef> &_secIdx) { secIdx = _secIdx; }

  std::shared_ptr<IdxRef> getIdx() const { return idx; }

  uint64_t getIdxValue() const { return idx->getValue(); }

  void setIdx(const std::shared_ptr<IdxRef> &_idx) { idx = _idx; }

  void writeDataTo(llvm::object::ELF64LE::Sym *sym) const {
    sym->st_name = st_name;
    sym->st_value = st_value;
    sym->st_size = st_size;
    sym->st_info = st_info;
    sym->st_other = st_other;
    sym->st_shndx = st_shndx;
  }

  void dump(std::ostream &oss) const {
    oss << std::setfill(' ');
    oss << "[" << std::setw(5) << idx->getValue() << "] ";

    oss << std::hex;
    oss << std::setfill('0');
    oss << std::setw(16) << st_value << " ";

    oss << std::dec;
    oss << std::setfill(' ');
    oss << std::setw(6) << st_size << " ";

    oss << std::setw(8) << TypeToString::symbolTypeToString(getStType()) << " ";
    oss << std::setw(8) << TypeToString::symbolBindToString(getStBind()) << " ";
    oss << std::setw(8) << TypeToString::symbolVisToString(st_other) << " ";

    if (specialShndx == -1) {
      oss << std::setw(8) << st_shndx << " ";
    } else {
      oss << std::setw(8) << TypeToString::symbolSShndxToString(specialShndx)
          << " ";
    }

    oss << std::setw(20) << name->getValue() << " ";

    oss << "    (value IdxRef: " << value->getValue() << ")";
  }

  std::string toString() const {
    std::stringstream oss;
    dump(oss);
    return oss.str();
  }
};

} // namespace elf
} // namespace funcv
} // namespace iclang

#endif // ICLANG_SYMBOL_HPP

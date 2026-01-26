//===--- Relocation.hpp - ELF relocation -------------------------------===//
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

#ifndef ICLANG_RELOCATION_HPP
#define ICLANG_RELOCATION_HPP

#include "iclang/FuncV/ELF/Config.hpp"
#include "iclang/FuncV/ELF/Reference.hpp"
#include "iclang/FuncV/ELF/Symbol.hpp"

namespace iclang {
namespace funcv {
namespace elf {

class Relocation {
private:
  // Update while writing, according to offset.
  uint64_t r_offset;
  std::shared_ptr<IdxRef> offset;
  // Update while writing, according to sym.
  uint64_t r_info;
  std::shared_ptr<Symbol> sym;
  int64_t r_addend;

public:
  Relocation(const uint64_t r_offset, const std::shared_ptr<IdxRef> &_offset,
             const uint64_t r_info, const std::shared_ptr<Symbol> &sym,
             const int64_t r_addend)
      : r_offset(r_offset), offset(_offset), r_info(r_info), sym(sym),
        r_addend(r_addend) {}

  explicit Relocation(const llvm::object::ELF64LE::Rela *elf_rela)
      : Relocation(elf_rela->r_offset, nullptr, elf_rela->r_info, nullptr,
                   elf_rela->r_addend) {}

  uint64_t getROffset() const { return r_offset; }

  void setROffset(const uint64_t _r_offset) { r_offset = _r_offset; }

  std::shared_ptr<IdxRef> getOffset() const { return offset; }

  uint64_t getOffsetValue() const { return offset->getValue(); }

  void setOffset(const std::shared_ptr<IdxRef> &_offset) { offset = _offset; }

  uint64_t getRInfo() const { return r_info; }

  void setRInfo(const uint64_t _r_info) { r_info = _r_info; }

  uint64_t getSymbolInfo() const { return r_info >> 32; }

  void setSymbolInfo(const uint64_t symbolInfo) {
    r_info &= 0xffffffff;
    r_info |= (symbolInfo << 32);
  }

  uint64_t getTypeInfo() const { return r_info & 0x0ff; }

  std::shared_ptr<Symbol> getSym() const { return sym; }

  void setSym(const std::shared_ptr<Symbol> &_sym) { sym = _sym; }

  uint64_t getRAddend() const { return r_addend; }

  void setRAddend(const int64_t _addend) { r_addend = _addend; }

  void writeDataTo(llvm::object::ELF64LE::Rela *rela) const {
    rela->r_offset = r_offset;
    rela->r_info = r_info;
    rela->r_addend = r_addend;
  }

  void dump(std::ostream &oss) const {
    oss << std::hex;
    oss << std::setfill('0');
    oss << std::setw(16) << r_offset << " ";

    oss << std::setw(16) << r_info << " ";

    oss << std::setfill(' ');
    oss << std::setw(20) << TypeToString::relaTypeToString(getTypeInfo())
        << " ";

    oss << std::setfill('0');
    oss << std::setw(16) << sym->getStValue() << " ";

    oss << std::setfill(' ');
    oss << std::setw(20) << sym->getName()->getValue() << " ";

    if (r_addend < 0) {
      oss << "- " << -r_addend;
    } else {
      oss << "+ " << r_addend;
    }

    oss << std::dec;

    oss << "    (offset IdxRef: " << offset->getValue() << ")";
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

#endif // ICLANG_RELOCATION_HPP

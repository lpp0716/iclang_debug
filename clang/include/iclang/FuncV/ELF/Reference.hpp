//===--- Reference.hpp - Shared reference -------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// Important design of funcv.
// FuncV reuse will break the indexes of strings/symbols/relocations/sections.
// For example:
//
// ```
// .rela.text._Z4testv:
// 0000000000000005 0000000500000004 R_X86_64_PLT32 0000000000000000 _Z3foov - 4
//                  ________                                            A
//                         |                                            |
//                         ----------------------------------------------
//
// .rela.text.main:
// 0000000000000010 0000000500000004 R_X86_64_PLT32 0000000000000000 _Z3foov - 4
//                  ________                                            A
//                         |                                            |
//                         ----------------------------------------------
//
// .symtab:
// ...
// -----------------------<add a new symbol>---------------------
// 5(+1): 0000000000000000     8 FUNC    GLOBAL DEFAULT    3 _Z3foov
// 6(+1): 0000000000000000    11 FUNC    GLOBAL DEFAULT    4 _Z4testv
// 7(+1): 0000000000000000    26 FUNC    GLOBAL DEFAULT    6 main
// ```
//
// When we add a new symbol to .symtab, the index of _Z3foov may be updated
// from 5 to 6.
// However, the relocation entries related to _Z3foov in .rela.text._Z4testv and
// .rela.text.main still point to symbol 5, we should update them to 6.
//
// For convenience, we will create an IdxRef (which can be seen as int *)
// for the index of _Z3foov, and adjust the corresponding relocation entries in
// .rela.text._Z4testv and .rela.text.main to point this IdxRef.
// This way, when we update the IdxRef of _Z3foov,
// the corresponding relocation entries will also be updated.
//
// This design concept runs through the entire funcv.
//===----------------------------------------------------------------------===/

#ifndef ICLANG_REFERENCE_HPP
#define ICLANG_REFERENCE_HPP

#include "iclang/Global.hpp"

namespace iclang {
namespace funcv {
namespace elf {

// Decoupling.
class IdxRef {
private:
  // Update while writing.
  uint64_t value;

public:
  explicit IdxRef(const uint64_t _value) : value(_value) {}

  uint64_t getValue() const { return value; }

  void setValue(const uint64_t _value) { value = _value; }
};

class StrRef {
private:
  std::string value;
  // Update while writing.
  uint64_t offset;

public:
  StrRef(const std::string &_value, const uint64_t _offset)
      : value(_value), offset(_offset) {}

  std::string getValue() const { return value; }

  uint64_t getOffset() const { return offset; }

  void setOffset(const size_t _offset) { offset = _offset; }

  size_t getLength() const { return value.size(); }

  void dump(std::ostream &oss) const { oss << value << "(" << offset << ")"; }

  std::string toString() const {
    std::stringstream oss;
    dump(oss);
    return oss.str();
  }
};

} // namespace elf
} // namespace funcv
} // namespace iclang

#endif // ICLANG_REFERENCE_HPP

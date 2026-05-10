//===--- Reference.h - Shared reference ----------------------------------===//
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
//
// Life cycle:
// * Create in parseReference.
// * For Owner, update in layout.
// * For Borrowed, update in fini.
//
// Borrowed reference must be read only.
//
//===----------------------------------------------------------------------===/

#ifndef ILLVM_REFERENCE_H
#define ILLVM_REFERENCE_H

#include <cstdint>
#include <string>

namespace illvm {
namespace funcv {
namespace elf {

class IntRef {
private:
  uint64_t value;

public:
  explicit IntRef(const uint64_t _value) : value(_value) {}

  IntRef(const IntRef &) = delete;
  IntRef& operator=(const IntRef &) = delete;
  IntRef(IntRef &&) = default;

  uint64_t getValue() const { return value; }

  void setValue(const uint64_t _value) { value = _value; }
};

class StrRef {
private:
  std::string value;
  uint64_t offset;

public:
  StrRef(const std::string &_value, const uint64_t _offset)
      : value(_value), offset(_offset) {}

  StrRef(const StrRef &) = delete;
  StrRef& operator=(const StrRef &) = delete;
  StrRef(StrRef &&) = default;

  std::string getValue() const { return value; }

  uint64_t getOffset() const { return offset; }

  void setOffset(const size_t _offset) { offset = _offset; }

  size_t getLength() const { return value.size(); }

  void dump(std::ostream &oss) const;

  std::string toString() const;
};

} // namespace elf
} // namespace funcv
} // namespace illvm

#endif // ILLVM_REFERENCE_H

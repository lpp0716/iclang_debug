//===--- Section.h - ELF section -----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// ELF section.
//
// Design:
// Life cycle for all ELF and DWARF object:
// * Constructor
// * InitRef
// * Layout
// * Write
//
// All ELF and DWARF object can only be modified by life cycle functions.
//
//===----------------------------------------------------------------------===/

#ifndef ILLVM_SECTION_H
#define ILLVM_SECTION_H

#include <variant>

#include "illvm/FuncV/ELF/Reference.h"
#include "illvm/FuncV/ELF/Type.h"
#include "illvm/Support/Memory.h"

namespace illvm {
namespace funcv {
namespace elf {

class Section {
protected:
  const SectionType type;

  struct ForHeaderRef {
    uint32_t sh_name;
    uint32_t sh_link;
    uint32_t sh_info;
  };

  const ForHeaderRef forHeaderRef;

  BPtr<const StrRef> name;
  const uint32_t sh_type;
  const uint64_t sh_flags;
  const uint64_t sh_addr;
  uint64_t sh_offset;
  uint64_t sh_size;
  std::variant<uint64_t, BPtr<const IntRef>> link;
  // Refer to
  // https://docs.oracle.com/cd/E26502_01/html/E26507/chapter6-94076.html#chapter6-47976
  std::variant<uint64_t, BPtr<const IntRef>> info;
  const uint64_t sh_addralign;
  const uint64_t sh_entsize;

  // The index of this section in section header table.
  OPtr<IntRef> idx;

  Section(const SectionType _type, const uint32_t _sh_name,
          const uint32_t _sh_type, const uint64_t _sh_flags,
          const uint64_t _sh_addr, const uint64_t _sh_offset,
          const uint64_t _sh_size, const uint32_t _sh_link,
          const uint32_t _sh_info, const uint64_t _sh_addralign,
          const uint64_t _sh_entsize, const uint64_t _idx)
      : type(_type), forHeaderRef({_sh_name, _sh_link, _sh_info}),
        sh_type(_sh_type), sh_flags(_sh_flags), sh_addr(_sh_addr),
        sh_offset(_sh_offset), sh_size(_sh_size), sh_addralign(_sh_addralign),
        sh_entsize(_sh_entsize), idx(make_owner<IntRef>(_idx)) {
    link = _sh_link;
    info = _sh_info;
  }

  Section(const SectionType _type, const Elf_Shdr *shdr, const uint64_t _idx)
      : Section(_type, shdr->sh_name, shdr->sh_type, shdr->sh_flags,
                shdr->sh_addr, shdr->sh_offset, shdr->sh_size, shdr->sh_link,
                shdr->sh_info, shdr->sh_addralign, shdr->sh_entsize, _idx) {}

  Section() = delete;

  virtual void writeDataImpl(uint8_t *buffer) const = 0;

public:
  Section(const Section &) = delete;
  Section& operator=(const Section &) = delete;
  Section(Section &&) = default;
  virtual ~Section() = default;

  SectionType getType() const { return type; }

  std::string getName() const { return name->getValue(); }

  uint32_t getShName() const { return forHeaderRef.sh_name; }

  BPtr<const IntRef> getIdx() const { return idx.constBorrow(); }

  void initNameRef(BPtr<const StrRef> &&_name) { name = std::move(_name); }

  void layoutIdx(const uint64_t _idx) {
    idx->setValue(_idx);
  }

  // Update sh_size, section data order,
  // owner ref in section data.
  virtual void layout() {}

  bool isAllocable() const { return sh_flags & llvm::ELF::SHF_ALLOC; }

  uint64_t layoutOffset(const uint64_t baseOffset);

  void layoutFirstNullSecSize(const uint64_t _size) { sh_size = _size; }

  void layoutFirstNullSecLink(const uint64_t _link) { link = _link; }

  void writeHeader(Elf_Shdr *shdr) const;

  // Note: do not use uint8_t *&buffer, because we need 'buffer + sh_offset',
  // but not 'buffer + sh_size'
  void writeData(uint8_t *bufferBase) const {
    writeDataImpl(bufferBase + sh_offset);
  }

  void dumpHeader(std::ostream &oss) const;

  std::string headerToString() const;

  virtual void dumpData(std::ostream &oss) const {}

  std::string dataToString() const;
};

} // namespace elf
} // namespace funcv
} // namespace illvm

#endif // ILLVM_SECTION_H

//===--- EhFrame.h - ELF exception frame ----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// ELF exception frame.
//
//===----------------------------------------------------------------------===/

#ifndef ILLVM_EHFRAME_H
#define ILLVM_EHFRAME_H

#include <memory>
#include <string>
#include <vector>

#include "illvm/FuncV/ELF/Relocation.h"

namespace illvm {
namespace funcv {
namespace elf {

// Frame Description Entry.
class FDE {
  uint32_t length;
  uint64_t extLength;
  uint32_t ciePointer;
  const uint8_t *otherData;
  // For convenience, we will set the r_offset relative to the FDE itself
  // during initRef, and update it to the global offset during layout.
  std::optional<OPtr<Relocation>> pcBeginRelaEntry;
  std::vector<OPtr<Relocation>> otherRelaEntries;

public:
  FDE(const uint32_t _length, const uint64_t _extLength,
      const uint32_t _ciePointer, const uint8_t *_otherData)
      : length(_length), extLength(_extLength), ciePointer(_ciePointer),
        otherData(_otherData) {}

  FDE(const FDE &) = delete;
  FDE& operator=(const FDE &) = delete;
  FDE(FDE &&) = default;

  uint32_t getCIEPointerPreFilling() const {
    if (length == 0xffffffff) {
      return sizeof(uint32_t) + sizeof(uint64_t);
    }
    return sizeof(uint32_t);
  }

  uint32_t getPCBeginPreFilling() const {
    return length == 0xffffffff
               ? sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint32_t)
               : sizeof(uint32_t) + sizeof(uint32_t);
  }

  uint64_t getSize() const {
    if (length == 0xffffffff) {
      return extLength + sizeof(uint32_t) + sizeof(uint64_t);
    }
    return length + sizeof(uint32_t);
  }

  void initRef(const uint64_t baseOffset,
               std::optional<OPtr<Relocation>> &&_pcBeginRelaEntry,
               std::vector<OPtr<Relocation>> &&_otherRelaEntries) {
    pcBeginRelaEntry = std::move(_pcBeginRelaEntry);
    otherRelaEntries = std::move(_otherRelaEntries);

    if (pcBeginRelaEntry.has_value()) {
      pcBeginRelaEntry.value()->subBaseOffset(baseOffset);
    }
    for (auto &relaEntry : otherRelaEntries) {
      relaEntry->subBaseOffset(baseOffset);
    }
  }

  void getRelocations(std::vector<BPtr<const Relocation>> &res) const {
    if (pcBeginRelaEntry.has_value()) {
      res.emplace_back(pcBeginRelaEntry.value().constBorrow());
    }
    for (auto &relaEntry : otherRelaEntries) {
      res.emplace_back(relaEntry.constBorrow());
    }
  }

  void layout(const uint64_t cieBaseOffset, const uint64_t baseOffset) {
    ciePointer = baseOffset - cieBaseOffset +
                 (length == 0xffffffff ? sizeof(uint32_t) + sizeof(uint64_t)
                                       : sizeof(uint32_t));
    if (pcBeginRelaEntry.has_value()) {
      pcBeginRelaEntry.value()->addBaseOffset(baseOffset);
    }
    for (auto &relaEntry : otherRelaEntries) {
      relaEntry->addBaseOffset(baseOffset);
    }
  }

  void write(uint8_t *&buffer) const;

  void dump(std::ostream &oss) const;

  std::string toString() const;
};

// Common Information Entry.
class CIE {
  uint32_t length;
  uint64_t extLength;
  uint32_t cieID;
  const uint8_t *otherData;

  // For convenience, we will set the r_offset relative to the CIE itself
  // during initRef, and update it to the global offset during layout.
  std::vector<OPtr<Relocation>> relaEntries;

public:
  CIE(const uint32_t _length, const uint64_t _extLength, const uint32_t _cieID,
      const uint8_t *_otherData)
      : length(_length), extLength(_extLength), cieID(_cieID),
        otherData(_otherData) {}

  CIE(const CIE &) = delete;
  CIE& operator=(const CIE &) = delete;
  CIE(CIE &&) = default;

  uint64_t getSize() const {
    if (length == 0xffffffff) {
      return extLength + sizeof(uint32_t) + sizeof(uint64_t);
    }
    return length + sizeof(uint32_t);
  }

  void initRef(const uint64_t baseOffset, std::vector<OPtr<Relocation>> &&_relaEntries) {
    relaEntries = std::move(_relaEntries);
    for (auto &relaEntry : relaEntries) {
      relaEntry->subBaseOffset(baseOffset);
    }
  }

  void getRelocations(std::vector<BPtr<const Relocation>> &res) const {
    for (auto &relaEntry : relaEntries) {
      res.emplace_back(relaEntry.constBorrow());
    }
  }

  void layout(const uint64_t baseOffset) {
    for (auto &relaEntry : relaEntries) {
      relaEntry->addBaseOffset(baseOffset);
    }
  }

  void write(uint8_t *&buffer) const;

  void dump(std::ostream &oss) const;

  std::string toString() const;
};

// Call Frame Information Format.
class CFI {
public:
  OPtr<CIE> cie;
  std::vector<OPtr<FDE>> fdes;

  CFI() = delete;
  explicit CFI(OPtr<CIE> &&_cie) : cie(std::move(_cie)) {}

  CFI(const CFI &) = delete;
  CFI& operator=(const CFI &) = delete;
  CFI(CFI &&) = default;

  void write(uint8_t *&buffer) const;

  void dump(std::ostream &oss) const;

  std::string toString() const;
};

} // namespace elf
} // namespace funcv
} // namespace illvm

#endif // ILLVM_EHFRAME_H
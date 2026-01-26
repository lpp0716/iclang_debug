//===--- ReuseSymbol.hpp - Reuse symbol ----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// Reuse symbol (include .iclang.reusev).
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_REUSESYMBOL_HPP
#define ICLANG_REUSESYMBOL_HPP

#include "iclang/FuncV/ELF/Reuse/BDG.hpp"

namespace iclang {
namespace funcv {
namespace elf {
namespace reuse {

class ReuseSymbol {
private:
  static void replaceNewSymbol(const std::shared_ptr<Section> &newSection,
                               const std::shared_ptr<Symbol> &newSymbol,
                               const std::shared_ptr<Symbol> &oldSymbol) {
    newSymbol->setStValue(0);
    newSymbol->getValue()->setValue(oldSymbol->getValueValue());
    newSymbol->setStSize(oldSymbol->getStSize());
    newSymbol->setStInfo(oldSymbol->getStInfo());
    newSymbol->setStOther(oldSymbol->getStOther());

    // Update st_shndx.
    newSymbol->setStShndx(0);
    const int specialShndx = oldSymbol->getSpecialShndx();
    newSymbol->setSpecialShndx(specialShndx);
    if (specialShndx == -1) {
      assert(newSection != nullptr);
      newSymbol->setSecIdx(newSection->getIdx());
    }
  }

  static std::shared_ptr<Symbol>
  createNewSymbol(ObjFile &newObjFile,
                  const std::shared_ptr<Section> &newSection,
                  const std::shared_ptr<Symbol> &oldSymbol) {
    using ELF_Sym = llvm::object::ELF64LE::Sym;

    ELF_Sym sym;
    sym.st_name = 0;
    sym.st_value = oldSymbol->getStValue();
    sym.st_size = oldSymbol->getStSize();
    sym.st_info = oldSymbol->getStInfo();
    sym.st_other = oldSymbol->getStOther();
    sym.st_shndx = 0;

    // Create new symbol.
    const auto newSymbol = std::make_shared<Symbol>(&sym);

    // Create new idx ref.
    newSymbol->setIdx(
        std::make_shared<IdxRef>(newObjFile.getSymTab()->getSize()));

    // Create new str ref.
    const auto name = oldSymbol->getNameValue();
    newSymbol->setName(newObjFile.getStrTab()->push_back(name));

    // Create new value idx ref.
    newSymbol->setValue(std::make_shared<IdxRef>(oldSymbol->getValueValue()));

    // Update st_shndx.
    const int specialShndx = oldSymbol->getSpecialShndx();
    newSymbol->setSpecialShndx(specialShndx);
    if (specialShndx == -1) {
      assert(newSection != nullptr);
      newSymbol->setSecIdx(newSection->getIdx());
    }

    newObjFile.getSymTab()->push_back(newSymbol);

    return newSymbol;
  }

  static void handleReuseVersionSymbol(ObjFile &newObjFile,
                                       const uint64_t oldReuseVersion) {
    bool replaced = false;
    const auto &symbols = newObjFile.getSymTab()->getSymbols();
    for (const auto &symbol : symbols) {
      const auto name = symbol->getNameValue();
      if (name != ".iclang.reusev") {
        continue;
      }
      symbol->setStValue(oldReuseVersion + 1);
      replaced = true;
    }
    if (replaced) {
      return;
    }

    using ELF_Sym = llvm::object::ELF64LE::Sym;

    ELF_Sym sym;
    sym.st_name = 0;
    sym.st_value = oldReuseVersion + 1;
    sym.st_size = 0;
    sym.st_info = 0;
    sym.st_other = 0;
    sym.st_shndx = 0;

    // Create new symbol.
    const auto newSymbol = std::make_shared<Symbol>(&sym);

    // Create new idx ref.
    newSymbol->setIdx(
        std::make_shared<IdxRef>(newObjFile.getSymTab()->getSize()));

    // Create new str ref.
    const std::string name = ".iclang.reusev";
    newSymbol->setName(newObjFile.getStrTab()->push_back(name));

    // Create new value idx ref.
    newSymbol->setValue(std::make_shared<IdxRef>(oldReuseVersion + 1));

    newSymbol->setSpecialShndx(llvm::ELF::SHN_UNDEF);

    newObjFile.getSymTab()->push_back(newSymbol);
  }

public:
  static void run(ObjFile &newObjFile, const BDG &bdg) {
    const auto &funcVReuseNodes = bdg.getFuncVReuseNodes();

    for (const auto &p : funcVReuseNodes) {
      const auto reuseNode = p.second;

      // Update symbol.
      const auto oldSymbol = reuseNode->getOldSymbol();
      auto newSymbol = reuseNode->getNewSymbol();
      if (newSymbol == nullptr) {
        newSymbol =
            createNewSymbol(newObjFile, reuseNode->getNewSection(), oldSymbol);
        reuseNode->setNewSymbol(newSymbol);
      } else if (oldSymbol->getSecIdx() != nullptr &&
                 newSymbol->getSecIdx() == nullptr) {
        replaceNewSymbol(reuseNode->getNewSection(), newSymbol, oldSymbol);
      }
    }

    // Handle reuse version symbol.
    handleReuseVersionSymbol(newObjFile, bdg.getOldReuseVersion());
  }
};

} // namespace reuse
} // namespace elf
} // namespace funcv
} // namespace iclang

#endif // ICLANG_REUSESYMBOL_HPP

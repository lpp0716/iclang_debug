//===--- ReuseRelocation.hpp - Reuse relocation ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// Reuse relocation sections, adjust relocation entries (include .rela.eh_frame,
// relocations in CIE/FDE).
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_REUSERELOCATION_HPP
#define ICLANG_REUSERELOCATION_HPP

#include "iclang/FuncV/ELF/Reuse/BDG.hpp"

namespace iclang {
namespace funcv {
namespace elf {
namespace reuse {

class ReuseRelocation {
private:
  static std::shared_ptr<RelocationSection> createNewRelaSection(
      const std::shared_ptr<ReuseNode> &reuseNode, ObjFile &newObjFile,
      const std::shared_ptr<Section> &newSection,
      const std::shared_ptr<RelocationSection> &oldRelocationSection) {
    using Elf_Shdr = llvm::object::ELF64LE::Shdr;

    Elf_Shdr shdr;
    shdr.sh_name = 0;
    shdr.sh_type = oldRelocationSection->getShType();
    shdr.sh_flags = oldRelocationSection->getShFlags();
    shdr.sh_addr = 0;
    shdr.sh_offset = 0;
    shdr.sh_size = 0;
    shdr.sh_link = 0;
    shdr.sh_info = 0;
    shdr.sh_addralign = oldRelocationSection->getShAddralign();
    shdr.sh_entsize = oldRelocationSection->getEntSize();

    // Create new relocation section.
    const auto newRelocationSection =
        std::make_shared<RelocationSection>(&shdr, nullptr);

    // Create new idx ref.
    newRelocationSection->setIdx(
        std::make_shared<IdxRef>(newObjFile.getSections().size()));

    // Create new str ref.
    const auto name = oldRelocationSection->getNameValue();
    newRelocationSection->setName(newObjFile.getShstrTab()->push_back(name));

    // Update linkage.
    newRelocationSection->setLink(newObjFile.getSymTab()->getIdx());

    // Update info linkage.
    newRelocationSection->setInfoLink(newSection->getIdx());

    // Create relocation entries.
    const auto &dependencies = reuseNode->getDependencies();
    const auto &oldRelaEntries = oldRelocationSection->getRelocations();
    for (const auto &oldRelaEntry : oldRelaEntries) {
      llvm::object::ELF64LE::Rela rela;
      rela.r_offset = 0;
      rela.r_info = oldRelaEntry->getRInfo();
      rela.r_addend = oldRelaEntry->getRAddend();

      // Create new rela entry.
      const auto newRelaEntry = std::make_shared<Relocation>(&rela);

      // Update offset idxRef.
      newRelaEntry->setOffset(
          std::make_shared<IdxRef>(oldRelaEntry->getOffsetValue()));

      // Update symbol.
      const auto oldSymbol = oldRelaEntry->getSym();
      const auto oldName = oldSymbol->getNameValue();
      const auto it = dependencies.find(oldName);
      assert(it != dependencies.end());
      const auto targetReuseNode = it->second;
      const auto targetSymbol = targetReuseNode.lock()->getNewSymbol();
      assert(targetSymbol != nullptr);
      newRelaEntry->setSym(targetSymbol);

      newRelocationSection->push_back(newRelaEntry);
    }

    newObjFile.getSections().push_back(newRelocationSection);

    return newRelocationSection;
  }

  static void reuseCIERelaEntries(const std::shared_ptr<ReuseNode> &reuseNode,
                                   ObjFile &newObjFile,
                                   const std::shared_ptr<CFI> &newCFI,
                                   const std::shared_ptr<CFI> &oldCFI) {
    const auto &dependencies = reuseNode->getDependencies();

    const auto oldRelaEntries = oldCFI->getCIE()->getRelaEntries();
    for (const auto &oldRelaEntry : oldRelaEntries) {
      llvm::object::ELF64LE::Rela rela;
      rela.r_offset = 0;
      rela.r_info = oldRelaEntry->getRInfo();
      rela.r_addend = oldRelaEntry->getRAddend();

      // Create new rela entry.
      const auto newRelaEntry = std::make_shared<Relocation>(&rela);

      // Update offset idxRef.
      newRelaEntry->setOffset(
          std::make_shared<IdxRef>(oldRelaEntry->getOffsetValue()));

      // Update symbol.
      const auto oldSymbol = oldRelaEntry->getSym();
      const auto oldName = oldSymbol->getNameValue();
      const auto it = dependencies.find(oldName);
      assert(it != dependencies.end());
      const auto targetReuseNode = it->second;
      const auto targetSymbol = targetReuseNode.lock()->getNewSymbol();
      assert(targetSymbol != nullptr);
      newRelaEntry->setSym(targetSymbol);

      newCFI->getCIE()->addRelaEntry(newRelaEntry);
      newObjFile.getRelaEhFrame()->push_back(newRelaEntry);
    }
  }

  static void reuseFDERelaEntries(const std::shared_ptr<ReuseNode> &reuseNode,
                                   ObjFile &newObjFile,
                                   const std::shared_ptr<FDE> &newFDE,
                                   const std::shared_ptr<FDE> &oldFDE) {
    // (1) Handle PCBegin.
    const auto oldPCBeginRelaEntry = oldFDE->getPCBeginRelaEntry();
    if (oldPCBeginRelaEntry != nullptr) {
      llvm::object::ELF64LE::Rela rela;
      rela.r_offset = 0;
      rela.r_info = oldPCBeginRelaEntry->getRInfo();
      rela.r_addend = oldPCBeginRelaEntry->getRAddend();

      // Create new rela entry.
      const auto newPCBeginRelaEntry = std::make_shared<Relocation>(&rela);

      // Update offset idxRef.
      newPCBeginRelaEntry->setOffset(
          std::make_shared<IdxRef>(oldPCBeginRelaEntry->getOffsetValue()));

      // Update symbol.
      const auto targetSymbol = reuseNode->getNewSymbol();
      assert(targetSymbol != nullptr);
      newPCBeginRelaEntry->setSym(targetSymbol);

      newFDE->setPCBeginRelaEntry(newPCBeginRelaEntry);
      newObjFile.getRelaEhFrame()->push_back(newPCBeginRelaEntry);
    }

    // (2) Handle other rela.
    const auto &dependencies = reuseNode->getDependencies();

    const auto oldRelaEntries = oldFDE->getOtherRelaEntries();
    for (const auto &oldRelaEntry : oldRelaEntries) {
      llvm::object::ELF64LE::Rela rela;
      rela.r_offset = 0;
      rela.r_info = oldRelaEntry->getRInfo();
      rela.r_addend = oldRelaEntry->getRAddend();

      // Create new rela entry.
      const auto newRelaEntry = std::make_shared<Relocation>(&rela);

      // Update offset idxRef.
      newRelaEntry->setOffset(
          std::make_shared<IdxRef>(oldRelaEntry->getOffsetValue()));

      // Update symbol.
      const auto oldSymbol = oldRelaEntry->getSym();
      const auto oldName = oldSymbol->getNameValue();
      const auto it = dependencies.find(oldName);
      assert(it != dependencies.end());
      const auto targetReuseNode = it->second;
      const auto targetSymbol = targetReuseNode.lock()->getNewSymbol();
      assert(targetSymbol != nullptr);
      newRelaEntry->setSym(targetSymbol);

      newFDE->addOtherRelaEntry(newRelaEntry);
      newObjFile.getRelaEhFrame()->push_back(newRelaEntry);
    }
  }


public:
  static void run(ObjFile &newObjFile, const BDG &bdg) {
    const auto &funcVReuseNodes = bdg.getFuncVReuseNodes();

    // Reuse rela sections.
    std::unordered_map<std::shared_ptr<RelocationSection>,
                       std::shared_ptr<RelocationSection>>
        visited;

    for (const auto &p : funcVReuseNodes) {
      const auto reuseNode = p.second;

      const auto oldSection = reuseNode->getOldSection();
      if (oldSection == nullptr) {
        continue;
      }

      const auto oldRelaSection = reuseNode->getOldRelaSection();
      if (oldRelaSection == nullptr ||
          reuseNode->getNewRelaSection() != nullptr) {
        continue;
      }

      const auto it = visited.find(oldRelaSection);
      if (it != visited.end()) {
        reuseNode->setNewRelaSection(it->second);
        continue;
      }

      const auto newSection = reuseNode->getNewSection();
      assert(newSection != nullptr);
      const auto newRelaSection = createNewRelaSection(reuseNode, newObjFile,
                                                 newSection, oldRelaSection);
      reuseNode->setNewRelaSection(newRelaSection);
      visited.emplace(oldRelaSection, newRelaSection);
    }

    // Reuse eh_frame relocations.
    std::unordered_set<std::shared_ptr<CFI>> visitedCFI;
    std::unordered_set<std::shared_ptr<FDE>> visitedFDE;

    for (const auto &p : funcVReuseNodes) {
      const auto reuseNode = p.second;

      const auto oldCFI = reuseNode->getOldCFI();
      const auto newCFI = reuseNode->getNewCFI();
      if (oldCFI == nullptr || newCFI == nullptr) {
        continue;
      }

      if (visitedCFI.emplace(oldCFI).second) {
        reuseCIERelaEntries(reuseNode, newObjFile, newCFI, oldCFI);
      }

      const auto &oldFDE = reuseNode->getOldFDE();
      const auto &newFDE = reuseNode->getNewFDE();
      if (oldFDE == nullptr || newFDE == nullptr) {
        continue;
      }

      if (visitedFDE.emplace(oldFDE).second) {
        reuseFDERelaEntries(reuseNode, newObjFile, newFDE, oldFDE);
      }
    }
  }
};

} // namespace reuse
} // namespace elf
} // namespace funcv
} // namespace iclang

#endif // ICLANG_REUSERELOCATION_HPP

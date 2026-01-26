//===--- ReuseEhFrame.hpp - Reuse eh_frame -------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// Reuse eh_frame (the reuse of related relocations is the responsibility of
// ReuseRelocation.hpp).
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_REUSEEHFRAME_HPP
#define ICLANG_REUSEEHFRAME_HPP

#include "iclang/FuncV/ELF/Reuse/BDG.hpp"

namespace iclang {
namespace funcv {
namespace elf {
namespace reuse {

class ReuseEhFrame {
private:
  static std::shared_ptr<CFI>
  createNewCFI(ObjFile &newObjFile, const std::shared_ptr<CFI> &oldCFI) {
    const auto oldCIE = oldCFI->getCIE();
    // Create new CIE.
    const auto newCIE =
        std::make_shared<CIE>(oldCIE->getLength(), oldCIE->getExtLength(),
                              oldCIE->getCIEID(), oldCIE->getOtherData());
    // Create new CFI.
    const auto newCFI = std::make_shared<CFI>(newCIE);
    newObjFile.getEhFrame()->addCFI(newCFI);

    return newCFI;
  }

  static std::shared_ptr<FDE>
  createNewFDE(const std::shared_ptr<CFI> &newCFI,
               const std::shared_ptr<FDE> &oldFDE) {
    // Create FDE.
    const auto newFDE = std::make_shared<FDE>(
        oldFDE->getLength(), oldFDE->getExtLength(), 0, oldFDE->getOtherData());

    newCFI->addFDE(newFDE);

    return newFDE;
  }

public:
  static void run(ObjFile &newObjFile, const BDG &bdg) {
    const auto &funcVReuseNodes = bdg.getFuncVReuseNodes();

    // Old CFI -> new CFI.
    std::unordered_map<std::shared_ptr<CFI>, std::shared_ptr<CFI>>
        visitedCFI;

    // Old FDE -> new FDE.
    std::unordered_map<std::shared_ptr<FDE>, std::shared_ptr<FDE>>
        visitedFDE;

    for (const auto &p : funcVReuseNodes) {
      const auto reuseNode = p.second;

      const auto oldCFI = reuseNode->getOldCFI();
      if (oldCFI == nullptr) {
        continue;
      }

      // (1) Reuse CFI.
      std::shared_ptr<CFI> newCFI = reuseNode->getNewCFI();
      if (newCFI == nullptr) {
        const auto cfiIt = visitedCFI.find(oldCFI);
        if (cfiIt != visitedCFI.end()) {
          newCFI = cfiIt->second;
          reuseNode->setNewCFI(newCFI);
        } else {
          newCFI = createNewCFI(newObjFile, oldCFI);
          reuseNode->setNewCFI(newCFI);
          visitedCFI.emplace(oldCFI, newCFI);
        }
      }

      // (2) Reuse FDE.
      const auto oldFDE = reuseNode->getOldFDE();
      assert(oldFDE != nullptr);
      if (reuseNode->getNewFDE() == nullptr) {
        const auto fdeIt = visitedFDE.find(oldFDE);
        if (fdeIt != visitedFDE.end()) {
          reuseNode->setNewFDE(fdeIt->second);
        } else {
          const auto newFDE = createNewFDE(newCFI, oldFDE);
          reuseNode->setNewFDE(newFDE);
          visitedFDE.emplace(oldFDE, newFDE);
        }
      }
    }
  }
};

} // namespace reuse
} // namespace elf
} // namespace funcv
} // namespace iclang

#endif // ICLANG_REUSEEHFRAME_HPP

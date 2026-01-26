//===--- ReuseNode.hpp - BDG node ----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// BDG node.
// Identifier: oldSymbol (old symbol name).
// Record the mapping between:
// * Old symbol and new Symbol.
// * Old (text/data) section and new (text/data) section.
// * Old rela section and new rela section.
// * Old CFI/CIE/FDE and new CFI/CIE/FDE.
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_REUSENODE_HPP
#define ICLANG_REUSENODE_HPP

#include "iclang/FuncV/ELF/Section.hpp"

#include "iclang/Global.hpp"

namespace iclang {
namespace funcv {
namespace elf {
namespace reuse {

class ReuseNode {
private:
  // Identifier.
  std::shared_ptr<Symbol> oldSymbol = nullptr;
  std::shared_ptr<Symbol> newSymbol = nullptr;

  std::shared_ptr<Section> oldSection = nullptr;
  std::shared_ptr<Section> newSection = nullptr;

  std::shared_ptr<RelocationSection> oldRelaSection = nullptr;
  std::shared_ptr<RelocationSection> newRelaSection = nullptr;

  std::shared_ptr<CFI> oldCFI = nullptr;
  std::shared_ptr<CFI> newCFI = nullptr;

  std::shared_ptr<FDE> oldFDE = nullptr;
  std::shared_ptr<FDE> newFDE = nullptr;

  std::unordered_map<std::string, std::weak_ptr<ReuseNode>> dependencies;

public:
  explicit ReuseNode(const std::shared_ptr<Symbol> &_oldSymbol)
      : oldSymbol(_oldSymbol) {}

  std::string getIdr() const { return oldSymbol->getNameValue(); }

  std::shared_ptr<Symbol> getOldSymbol() const { return oldSymbol; }

  void setNewSymbol(const std::shared_ptr<Symbol> &_newSymbol) {
    newSymbol = _newSymbol;
  }

  std::shared_ptr<Symbol> getNewSymbol() const { return newSymbol; }

  void setOldSection(const std::shared_ptr<Section> &_oldSection) {
    oldSection = _oldSection;
  }

  std::shared_ptr<Section> getOldSection() const { return oldSection; }

  void setNewSection(const std::shared_ptr<Section> &_newSection) {
    newSection = _newSection;
  }

  std::shared_ptr<Section> getNewSection() const { return newSection; }

  void
  setOldRelaSection(const std::shared_ptr<RelocationSection> &_oldRelaSection) {
    oldRelaSection = _oldRelaSection;
  }

  std::shared_ptr<RelocationSection> getOldRelaSection() const {
    return oldRelaSection;
  }

  void
  setNewRelaSection(const std::shared_ptr<RelocationSection> &_newRelaSection) {
    newRelaSection = _newRelaSection;
  }

  std::shared_ptr<RelocationSection> getNewRelaSection() const {
    return newRelaSection;
  }

  void addDependencies(const std::shared_ptr<ReuseNode> &_reuseNode) {
    dependencies[_reuseNode->getIdr()] = _reuseNode;
  }

  const std::unordered_map<std::string, std::weak_ptr<ReuseNode>> &
  getDependencies() const {
    return dependencies;
  }

  std::shared_ptr<CFI> getOldCFI() const { return oldCFI; }

  void setOldCFI(const std::shared_ptr<CFI> &_oldCFI) { oldCFI = _oldCFI; }

  std::shared_ptr<CFI> getNewCFI() const { return newCFI; }

  void setNewCFI(const std::shared_ptr<CFI> &_newCFI) { newCFI = _newCFI; }

  std::shared_ptr<FDE> getOldFDE() const { return oldFDE; }

  void setOldFDE(const std::shared_ptr<FDE> &_oldFDE) { oldFDE = _oldFDE; }

  std::shared_ptr<FDE> getNewFDE() const { return newFDE; }

  void setNewFDE(const std::shared_ptr<FDE> &_newFDE) { newFDE = _newFDE; }

  void dump(std::ostream &oss) const {
    oss << "[idr] " << getIdr() << std::endl;

    oss << "[oldSymbol] ";
    if (oldSymbol == nullptr) {
      oss << "NULL\n";
    } else {
      oldSymbol->dump(oss);
      oss << "\n";
    }
    oss << "[newSymbol] ";
    if (newSymbol == nullptr) {
      oss << "NULL\n";
    } else {
      newSymbol->dump(oss);
      oss << "\n";
    }

    oss << "[oldSection] ";
    if (oldSection == nullptr) {
      oss << "NULL\n";
    } else {
      oldSection->dumpHeader(oss);
      oss << "\n";
    }
    oss << "[newSection] ";
    if (newSection == nullptr) {
      oss << "NULL\n";
    } else {
      newSection->dumpHeader(oss);
      oss << "\n";
    }

    oss << "[oldRelaSection] ";
    if (oldRelaSection == nullptr) {
      oss << "NULL\n";
    } else {
      oldRelaSection->dumpHeader(oss);
      oss << "\n";
      oldRelaSection->dumpData(oss);
    }
    oss << "[newRelaSection] ";
    if (newRelaSection == nullptr) {
      oss << "NULL\n";
    } else {
      newRelaSection->dumpHeader(oss);
      oss << "\n";
      newRelaSection->dumpData(oss);
    }

    oss << "[oldCFI-CIE] ";
    if (oldCFI == nullptr) {
      oss << "NULL\n";
    } else {
      oldCFI->getCIE()->dump(oss);
      oss << "\n";
    }
    oss << "[newCFI-CIE] ";
    if (newCFI == nullptr) {
      oss << "NULL\n";
    } else {
      newCFI->getCIE()->dump(oss);
      oss << "\n";
    }

    oss << "[oldFDE] ";
    if (oldFDE == nullptr) {
      oss << "NULL\n";
    } else {
      oldFDE->dump(oss);
      oss << "\n";
    }
    oss << "[newFDE] ";
    if (newFDE == nullptr) {
      oss << "NULL\n";
    } else {
      newFDE->dump(oss);
      oss << "\n";
    }

    oss << "[dependencies]";
    for (const auto &dep : dependencies) {
      oss << " " << dep.second.lock()->getIdr();
    }
    oss << "\n";
  }

  std::string toString() const {
    std::stringstream oss;
    dump(oss);
    return oss.str();
  }
};

} // namespace reuse
} // namespace elf
} // namespace funcv
} // namespace iclang

#endif // ICLANG_REUSENODE_HPP

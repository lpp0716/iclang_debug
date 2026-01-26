//===--- BDG.hpp - Binary dependency graph -------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// Dependencies:
// * Symbol - section dependencies.
// * Section symbol dependencies.
// * Relocation dependencies.
// * Exception frame dependencies.
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_BDG_HPP
#define ICLANG_BDG_HPP

#include "iclang/FuncV/ELF/ObjFile.hpp"
#include "iclang/FuncV/ELF/Reuse/ReuseNode.hpp"

namespace iclang {
namespace funcv {
namespace elf {
namespace reuse {

class BDG {
private:
  using ReuseNodeIdMapType =
      std::unordered_map<std::string, std::shared_ptr<ReuseNode>>;
  using SymbolNameMapType =
      std::unordered_map<std::string, std::shared_ptr<Symbol>>;
  using SymbolSectionMapType =
      std::unordered_map<std::shared_ptr<Symbol>, std::shared_ptr<Section>>;
  using SectionRelaMapType =
      std::unordered_map<std::shared_ptr<Section>,
                         std::shared_ptr<RelocationSection>>;

  // Load from symbol .iclang.reusev.
  uint64_t oldReuseVersion = 0;
  uint64_t newReuseVersion = 1;

  // Reuse node idr -> reuse node.
  ReuseNodeIdMapType reuseNodeIdrMap;

  // Save propagation results.
  ReuseNodeIdMapType funcVReuseNodes;

  void loadOldReuseVersion(ObjFile &oldObjFile) {
    const auto &symbols = oldObjFile.getSymTab()->getSymbols();
    for (const auto &symbol : symbols) {
      const auto name = symbol->getNameValue();
      if (name != ".iclang.reusev") {
        continue;
      }
      oldReuseVersion = symbol->getStValue();
      newReuseVersion = oldReuseVersion + 1;
    }
  }

  static void renameAnonymousSymbol(ObjFile &objFile,
                                 const uint64_t reuseVersion) {
    const auto &symbols = objFile.getSymTab()->getSymbols();
    const auto &sections = objFile.getSections();

    for (const auto &symbol : symbols) {
      const std::string symbolName = symbol->getNameValue();

      const auto secIdx = symbol->getSecIdx();
      if (secIdx == nullptr) {
        continue;
      }

      const auto section = sections[secIdx->getValue()];
      const auto sectionName = section->getNameValue();

      if (String::hasPrefix(symbolName, "GCC_except_table")) {
        // Rename symbol.
        if (symbolName.find("@") == std::string::npos) {
          const auto newSymbolName =
            symbolName + "@" + std::to_string(reuseVersion);
          symbol->setName(objFile.getStrTab()->push_back(newSymbolName));
        }
      } else if (String::hasPrefix(sectionName, ".rodata.str") ||
        String::hasPrefix(sectionName, ".rodata.cst")) {
        // Rename section.
        if (sectionName.find("@") == std::string::npos) {
          const auto newSectionName =
            sectionName + "@" + std::to_string(reuseVersion);
          section->setName(objFile.getShstrTab()->push_back(newSectionName));
        }

        // Rename symbol.
        if (symbolName.find("@") == std::string::npos) {
          const auto newSymbolName =
            symbolName + "@" + std::to_string(reuseVersion);
          symbol->setName(objFile.getStrTab()->push_back(newSymbolName));
        }
      }
    }
  }

  // Symbol name -> symbol.
  static SymbolNameMapType symbolNameMapping(ObjFile &objFile) {
    SymbolNameMapType res;

    const auto &symbols = objFile.getSymTab()->getSymbols();
    for (const auto &symbol : symbols) {
      const auto name = symbol->getNameValue();
      if (name.empty() || name == ".iclang.reusev") {
        continue;
      }

      const auto type = symbol->getStType();
      if (type != llvm::ELF::STT_FILE) {
        res[name] = symbol;
      }
    }

    return res;
  }

  // Symbol -> section.
  static SymbolSectionMapType symbolSectionMapping(ObjFile &objFile) {
    SymbolSectionMapType res;

    const auto &symbols = objFile.getSymTab()->getSymbols();
    const auto &sections = objFile.getSections();

    for (const auto &symbol : symbols) {
      // NOTYPE symbol can also have section ndx, e.g.
      // -------------------------------------------
      // | NOTYPE | LOCAL | DEFAULT | 3 | .LCPI0_0 |
      // -------------------------------------------
      const auto secIdx = symbol->getSecIdx();
      if (secIdx != nullptr) {
        res[symbol] = sections[secIdx->getValue()];
      }
    }

    return res;
  }

  // Section -> rela section.
  static SectionRelaMapType sectionRelaMapping(ObjFile &objFile) {
    SectionRelaMapType res;

    const auto &sections = objFile.getSections();

    for (const auto &section : sections) {
      const auto type = section->getType();
      if (type == SectionType::RelaTab) {
        const auto info = section->getInfoLink();
        if (info != nullptr) {
          res[sections[info->getValue()]] =
              std::static_pointer_cast<RelocationSection>(section);
        }
      }
    }

    return res;
  }

  void initReuseNodesWithSymbol(const SymbolNameMapType &oldSymbolNameMap,
                                const SymbolNameMapType &newSymbolNameMap) {
    for (auto &p : oldSymbolNameMap) {
      const auto oldSymbolName = p.first;
      const auto oldSymbol = p.second;

      const auto reuseNode = std::make_shared<ReuseNode>(oldSymbol);

      // matching
      const auto it = newSymbolNameMap.find(oldSymbolName);
      if (it != newSymbolNameMap.end()) {
        reuseNode->setNewSymbol(it->second);
      }

      reuseNodeIdrMap[oldSymbolName] = reuseNode;
    }
  }

  void addSectionsForReusedNodes(ObjFile &oldObjFile,
                                 ObjFile &newObjFile) const {
    // (1) Add text/data sections.

    // symbol -> section.
    const auto oldSymbolSectionMap = symbolSectionMapping(oldObjFile);
    const auto newSymbolSectionMap = symbolSectionMapping(newObjFile);

    for (const auto &p : reuseNodeIdrMap) {
      const auto reuseNode = p.second;

      // Update sections.
      const auto oldSymbol = reuseNode->getOldSymbol();
      auto it = oldSymbolSectionMap.find(oldSymbol);
      if (it != oldSymbolSectionMap.end()) {
        reuseNode->setOldSection(it->second);
      }
      const auto newSymbol = reuseNode->getNewSymbol();
      if (newSymbol != nullptr) {
        it = newSymbolSectionMap.find(newSymbol);
        if (it != newSymbolSectionMap.end()) {
          reuseNode->setNewSection(it->second);
        }
      }
    }

    // (2) Add rela sections.

    // section -> rela section.
    const auto oldSectionRelaMap = sectionRelaMapping(oldObjFile);
    const auto newSectionRelaMap = sectionRelaMapping(newObjFile);

    // llvm::errs() << "old section rela map:\n";
    // for (auto &p : oldSectionRelaMap) {
    //   llvm::errs() << p.first->getNameValue() << "->"
    //                << p.second->getNameValue() << "\n";
    // }
    //    llvm::errs() << "new section rela map:\n";
    //    for (auto &p : newSectionRelaMap) {
    //      llvm::errs() << p.first->getNameValue() << "->"
    //                   << p.second->getNameValue() << "\n";
    //    }

    for (const auto &p : reuseNodeIdrMap) {
      const auto reuseNode = p.second;

      // Update rela sections.
      const auto oldSection = reuseNode->getOldSection();
      if (oldSection != nullptr) {
        const auto it = oldSectionRelaMap.find(oldSection);
        if (it != oldSectionRelaMap.end()) {
          reuseNode->setOldRelaSection(it->second);
        }
      }
      const auto newSection = reuseNode->getNewSection();
      if (newSection != nullptr) {
        const auto it = newSectionRelaMap.find(newSection);
        if (it != newSectionRelaMap.end()) {
          reuseNode->setNewRelaSection(it->second);
        }
      }
    }
  }

  void addEHsForReuseNodes(ObjFile &oldObjFile, ObjFile &newObjFile) {
    const auto &oldCFIs = oldObjFile.getEhFrame()->getCFIs();
    for (const auto &cfi : oldCFIs) {
      const auto &fdes = cfi->getFDEs();
      for (const auto &fde : fdes) {
        const auto pcBeginRelaEntry = fde->getPCBeginRelaEntry();
        if (pcBeginRelaEntry == nullptr) {
          continue;
        }
        const auto symbolName = pcBeginRelaEntry->getSym()->getNameValue();
        const auto it = reuseNodeIdrMap.find(symbolName);
        if (it == reuseNodeIdrMap.end()) {
          continue;
        }
        const auto reuseNode = it->second;
        reuseNode->setOldCFI(cfi);
        reuseNode->setOldFDE(fde);
      }
    }

    const auto &newCFIs = newObjFile.getEhFrame()->getCFIs();
    for (const auto &cfi : newCFIs) {
      const auto &fdes = cfi->getFDEs();
      for (const auto &fde : fdes) {
        const auto pcBeginRelaEntry = fde->getPCBeginRelaEntry();
        if (pcBeginRelaEntry == nullptr) {
          continue;
        }
        const auto symbolName = pcBeginRelaEntry->getSym()->getNameValue();
        const auto it = reuseNodeIdrMap.find(symbolName);
        if (it == reuseNodeIdrMap.end()) {
          continue;
        }
        const auto reuseNode = it->second;
        reuseNode->setNewCFI(cfi);
        reuseNode->setNewFDE(fde);
      }
    }
  }

  // Example:
  // .text._ZL4testv -> _ZL4testv
  // .data._ZL1x -> _ZL1x
  // .rodata..L__const._ZL4testv.a -> .L__const._ZL4testv.a
  // Invalid: return "".
  static std::string extractSectionSymbolName(const std::string &name) {
    int dotCnt = 0;
    int dotIdx = -1;
    for (size_t i = 0; i < name.size(); i++) {
      if (name[i] != '.') {
        continue;
      }
      dotCnt += 1;
      if (dotCnt == 1) {
        continue;
      }
      if (dotCnt > 1) {
        dotIdx = i;
        break;
      }
    }
    if (dotIdx == -1) {
      return "";
    }
    return name.substr(dotIdx + 1);
  }

  void buildReuseNodesDependencies(ObjFile &oldObjFile) {
    for (const auto &p : reuseNodeIdrMap) {
      const auto reuseNode = p.second;

      // (1) sym <-> sym section
      const auto oldSymbol = reuseNode->getOldSymbol();
      if (oldSymbol->getStType() == llvm::ELF::STT_SECTION) {
        const auto name = oldSymbol->getNameValue();
        const auto targetSymbolName = extractSectionSymbolName(name);
        if (targetSymbolName != "") {
          const auto it = reuseNodeIdrMap.find(targetSymbolName);
          if (it != reuseNodeIdrMap.end()) {
            const auto targetReuseNode = it->second;
            reuseNode->addDependencies(targetReuseNode);
            targetReuseNode->addDependencies(reuseNode);
          }
        }
      }

      const auto oldRelaSection = reuseNode->getOldRelaSection();
      if (oldRelaSection == nullptr) {
        continue;
      }

      // (2) Extract dependencies from relocation entries.
      const auto &relaEntries = oldRelaSection->getRelocations();
      for (const auto &relaEntry : relaEntries) {
        const auto symbol = relaEntry->getSym();
        assert(symbol != nullptr);

        const auto name = symbol->getNameValue();
        const auto it = reuseNodeIdrMap.find(name);
        if (it != reuseNodeIdrMap.end()) {
          reuseNode->addDependencies(it->second);
        }
      }
    }

    // (3) Extract eh dependencies.
    const auto &cfis = oldObjFile.getEhFrame()->getCFIs();
    for (const auto &cfi : cfis) {
      const auto cie = cfi->getCIE();
      const auto &fdes = cfi->getFDEs();
      for (const auto &fde : fdes) {
        const auto pcBeginRelaEntry = fde->getPCBeginRelaEntry();
        if (pcBeginRelaEntry == nullptr) {
          continue;
        }

        const auto srcIt = reuseNodeIdrMap.find(
          pcBeginRelaEntry->getSym()->getNameValue());
        if (srcIt == reuseNodeIdrMap.end()) {
          continue;
        }
        const auto sourceReuseNode = srcIt->second;

        const auto &cieRelaEntries = cie->getRelaEntries();
        for (const auto &relaEntry : cieRelaEntries) {
          const auto targetIt = reuseNodeIdrMap.find(
            relaEntry->getSym()->getNameValue());
          if (targetIt != reuseNodeIdrMap.end()) {
            sourceReuseNode->addDependencies(targetIt->second);
          }
        }
        const auto &fdeRelaEntries = fde->getOtherRelaEntries();
        for (const auto &relaEntry : fdeRelaEntries) {
          const auto targetIt = reuseNodeIdrMap.find(
                      relaEntry->getSym()->getNameValue());
          if (targetIt != reuseNodeIdrMap.end()) {
            sourceReuseNode->addDependencies(targetIt->second);
          }
        }
      }
    }
  }

public:
  void build(ObjFile &oldObjFile, ObjFile &newObjFile) {
    // (1) Init old reuse version.
    loadOldReuseVersion(oldObjFile);

    // llvm::errs() << oldReuseVersion << "\n";
    // llvm::errs() << newReuseVersion << "\n";

    // (2) Rename anonymouse symbol.
    renameAnonymousSymbol(oldObjFile, oldReuseVersion);
    renameAnonymousSymbol(newObjFile, newReuseVersion);

    // llvm::errs() << "old obj file:==========\n";
    // llvm::errs() << oldObjFile.getSymTab()->dataToString() << "\n";
    // llvm::errs() << "new obj file:==========\n";
    // llvm::errs() << newObjFile.getSymTab()->dataToString() << "\n";

    // (3) Symbol mapping.
    const auto oldSymbolNameMap = symbolNameMapping(oldObjFile);
    const auto newSymbolNameMap = symbolNameMapping(newObjFile);

    // (4) Init reuse nodes with symbols.
    initReuseNodesWithSymbol(oldSymbolNameMap, newSymbolNameMap);

    // llvm::errs() << toString() << "\n";

    // (5) Add associated sections for reuse nodes.
    addSectionsForReusedNodes(oldObjFile, newObjFile);

    // llvm::errs() << toString() << "\n";

    // (6) Add CFI-CIE-FDEs for reuse nodes.
    addEHsForReuseNodes(oldObjFile, newObjFile);

    // llvm::errs() << toString() << "\n";

    // (7) Build dependencies between reuse nodes.
    buildReuseNodesDependencies(oldObjFile);

    // llvm::errs() << toString() << "\n";
  }

  void propagation(const std::unordered_set<std::string> &funcXSet) {
    std::queue<std::shared_ptr<ReuseNode>> que;
    for (const auto &funcMangledName : funcXSet) {
      const auto it = reuseNodeIdrMap.find(funcMangledName);
      if (it == reuseNodeIdrMap.end()) {
        continue;
      }
      que.push(it->second);
    }

    while (!que.empty()) {
      const auto curReuseNode = que.front();
      que.pop();

      // Cut.
      if (curReuseNode->getNewSymbol() != nullptr &&
          curReuseNode->getNewSection() != nullptr) {
        continue;
      }

      if (!funcVReuseNodes.emplace(curReuseNode->getIdr(), curReuseNode)
               .second) {
        continue;
      }

      for (const auto &p : curReuseNode->getDependencies()) {
        const auto childReuseNode = p.second.lock();
        que.push(childReuseNode);
      }
    }
  }

  const auto &getReuseNodeIdrMap() const { return reuseNodeIdrMap; }

  const auto &getFuncVReuseNodes() const { return funcVReuseNodes; }

  uint64_t getOldReuseVersion() const { return oldReuseVersion; }

  void dump(std::ostream &oss) const {
    for (const auto &p : reuseNodeIdrMap) {
      oss << "----------\n";
      const auto reuseNode = p.second;
      reuseNode->dump(oss);
    }
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

#endif // ICLANG_BDG_HPP

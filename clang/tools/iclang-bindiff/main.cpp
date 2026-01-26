#include "iclang/FuncV/ELF/FuncV.hpp"

using namespace iclang;
using namespace iclang::funcv::elf;

const std::set<std::string> focusSymbols = {
};

static bool dataCmp(const char *d1, const uint64_t size1,
  const char *d2, const uint64_t size2) {
  if (size1 != size2) {
    return false;
  }
  for (uint64_t i = 0; i < size1; i++) {
    if (d1[i] != d2[i]) {
      return false;
    }
  }
  return true;
}

static std::string dumpData(const char *data, const uint64_t size) {
  std::ostringstream oss;
  oss << std::hex;
  oss << std::setfill('0');
  for (uint64_t i = 0; i < size; i += 16) {
    if (i != 0) {
      oss << "\n";
    }
    // Address.
    oss << "0x" << std::setw(10) << i << ":";
    // Hex data.
    uint64_t j = 0;
    for (; j < 16 && i + j < size; ++j) {
      const uint8_t dataV = data[i + j];
      oss << " " << std::setw(2) << static_cast<unsigned>(dataV);
    }
    // Align.
    for (; j < 16; ++j) {
      oss << "   ";
    }
    oss << " ";
    // String data.
    j = 0;
    for (; j < 16 && i + j < size; ++j) {
      const uint8_t dataV = data[i + j];
      if (std::isprint(dataV)) {
        oss << dataV;
      } else {
        oss << ".";
      }
    }
  }
  oss << std::dec;
  std::setfill(' ');
  oss << '\n';
  return oss.str();
}

// Only support ELF64LEKind
int main(const int argc, char **argv) {
  if (argc != 3) {
    llvm::errs() << "Usage: " << argv[0] << "<old-obj-path> <new-obj-path>\n";
    return 1;
  }

  const std::string oldObjPath = argv[1];
  const std::string newObjPath = argv[2];

  const BinFile oldBinFile(oldObjPath);
  const BinFile newBinFile(newObjPath);

  ObjFile oldObjFile(oldBinFile);
  ObjFile newObjFile(newBinFile);

  // Parse ELF header and sections.
  oldObjFile.init();
  newObjFile.init();

  oldObjFile.fini();
  newObjFile.fini();

  llvm::errs() << "==================== Symbol CMP ====================\n";
  const auto &oldSymbols = oldObjFile.getSymTab()->getSymbols();
  const auto &newSymbols = newObjFile.getSymTab()->getSymbols();
  std::unordered_map<std::string, std::shared_ptr<Symbol>> oldSymbolMap;
  std::unordered_map<std::string, std::shared_ptr<Symbol>> newSymbolMap;
  const auto &oldSections  = oldObjFile.getSections();
  const auto &newSections = newObjFile.getSections();
  for (const auto &symbol : oldSymbols) {
    const std::string name = symbol->getNameValue();
    if (name.empty()) {
      continue;
    }
    const auto type = symbol->getStType();
    if (type == llvm::ELF::STT_FILE) {
      continue;
    }
    std::string sectionName;
    if (symbol->getSecIdx() != nullptr) {
      sectionName = oldSections[symbol->getSecIdxValue()]->getNameValue();
    }
    if (String::hasPrefix(sectionName, ".rodata.str") ||
        String::hasPrefix(sectionName, ".rodata.cst") ||
        String::hasPrefix(name, ".rodata.str") ||
        String::hasPrefix(name, ".L.str") ||
        String::hasPrefix(name, ".rodata.cst") ||
        String::hasPrefix(name, ".LCPI") ||
        String::hasPrefix(name, "GCC_except_table") ||
        name == ".iclang.reusev") {
      continue;
    }
    oldSymbolMap[name] = symbol;
  }
  for (const auto &symbol : newSymbols) {
    const std::string name = symbol->getNameValue();
    if (name.empty()) {
      continue;
    }
    const auto type = symbol->getStType();
    if (type == llvm::ELF::STT_FILE) {
      continue;
    }
    std::string sectionName;
    if (symbol->getSecIdx() != nullptr) {
      sectionName = newSections[symbol->getSecIdxValue()]->getNameValue();
    }
    if (String::hasPrefix(sectionName, ".rodata.str") ||
        String::hasPrefix(sectionName, ".rodata.cst") ||
        String::hasPrefix(name, ".rodata.str") ||
        String::hasPrefix(name, ".L.str") ||
        String::hasPrefix(name, ".rodata.cst") ||
        String::hasPrefix(name, ".LCPI") ||
        String::hasPrefix(name, "GCC_except_table") ||
        name == ".iclang.reusev") {
      continue;
    }
    newSymbolMap[name] = symbol;
  }
  llvm::errs() << "[Old - New]\n";
  for (const auto &p : oldSymbolMap) {
    if (newSymbolMap.find(p.first) == newSymbolMap.end()) {
      llvm::errs() << "[Old Symbol] " << p.second->getNameValue() << "\n";
    }
  }
  llvm::errs() << "[New - Old]\n";
  for (const auto &p : newSymbolMap) {
    if (oldSymbolMap.find(p.first) == oldSymbolMap.end()) {
      llvm::errs() << "[New Symbol] " << p.second->getNameValue() << "\n";
    }
  }

  llvm::errs() << "==================== Section CMP ====================\n";
  for (const auto &oldSymbol : oldSymbols) {
    const std::string symbolName = oldSymbol->getNameValue();
    if (symbolName.empty()) {
      continue;
    }
    const auto it = newSymbolMap.find(symbolName);
    if (it == newSymbolMap.end()) {
      continue;
    }
    const auto newSymbol = it->second;
    if (oldSymbol->getSecIdx() == nullptr) {
      if (newSymbol->getSecIdx() != nullptr) {
        llvm::errs() << symbolName << ": old secIdx == nullptr, but "
                                      "new secIdx != nullptr\n";
      }
      continue;
    }
    if (newSymbol->getSecIdx() == nullptr) {
      llvm::errs() << symbolName << ": old secIdx != nullptr, but "
                                    "new secIdx == nullptr\n";
      continue;
    }
    const auto oldSection = oldSections[oldSymbol->getSecIdxValue()];
    const auto newSection = newSections[newSymbol->getSecIdxValue()];
    const auto oldSectionName = oldSection->getNameValue();
    const auto newSectionName = newSection->getNameValue();
    if (oldSectionName != newSectionName) {
      llvm::errs() <<  symbolName << ": old section (" << oldSectionName <<
        ") name != new section (" << newSectionName << ") name\n";
    }
    if (!dataCmp(oldSection->getData(), oldSection->getShSize(),
      newSection->getData(), newSection->getShSize())) {
      llvm::errs() <<  symbolName << ": old section (" << oldSectionName <<
        ") data != new section (" << newSectionName << ") data\n";
      if (focusSymbols.count(symbolName) != 0) {
        llvm::errs() << dumpData(oldSection->getData(), oldSection->getShSize()) << "\n";
        llvm::errs() << dumpData(newSection->getData(), newSection->getShSize()) << "\n";
      }
    }
  }

  llvm::errs() << "==================== Eh_frame CMP ====================\n";
  std::unordered_map<std::string, std::shared_ptr<CIE>> oldCIEMap;
  std::unordered_map<std::string, std::shared_ptr<CIE>> newCIEMap;
  std::unordered_map<std::string, std::shared_ptr<FDE>> oldFDEMap;
  std::unordered_map<std::string, std::shared_ptr<FDE>> newFDEMap;
  const auto &oldCFIs = oldObjFile.getEhFrame()->getCFIs();
  const auto &newCFIs = newObjFile.getEhFrame()->getCFIs();
  for (const auto &cfi : oldCFIs) {
    const auto cie = cfi->getCIE();
    const auto &fdes = cfi->getFDEs();
    for (const auto &fde : fdes) {
      const auto rela = fde->getPCBeginRelaEntry();
      if (rela != nullptr) {
        oldCIEMap[rela->getSym()->getNameValue()] = cie;
        oldFDEMap[rela->getSym()->getNameValue()] = fde;
      }
    }
  }
  for (const auto &cfi : newCFIs) {
    const auto cie = cfi->getCIE();
    const auto &fdes = cfi->getFDEs();
    for (const auto &fde : fdes) {
      const auto rela = fde->getPCBeginRelaEntry();
      if (rela != nullptr) {
        newCIEMap[rela->getSym()->getNameValue()] = cie;
        newFDEMap[rela->getSym()->getNameValue()] = fde;
      }
    }
  }
  llvm::errs() << "[Old - New]\n";
  for (const auto &p : oldFDEMap) {
    if (newFDEMap.find(p.first) == newFDEMap.end()) {
      llvm::errs() << "[Old FDE] " << p.first << "\n";
    }
  }
  llvm::errs() << "[New - Old]\n";
  for (const auto &p : newFDEMap) {
    if (oldFDEMap.find(p.first) == oldFDEMap.end()) {
      llvm::errs() << "[New FDE] " << p.first << "\n";
    }
  }
  llvm::errs() << "[CIE/FDE Data Cmp]\n";
  for (const auto &p : oldFDEMap) {
    const auto symbolName = p.first;
    const auto oldFDE = p.second;
    const auto it = newFDEMap.find(symbolName);
    if (it == newFDEMap.end()) {
      continue;
    }
    const auto newFDE = it->second;

    const auto oldCIEP = oldCIEMap.find(symbolName);
    assert(oldCIEP != oldCIEMap.end());
    const auto oldCIE = oldCIEP->second;
    const auto newCIEP = newCIEMap.find(symbolName);
    assert(newCIEP != newCIEMap.end());
    const auto newCIE = newCIEP->second;

    if (!dataCmp(oldCIE->getOtherData(), oldCIE->getOtherDataSize(),
          newCIE->getOtherData(), newCIE->getOtherDataSize())) {
      llvm::errs() << "[CIE] " << symbolName << " data does not match\n";
      if (focusSymbols.count(symbolName) != 0) {
        llvm::errs() << dumpData(oldCIE->getOtherData(), oldCIE->getOtherDataSize()) << "\n";
        llvm::errs() << dumpData(newCIE->getOtherData(), newCIE->getOtherDataSize()) << "\n";
      }
    }

    if (!dataCmp(oldFDE->getOtherData(), oldFDE->getOtherDataSize(),
      newFDE->getOtherData(), newFDE->getOtherDataSize())) {
      llvm::errs() << "[FDE] " << symbolName << " data does not match\n";
      if (focusSymbols.count(symbolName) != 0) {
        llvm::errs() << dumpData(oldFDE->getOtherData(), oldFDE->getOtherDataSize()) << "\n";
        llvm::errs() << dumpData(newFDE->getOtherData(), newFDE->getOtherDataSize()) << "\n";
      }
    }

  }

  return 0;
}
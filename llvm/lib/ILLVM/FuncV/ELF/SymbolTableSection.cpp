#include "illvm/FuncV/ELF/SymbolTableSection.h"

#include <iomanip>
#include <sstream>

#include "illvm/Support/Diagnostics.h"

#include "llvm/IR/InlineAsm.h"

namespace illvm {
namespace funcv {
namespace elf {

ExtSymTabSection::ExtSymTabSection(const Elf_Shdr *shdr,
                                       const uint64_t _idx, const uint8_t *data)
    : ExtSymTabSection(shdr, _idx) {
  // Refer to llvm/include/llvm/Object/ELF.h::ELFFile::getSHNDXTable
  ILLVM_FCHECK(shdr->sh_entsize == sizeof(Elf_Word), "");
  ILLVM_FCHECK(shdr->sh_size % shdr->sh_entsize == 0, "");
  const size_t entryNum = shdr->sh_size / shdr->sh_entsize;
  forDataRef.indexes.resize(entryNum, 0);
  for (size_t i = 0; i < entryNum; i++) {
    const auto *shndx =
        reinterpret_cast<const Elf_Word *>(data + i * shdr->sh_entsize);
    forDataRef.indexes[i] = *shndx;
  }
}

void ExtSymTabSection::layout() {
  sh_size = symbols.size() * sizeof(Elf_Word);
}

void ExtSymTabSection::writeDataImpl(uint8_t *buffer) const {
  for (const auto &symbol : symbols) {
    auto secIdxOpt = symbol->getXSecIdx();
    const Elf_Word secIdx = secIdxOpt.has_value() ? secIdxOpt.value() : 0;
    MemoryUtils::write(buffer, &secIdx, sizeof(Elf_Word));
  }
}

void ExtSymTabSection::dumpData(std::ostream &oss) const {
  for (size_t i = 0; i < symbols.size(); i++) {
    auto secIdx = symbols[i]->getXSecIdx();
    oss << "[" << i << "] " << (secIdx.has_value() ? secIdx.value() : 0)
        << std::endl;
  }
}

SymbolTableSection::SymbolTableSection(const Elf_Shdr *shdr,
                                       const uint64_t _idx, const uint8_t *data)
    : Section(SectionType::SymTab, shdr, _idx) {
  ILLVM_FCHECK(shdr->sh_entsize == sizeof(Elf_Sym), "");
  ILLVM_FCHECK(shdr->sh_size % shdr->sh_entsize == 0, "");

  const size_t symNum = shdr->sh_size / shdr->sh_entsize;
  symbols.resize(symNum);

  for (size_t i = 0; i < symNum; i++) {
    const auto *sym =
        reinterpret_cast<const Elf_Sym *>(data + i * shdr->sh_entsize);
    symbols[i] = make_owner<Symbol>(sym, i);
  }

  uint64_t _localSymNum = 0;
  for (size_t i = 0; i < symbols.size(); i++) {
    const auto &symbol = symbols[i];
    if (symbol->isLocal()) {
      _localSymNum += 1;
      continue;
    }
    break;
  }
  localSymNum = make_owner<IntRef>(_localSymNum);
}

llvm::Error SymbolTableSection::initRef(
    BPtr<StringTableSection> &strTabSec,
    const std::vector<OPtr<Section>> &sections,
    std::optional<OPtr<ExtSymTabSection>> &&_extSymTabSection) {
  link = strTabSec->getIdx();
  info = localSymNum.constBorrow();

  extSymTabSection = std::move(_extSymTabSection);

  std::optional<std::vector<Elf_Word>> indexes = std::nullopt;
  if (extSymTabSection.has_value()) {
    indexes = extSymTabSection.value()->extractIndexes();
  }

  for (size_t i = 0; i < symbols.size(); i++) {
    auto &symbol = symbols[i];
    ILLVM_ETRANS(symbol->initRef(strTabSec, sections, indexes));
  }

  return llvm::Error::success();
}

void SymbolTableSection::layout() {
  // Local symbols need to be placed before global symbols.
  std::vector<OPtr<Symbol>> localSymbols;
  std::vector<OPtr<Symbol>> otherSymbols;
  for (size_t i = 0; i < symbols.size(); i++) {
    if (symbols[i]->isLocal()) {
      localSymbols.emplace_back(std::move(symbols[i]));
    } else {
      otherSymbols.emplace_back(std::move(symbols[i]));
    }
  }
  symbols.clear();
  symbols.reserve(localSymbols.size() + otherSymbols.size());
  for (size_t i = 0; i < localSymbols.size(); i++) {
    symbols.emplace_back(std::move(localSymbols[i]));
  }
  for (size_t i = 0; i < otherSymbols.size(); i++) {
    symbols.emplace_back(std::move(otherSymbols[i]));
  }

  localSymNum->setValue(localSymbols.size());

  for (size_t i = 0; i < symbols.size(); i++) {
    symbols[i]->layout(i);
  }

  sh_size = symbols.size() * sh_entsize;
}

bool SymbolTableSection::needExtSymTabSec() const {
  for (const auto &symbol : symbols) {
    auto secIdx = symbol->getXSecIdx();
    if (secIdx.has_value()) {
      return true;
    }
  }
  return false;
}

std::optional<OPtr<ExtSymTabSection>>
SymbolTableSection::extractExtSymTabSection(
    BPtr<StringTableSection> &strTabSec) {
  if (!needExtSymTabSec()) {
    return std::nullopt;
  }

  OPtr<ExtSymTabSection> res;

  if (!extSymTabSection.has_value()) {
    extSymTabSection = {};

    Elf_Shdr shdr;
    shdr.sh_name = 0;
    shdr.sh_type = llvm::ELF::SHT_SYMTAB_SHNDX;
    shdr.sh_flags = 0;
    shdr.sh_addr = 0;
    shdr.sh_offset = 0;
    shdr.sh_size = 0;
    shdr.sh_link = 0;
    shdr.sh_info = 0;
    shdr.sh_addralign = sizeof(Elf_Word);
    shdr.sh_entsize = sizeof(Elf_Word);

    res = ExtSymTabSection::create(&shdr, 0);
    res->initNameRef(strTabSec->addStr(".symtab_shndx"));
    res->initRef(getIdx());
  } else {
    res = std::move(extSymTabSection.value());
    extSymTabSection = std::nullopt;
  }

  std::vector<BPtr<const Symbol>> _symbols;
  _symbols.reserve(symbols.size());
  for (const auto &symbol : symbols) {
    _symbols.emplace_back(symbol.constBorrow());
  }
  res->initRef(std::move(_symbols));

  return res;
}

void SymbolTableSection::writeDataImpl(uint8_t *buffer) const {
  for (const auto &symbol : symbols) {
    auto *sym = reinterpret_cast<Elf_Sym *>(buffer);
    symbol->write(sym);
    buffer += sh_entsize;
  }
}

void SymbolTableSection::dumpData(std::ostream &oss) const {
  oss << std::setfill(' ');
  oss << "[" << std::setw(5) << "Nr" << "] ";
  oss << std::setw(16) << "Value" << " ";
  oss << std::setw(6) << "Size" << " ";
  oss << std::setw(8) << "Type" << " ";
  oss << std::setw(8) << "Bind" << " ";
  oss << std::setw(8) << "Vis" << " ";
  oss << std::setw(8) << "Ndx" << " ";
  oss << std::setw(20) << "Name" << " ";
  oss << std::endl;

  for (const auto &symbol : symbols) {
    symbol->dump(oss);
    oss << std::endl;
  }
}

} // namespace elf
} // namespace funcv
} // namespace illvm

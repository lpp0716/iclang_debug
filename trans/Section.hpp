//===--- Section.hpp - ELF section ---------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// ELF section.
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_SECTION_HPP
#define ICLANG_SECTION_HPP

#include "iclang/FuncV/ELF/Config.hpp"
#include "iclang/FuncV/ELF/EhFrame.hpp"
#include "iclang/FuncV/ELF/Reference.hpp"
#include "iclang/FuncV/ELF/Relocation.hpp"
#include "iclang/FuncV/ELF/Symbol.hpp"
#include "iclang/FuncV/ELF/Tools.hpp"

#include "llvm/Support/Endian.h"
#include <llvm/Support/LEB128.h>

namespace iclang {
namespace funcv {
namespace elf {

class Section {
protected:
  SectionType type;

  // Update while writing, according to name.
  uint32_t sh_name;
  std::shared_ptr<StrRef> name;
  uint32_t sh_type;
  uint64_t sh_flags;
  // Always 0.
  uint64_t sh_addr;
  // Update while writing.
  uint64_t sh_offset;
  // Update while writing.
  uint64_t sh_size;
  // Update while writing, according to link.
  uint32_t sh_link;
  // May be nullptr.
  std::shared_ptr<IdxRef> link;
  uint32_t sh_info;
  // Refer to
  // https://docs.oracle.com/cd/E26502_01/html/E26507/chapter6-94076.html#chapter6-47976 .
  // May be nullptr.
  std::shared_ptr<IdxRef> infoLink;
  uint64_t sh_addralign;
  uint64_t sh_entsize;

  // Section data.
  // Important: Once data is created, it cannot be modified.
  // We will recreate a new buffer of data while writing.
  const char *data;

  // The index of this section in section header table，
  // update while writing.
  std::shared_ptr<IdxRef> idx;

public:
  Section(const SectionType _type, const uint32_t _sh_name,
          const std::shared_ptr<StrRef> &_name, const uint32_t _sh_type,
          const uint64_t _sh_flags, const uint64_t _sh_addr,
          const uint64_t _sh_offset, const uint64_t _sh_size,
          const uint32_t _sh_link, const std::shared_ptr<IdxRef> &_link,
          const uint32_t _sh_info, const std::shared_ptr<IdxRef> &_info_link,
          const uint64_t _sh_addralign, const uint64_t _sh_entsize,
          const char *_data, const std::shared_ptr<IdxRef> &_idx)
      : type(_type), sh_name(_sh_name), name(_name), sh_type(_sh_type),
        sh_flags(_sh_flags), sh_addr(_sh_addr), sh_offset(_sh_offset),
        sh_size(_sh_size), sh_link(_sh_link), link(_link), sh_info(_sh_info),
        infoLink(_info_link), sh_addralign(_sh_addralign),
        sh_entsize(_sh_entsize), data(_data), idx(_idx) {}

  Section(const SectionType _type, const llvm::object::ELF64LE::Shdr *shdr,
          const char *_data)
      : Section(_type, shdr->sh_name, nullptr, shdr->sh_type, shdr->sh_flags,
                shdr->sh_addr, shdr->sh_offset, shdr->sh_size, shdr->sh_link,
                nullptr, shdr->sh_info, nullptr, shdr->sh_addralign,
                shdr->sh_entsize, _data, nullptr) {}

  virtual ~Section() = default;

  SectionType getType() const { return type; }

  uint64_t getShName() const { return sh_name; }

  void setShName(const uint64_t _sh_name) { sh_name = _sh_name; }

  std::shared_ptr<StrRef> getName() const { return name; }

  std::string getNameValue() const { return name->getValue(); }

  void setName(const std::shared_ptr<StrRef> &_name) { name = _name; }

  uint64_t getShType() const { return sh_type; }

  uint64_t getShFlags() const { return sh_flags; }

  uint64_t getShOffset() const { return sh_offset; }

  void setShOffset(const uint64_t offset) { sh_offset = offset; }

  uint64_t getShSize() const { return sh_size; }

  void setShSize(const uint64_t size) { sh_size = size; }

  uint64_t getShLink() const { return sh_link; }

  void setShLink(const uint64_t _sh_link) { sh_link = _sh_link; }

  std::shared_ptr<IdxRef> getLink() const { return link; }

  void setLink(const std::shared_ptr<IdxRef> &_link) { link = _link; }

  uint32_t getShInfo() const { return sh_info; }

  void setShInfo(const uint32_t _sh_info) { sh_info = _sh_info; }

  std::shared_ptr<IdxRef> getInfoLink() const { return infoLink; }

  void setInfoLink(const std::shared_ptr<IdxRef> &_info_link) {
    infoLink = _info_link;
  }

  uint64_t getShAddralign() const { return sh_addralign; }

  uint64_t getEntSize() const { return sh_entsize; }

  const char *getData() const { return data; }

  void setData(const char *_data) { data = _data; }

  std::shared_ptr<IdxRef> getIdx() const { return idx; }

  uint64_t getIdxValue() const { return idx->getValue(); }

  void setIdx(const std::shared_ptr<IdxRef> &_idx) { idx = _idx; }

  // Update order, idx, offset, size.
  virtual void layout() {}

  virtual void fini() {}

  void writeHeaderTo(llvm::object::ELF64LE::Shdr *shdr) const {
    shdr->sh_name = sh_name;
    shdr->sh_type = sh_type;
    shdr->sh_flags = sh_flags;
    shdr->sh_addr = sh_addr;
    shdr->sh_offset = sh_offset;
    shdr->sh_size = sh_size;
    shdr->sh_link = sh_link;
    shdr->sh_info = sh_info;
    shdr->sh_addralign = sh_addralign;
    shdr->sh_entsize = sh_entsize;
  }

  virtual void writeDataTo(char *buffer) { memcpy(buffer, data, sh_size); }

  void dumpHeader(std::ostream &oss) const {
    oss << std::setfill(' ');
    oss << "[" << std::setw(5) << idx->getValue() << "] ";
    oss << std::setw(20) << name->getValue() << " ";
    oss << std::setw(20) << TypeToString::sectionTypeToString(sh_type) << " ";
    oss << std::hex;
    oss << std::setfill('0');
    oss << std::setw(6) << sh_offset << " ";
    oss << std::setw(6) << sh_size << " ";
    oss << std::setw(4) << sh_entsize << " ";
    oss << std::dec;
    oss << std::setfill(' ');
    oss << std::setw(4) << TypeToString::sectionFlagToString(sh_flags) << " ";
    oss << std::setw(5) << sh_link << " ";
    oss << std::setw(4) << sh_info << " ";
    oss << std::setw(4) << sh_addralign;

    oss << "    " << "(link IdxRef: ";
    if (link == nullptr) {
      oss << "NULL";
    } else {
      oss << link->getValue();
    }
    oss << ", info IdxRef: ";
    if (infoLink == nullptr) {
      oss << "NULL";
    } else {
      oss << infoLink->getValue();
    }
    oss << ")";
  }

  std::string headerToString() const {
    std::stringstream oss;
    dumpHeader(oss);
    return oss.str();
  }

  virtual void dumpData(std::ostream &oss) const {
    oss << std::hex;
    oss << std::setfill('0');
    for (uint64_t i = 0; i < sh_size; i += 16) {
      if (i != 0) {
        oss << "\n";
      }
      // Address.
      oss << "0x" << std::setw(10) << i << ":";
      // Hex data.
      uint64_t j = 0;
      for (; j < 16 && i + j < sh_size; ++j) {
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
      for (; j < 16 && i + j < sh_size; ++j) {
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
  }

  virtual std::string dataToString() const {
    std::stringstream oss;
    dumpData(oss);
    return oss.str();
  }
};

class OrdinarySection final : public Section {
private:
public:
  OrdinarySection(const llvm::object::ELF64LE::Shdr *shdr, const char *_data)
      : Section(SectionType::Ordinary, shdr, _data) {}
};

class StringTableSection final : public Section {
private:
  // offset -> strRef.
  // It can only work during parsing.
  std::map<uint64_t, std::shared_ptr<StrRef>> originalIndexes;
  // The first string should be "".
  std::vector<std::shared_ptr<StrRef>> strs;

public:
  StringTableSection(const llvm::object::ELF64LE::Shdr *shdr, const char *_data)
      : Section(SectionType::StrTab, shdr, _data) {
    const auto firstStrRef = std::make_shared<StrRef>(data, 0);
    originalIndexes[0] = firstStrRef;
    strs.push_back(firstStrRef);
    for (uint64_t i = 0; i < sh_size; i++) {
      if (data[i] == '\0' && i + 1 < sh_size) {
        const auto strRef = std::make_shared<StrRef>(data + i + 1, i + 1);
        originalIndexes[i + 1] = strRef;
        strs.push_back(strRef);
      }
    }
  }

  std::shared_ptr<StrRef> parseOriginalIndex(uint64_t strOff) {
    const auto it = originalIndexes.find(strOff);
    if (it != originalIndexes.end()) {
      return it->second;
    }
    // Handle string overlap (compression).
    const auto strRef = std::make_shared<StrRef>(data + strOff, strOff);
    originalIndexes[strOff] = strRef;
    strs.push_back(strRef);
    return strRef;
  }

  void layout() override {
    uint64_t strOff = 0;
    sh_size = 0;
    for (const auto &strRef : strs) {
      strRef->setOffset(strOff);
      strOff += strRef->getLength() + 1;
      sh_size += strRef->getLength() + 1;
    }
  }

  void writeDataTo(char *buffer) override {
    uint64_t strOff = 0;
    for (const auto &strRef : strs) {
      const uint64_t length = strRef->getLength() + 1;
      memcpy(buffer + strOff, strRef->getValue().data(), length);
      strOff += length;
    }
  }

  void dumpData(std::ostream &oss) const override {
    for (size_t i = 0; i < strs.size(); i++) {
      oss << "[" << i << "] \"";
      strs[i]->dump(oss);
      oss << "\"\n";
    }
  }

  std::string dataToString() const override {
    std::stringstream oss;
    dumpData(oss);
    return oss.str();
  }

  // Add an existed str ref to strtab.
  void push_back(const std::shared_ptr<StrRef> &str) { strs.push_back(str); }

  // Add a new str to strtab.
  std::shared_ptr<StrRef> push_back(const std::string &str) {
    const auto res = std::make_shared<StrRef>(str, 0);
    strs.push_back(res);
    return res;
  }
};

// Updated by SymbolTableSection.
class SymtabShndxSection final : public Section {
private:
  // idx -> symbol shndx.
  std::vector<uint32_t> originalIndexes;
  // Reconstruction by SymbolTableSection.
  std::vector<uint32_t> indexes;

public:
  friend class SymbolTableSection;

  SymtabShndxSection(const llvm::object::ELF64LE::Shdr *shdr, const char *_data)
      : Section(SectionType::SymTabShNdx, shdr, _data) {
    const auto &logger = Logger::getInstance();

    // Refer to llvm/include/llvm/Object/ELF.h::ELFFile::getSHNDXTable
    if (shdr->sh_entsize != sizeof(llvm::object::ELF64LE::Word)) {
      logger.fatal("Invalid symtab shndx: shdr->sh_entsize != sizeof(Word)");
    }
    if (shdr->sh_size % shdr->sh_entsize != 0) {
      logger.fatal(
          "Invalid symtab shndx: shdr->sh_size % shdr->sh_entsize != 0");
    }
    const size_t symNum = shdr->sh_size / shdr->sh_entsize;
    originalIndexes.reserve(symNum);
    for (size_t i = 0; i < symNum; i++) {
      const auto *shndx = reinterpret_cast<const llvm::object::ELF64LE::Word *>(
          _data + i * shdr->sh_entsize);
      originalIndexes.push_back(*shndx);
    }
  }

  void writeDataTo(char *buffer) override {
    for (size_t i = 0; i < indexes.size(); i++) {
      auto *shndx = reinterpret_cast<llvm::object::ELF64LE::Word *>(
          buffer + i * sh_entsize);
      *shndx = indexes[i];
    }
  }

  void dumpData(std::ostream &oss) const override {
    for (size_t i = 0; i < indexes.size(); i++) {
      oss << "[" << i << "] " << indexes[i] << "\n";
    }
  }

  std::string dataToString() const override {
    std::stringstream oss;
    dumpData(oss);
    return oss.str();
  }
};

class SymbolTableSection final : public Section {
private:
  using Elf_Sym = llvm::object::ELF64LE::Sym;
  std::shared_ptr<SymtabShndxSection> symtabShndx;
  std::vector<std::shared_ptr<Symbol>> symbols;

  // Update after layout.
  uint64_t localSymNum;

public:
  SymbolTableSection(const llvm::object::ELF64LE::Shdr *shdr,
                     const char *_data)
      : Section(SectionType::SymTab, shdr, _data), symtabShndx(nullptr),
        localSymNum(0) {
    const auto &logger = Logger::getInstance();

    if (shdr->sh_entsize != sizeof(Elf_Sym)) {
      logger.fatal("Invalid symbol table: shdr->sh_entsize != sizeof(Elf_Sym)");
    }
    if (shdr->sh_size % shdr->sh_entsize != 0) {
      logger.fatal(
          "Invalid symbol table: shdr->sh_size % shdr->sh_entsize != 0");
    }

    const size_t symNum = shdr->sh_size / shdr->sh_entsize;
    symbols.reserve(symNum);

    for (size_t i = 0; i < symNum; i++) {
      const auto *sym =
          reinterpret_cast<const Elf_Sym *>(_data + i * shdr->sh_entsize);
      const auto symbol = std::make_shared<Symbol>(sym);
      symbols.push_back(symbol);
    }
  }

  void
  parseReferences(const std::vector<std::shared_ptr<Section>> &sections,
                  const std::shared_ptr<StringTableSection> &strTab) const {
    for (size_t i = 0; i < symbols.size(); i++) {
      const auto symbol = symbols[i];
      // Name offset -> strRef.
      symbol->setName(strTab->parseOriginalIndex(symbol->getStName()));
      // Value -> idxRef
      symbol->setValue(std::make_shared<IdxRef>(symbol->getStValue()));

      // Handle shndx.
      const uint64_t shndx = symbol->getStShndx();
      if (shndx == llvm::ELF::SHN_UNDEF || (llvm::ELF::SHN_LORESERVE <= shndx &&
          shndx < llvm::ELF::SHN_XINDEX)) {
        symbol->setSpecialShndx(shndx);
      } else {
        uint64_t secIdx = 0;
        if (shndx == llvm::ELF::SHN_XINDEX) {
          assert(symtabShndx != nullptr);
          secIdx = symtabShndx->originalIndexes[i];
        } else {
          secIdx = shndx;
        }
        const auto &section = sections[secIdx];
        // Handle section symbol name.
        if (symbol->getStType() == llvm::ELF::STT_SECTION) {
          symbol->setName(section->getName());
        }
        // shndx -> sec idx ref.
        symbol->setSecIdx(section->getIdx());
      }

      // idx -> idxRef.
      symbol->setIdx(std::make_shared<IdxRef>(i));
    }
  }

  void layout() override {
    // Local symbols need to be placed before global symbols.
    std::vector<std::shared_ptr<Symbol>> temp;
    temp.reserve(symbols.size());
    // Local
    for (const auto &symbol : symbols) {
      if (symbol->getStBind() != llvm::ELF::STB_LOCAL) {
        continue;
      }
      temp.push_back(symbol);
    }
    localSymNum = temp.size();
    // Other
    for (const auto &symbol : symbols) {
      if (symbol->getStBind() == llvm::ELF::STB_LOCAL) {
        continue;
      }
      temp.push_back(symbol);
    }
    symbols = temp;

    // Update idx, size (symtab + shndxtab).
    for (size_t i = 0; i < symbols.size(); i++) {
      const auto &symbol = symbols[i];
      symbol->getIdx()->setValue(i);
    }
    sh_size = symbols.size() * sh_entsize;
    if (symtabShndx != nullptr) {
      symtabShndx->sh_size = symbols.size() * symtabShndx->sh_entsize;
    }
  }

  // Call after section layout.
  bool needSymtabShNdx() const {
    for (const auto &symbol : symbols) {
      const int specialShndx = symbol->getSpecialShndx();
      if (specialShndx != -1) {
        continue;
      }
      const uint64_t secIdx = symbol->getSecIdxValue();
      if (secIdx >= llvm::ELF::SHN_LORESERVE) {
        return true;
      }
    }
    return false;
  }

  void fini() override {
    // Reconstruction by SymbolTableSection.
    if (symtabShndx != nullptr) {
      symtabShndx->indexes.resize(symbols.size(), 0);
    }

    for (size_t i = 0; i < symbols.size(); i++) {
      const auto &symbol = symbols[i];
      // Update name.
      if (symbol->getStType() == llvm::ELF::STT_SECTION) {
        symbol->setStName(0);
      } else {
        symbol->setStName(symbol->getName()->getOffset());
      }
      // Update value.
      symbol->setStValue(symbol->getValueValue());
      // Update st_shndx.
      const int specialShndx = symbol->getSpecialShndx();
      if (specialShndx != -1) {
        symbol->setStShndx(specialShndx);
      } else {
        const uint64_t secIdx = symbol->getSecIdxValue();
        if (secIdx >= llvm::ELF::SHN_LORESERVE) {
          symbol->setStShndx(llvm::ELF::SHN_XINDEX);
          // Update SymtabShndxSection.
          symtabShndx->indexes[i] = secIdx;
        } else {
          symbol->setStShndx(secIdx);
        }
      }
    }

    // Update info.
    sh_info = localSymNum;
  }

  void writeDataTo(char *buffer) override {
    for (size_t i = 0; i < symbols.size(); i++) {
      auto *sym = reinterpret_cast<Elf_Sym *>(buffer + i * sh_entsize);
      symbols[i]->writeDataTo(sym);
    }
  }

  void dumpData(std::ostream &oss) const override {
    oss << std::setfill(' ');
    oss << "[" << std::setw(5) << "Nr" << "] ";
    oss << std::setw(16) << "Value" << " ";
    oss << std::setw(6) << "Size" << " ";
    oss << std::setw(8) << "Type" << " ";
    oss << std::setw(8) << "Bind" << " ";
    oss << std::setw(8) << "Vis" << " ";
    oss << std::setw(8) << "Ndx" << " ";
    oss << std::setw(20) << "Name" << " ";
    oss << "\n";

    for (const auto &symbol : symbols) {
      symbol->dump(oss);
      oss << "\n";
    }
  }

  std::string dataToString() const override {
    std::stringstream oss;
    dumpData(oss);
    return oss.str();
  }

  size_t getSize() const { return symbols.size(); }

  auto getSymbol(const size_t idx) { return symbols[idx]; }

  void push_back(const std::shared_ptr<Symbol> &symbol) {
    symbols.push_back(symbol);
  }

  const auto &getSymbols() const { return symbols; }

  void setSymtabShndx(const std::shared_ptr<SymtabShndxSection> &_symtabShndx) {
    symtabShndx = _symtabShndx;
  }

  auto getSymtabShndx() { return symtabShndx; }
};

class RelocationSection final : public Section {
private:
  using Elf_Rela = llvm::object::ELF64LE::Rela;
  // index -> relocation entry.
  std::vector<std::shared_ptr<Relocation>> relocations;

public:
  RelocationSection(const llvm::object::ELF64LE::Shdr *shdr, const char *_data)
      : Section(SectionType::RelaTab, shdr, _data) {
    const auto &logger = Logger::getInstance();

    if (shdr->sh_entsize != sizeof(Elf_Rela)) {
      logger.fatal(
          "Invalid symbol table: shdr->sh_entsize != sizeof(Elf_Rela)");
    }
    if (shdr->sh_size % shdr->sh_entsize != 0) {
      logger.fatal(
          "Invalid symbol table: shdr->sh_size % shdr->sh_entsize != 0");
    }

    const size_t relocationNum = shdr->sh_size / shdr->sh_entsize;
    relocations.reserve(relocationNum);

    for (size_t i = 0; i < relocationNum; i++) {
      const auto *rela =
          reinterpret_cast<const Elf_Rela *>(_data + i * shdr->sh_entsize);
      const auto relocation = std::make_shared<Relocation>(rela);
      relocations.push_back(relocation);
    }
  }

  void parseReferences(const std::shared_ptr<SymbolTableSection> &symTab) const {
    for (const auto &relocation : relocations) {
      // Handle offset idxRef
      relocation->setOffset(std::make_shared<IdxRef>(relocation->getROffset()));
      // Handle symbol ref.
      const auto sym = symTab->getSymbol(relocation->getSymbolInfo());
      relocation->setSym(sym);
    }
  }

  void layout() override { sh_size = relocations.size() * sh_entsize; }

  void fini() override {
    for (const auto &relocation : relocations) {
      // Update r_offset.
      relocation->setROffset(relocation->getOffsetValue());
      // Update symbol info.
      const auto idx = relocation->getSym()->getIdxValue();
      relocation->setSymbolInfo(idx);
    }
  }

  void writeDataTo(char *buffer) override {
    for (size_t i = 0; i < relocations.size(); i++) {
      auto *rela = reinterpret_cast<Elf_Rela *>(buffer + i * sh_entsize);
      relocations[i]->writeDataTo(rela);
    }
  }

  void dumpData(std::ostream &oss) const override {
    oss << std::setfill(' ');
    oss << std::setw(16) << "Offset" << " ";
    oss << std::setw(16) << "Info" << " ";
    oss << std::setw(20) << "Type" << " ";
    oss << std::setw(16) << "SymbolValue" << " ";
    oss << std::setw(20) << "SymbolName" << " ";
    oss << "Addend" << " ";
    oss << "\n";

    for (const auto &relocation : relocations) {
      relocation->dump(oss);
      oss << "\n";
    }
  }

  std::string dataToString() const override {
    std::stringstream oss;
    dumpData(oss);
    return oss.str();
  }

  const auto &getRelocations() const  {
    return relocations;
  }

  void push_back(const std::shared_ptr<Relocation> &relocation) {
    relocations.push_back(relocation);
  }
};

class GroupSection final : public Section {
private:
  std::vector<std::shared_ptr<Section>> sections;

public:
  GroupSection(const llvm::object::ELF64LE::Shdr *shdr, const char *_data)
      : Section(SectionType::Group, shdr, _data) {
    const auto &logger = Logger::getInstance();

    if (shdr->sh_entsize != sizeof(uint32_t)) {
      logger.fatal(
          "Invalid group section: shdr->sh_entsize != sizeof(Elf_Rela)");
    }
    if (shdr->sh_size % shdr->sh_entsize != 0) {
      logger.fatal(
          "Invalid group section: shdr->sh_size % shdr->sh_entsize != 0");
    }
  }

  void
  parseReferences(const std::vector<std::shared_ptr<Section>> &allSections) {
    const size_t num = sh_size / sh_entsize;
    const uint32_t *secNdx =
        reinterpret_cast<uint32_t *>(const_cast<char *>(data));
    for (size_t i = 1; i < num; i++) {
      sections.push_back(allSections[secNdx[i]]);
    }
  }

  void layout() override {
    sh_size = sections.size() * sh_entsize + sh_entsize;
  }

  void writeDataTo(char *buffer) override {
    uint64_t off = 0;
    uint32_t comdatFlag = llvm::ELF::GRP_COMDAT;
    memcpy(buffer, &comdatFlag, sh_entsize);
    off += sh_entsize;
    for (const auto &sec : sections) {
      const uint32_t secNdx = sec->getIdxValue();
      memcpy(buffer + off, &secNdx, sh_entsize);
      off += sh_entsize;
    }
  }

  void dumpData(std::ostream &oss) const override {
    for (const auto &sec : sections) {
      oss << "section: " << sec->getIdxValue() << " " << sec->getNameValue()
          << "\n";
    }
  }

  std::string dataToString() const override {
    std::stringstream oss;
    dumpData(oss);
    return oss.str();
  }

  const auto &getSections() { return sections; }

  void push_back(const std::shared_ptr<Section> &section) {
    sections.push_back(section);
  }
};

class EhFrameSection final : public Section {
private:
  std::vector<std::shared_ptr<CFI>> cfis;

  // Return false means termination.
  bool loadEntry(uint64_t &offset, uint32_t &length, uint64_t &extLength,
                 uint32_t &entryFlag, const char *&otherData) const {
    const auto &logger = Logger::getInstance();

    uint64_t actLength = 0;
    length = 0;
    extLength = 0;
    if (offset + sizeof(uint32_t) > sh_size) {
      logger.fatal("can not load CIE/FDE length.");
    }
    memcpy(&length, data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    if (length == 0) {
      return false;
    }

    if (length == 0xffffffff) {
      if (offset + sizeof(uint64_t) > sh_size) {
        logger.fatal("can not load CIE/FDE ext length.");
      }
      memcpy(&extLength, data + offset, sizeof(uint64_t));
      offset += sizeof(uint64_t);

      actLength = extLength;
    } else {
      actLength = length;
    }

    if (offset + sizeof(uint32_t) > sh_size) {
      logger.fatal("can not load CIE/FDE flag.");
    }
    memcpy(&entryFlag, data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    actLength -= sizeof(uint32_t);

    if (offset + actLength > sh_size) {
      logger.fatal("CIE/FDE data leak.");
    }
    otherData = data + offset;
    offset += actLength;

    return true;
  }

public:
  EhFrameSection(const llvm::object::ELF64LE::Shdr *shdr, const char *_data)
      : Section(SectionType::EhFrame, shdr, _data) {
    const auto &logger = Logger::getInstance();
    // ref:
    // https://refspecs.linuxfoundation.org/LSB_3.0.0/LSB-Core-generic/LSB-Core-generic/ehframechpt.html
    uint64_t offset = 0;
    uint32_t length = 0;
    uint64_t extLength = 0;
    uint32_t entryFlag = 0;
    const char *otherData = nullptr;

    // Load the first CIE.
    if (!loadEntry(offset, length, extLength, entryFlag, otherData)) {
      return;
    }
    if (entryFlag != 0) {
      logger.fatal("the first entry of CFI should be CIE.");
    }
    const auto firstCIE =
        std::make_shared<CIE>(length, extLength, entryFlag, otherData);
    auto curCFI = std::make_shared<CFI>(firstCIE);
    cfis.push_back(curCFI);

    while (offset < sh_size) {
      if (!loadEntry(offset, length, extLength, entryFlag, otherData)) {
        return;
      }
      if (entryFlag == 0) {
        // Load CIE.
        const auto cie =
            std::make_shared<CIE>(length, extLength, entryFlag, otherData);
        curCFI = std::make_shared<CFI>(cie);
        cfis.push_back(curCFI);
      } else {
        // Load FDE.
        const auto fde =
            std::make_shared<FDE>(length, extLength, entryFlag, otherData);
        curCFI->addFDE(fde);
      }
    }
  }

  void
  parseReferences(const std::shared_ptr<RelocationSection> &relaEhFrame) const {
    // r_offest -> rela entry.
    const auto &relaEntries = relaEhFrame->getRelocations();
    using RelaPair = std::pair<uint64_t, std::shared_ptr<Relocation>>;
    std::vector<RelaPair> rOffsetToRela;
    for (const auto &relaEntry : relaEntries) {
      rOffsetToRela.emplace_back(relaEntry->getROffset(), relaEntry);
    }

    std::sort(rOffsetToRela.begin(), rOffsetToRela.end(),
      [](const RelaPair& p1, const RelaPair& p2) {
           return p1.first < p2.first;
       });
    size_t rOffsetRelaIdx = 0;

    // Link rela to CIE/FDE.
    uint64_t offset = 0;
    for (const auto &cfi : cfis) {
      const auto cie = cfi->getCIE();

      while (rOffsetRelaIdx < rOffsetToRela.size() &&
          offset <= rOffsetToRela[rOffsetRelaIdx].first &&
          rOffsetToRela[rOffsetRelaIdx].first < offset + cie->getSize()) {
        cie->addRelaEntry(rOffsetToRela[rOffsetRelaIdx].second);
        rOffsetRelaIdx += 1;
      }

      offset += cie->getSize();

      const auto &fdes = cfi->getFDEs();
      for (const auto &fde : fdes) {

        while (rOffsetRelaIdx < rOffsetToRela.size() &&
               offset <= rOffsetToRela[rOffsetRelaIdx].first &&
               rOffsetToRela[rOffsetRelaIdx].first < offset + fde->getSize()) {
          if (rOffsetToRela[rOffsetRelaIdx].first == offset + fde->getPCBeginPreFilling()) {
            // Handle PCBegin rela.
            fde->setPCBeginRelaEntry(rOffsetToRela[rOffsetRelaIdx].second);
          } else {
            // Handle other rela.
            fde->addOtherRelaEntry(rOffsetToRela[rOffsetRelaIdx].second);
          }
          rOffsetRelaIdx += 1;
        }

        offset += fde->getSize();
      }
    }

    // llvm::errs() << this->dataToString() << "\n";

    // Adjust r_offset relative to CIE/FDE.
    offset = 0;
    for (const auto &cfi : cfis) {
      const auto cie = cfi->getCIE();

      const auto &cieRelaEntries = cie->getRelaEntries();
      for (const auto &relaEntry : cieRelaEntries) {
        relaEntry->getOffset()->setValue(relaEntry->getROffset()-offset);
      }

      offset += cie->getSize();

      const auto &fdes = cfi->getFDEs();
      for (const auto &fde : fdes) {

        const auto pcBeginRelaEntry = fde->getPCBeginRelaEntry();
        if (pcBeginRelaEntry != nullptr) {
          pcBeginRelaEntry->getOffset()->setValue(
            pcBeginRelaEntry->getROffset()-offset);
        }

        const auto &fdeRelaEntries = fde->getOtherRelaEntries();
        for (const auto &relaEntry : fdeRelaEntries) {
          relaEntry->getOffset()->setValue(relaEntry->getROffset()-offset);
        }

        offset += fde->getSize();
      }
    }

    // llvm::errs() << this->dataToString() << "\n";
  }

  void layout() override {
    sh_size = 0;
    uint64_t offset = 0;

    for (const auto &cfi : cfis) {
      const auto cie = cfi->getCIE();

      // Update rela.r_offset.
      // Note that this update should be completed before
      // RelocationSection::fini.
      const auto &cieRelaEntries = cie->getRelaEntries();
      for (const auto &relaEntry : cieRelaEntries) {
        const uint64_t newOffset = relaEntry->getOffsetValue() + offset;
        relaEntry->getOffset()->setValue(newOffset);
      }

      const uint64_t cieBaseOffset = offset;
      offset += cie->getSize();

      const auto &fdes = cfi->getFDEs();
      for (const auto &fde : fdes) {

        // Update CIE pointer.
        fde->setCIEPointer(offset - cieBaseOffset +
          fde->getCIEPointerPreFilling());

        // Update rela.r_offset.
        // Note that this update should be completed before
        // RelocationSection::fini.
        const auto pcBeginRelaEntry = fde->getPCBeginRelaEntry();
        if (pcBeginRelaEntry != nullptr) {
          const uint64_t newOffset = pcBeginRelaEntry->getOffsetValue() + offset;
          pcBeginRelaEntry->getOffset()->setValue(newOffset);
        }

        const auto &fdeRelaEntries = fde->getOtherRelaEntries();
        for (const auto &relaEntry : fdeRelaEntries) {
          const uint64_t newOffset = relaEntry->getOffsetValue() + offset;
          relaEntry->getOffset()->setValue(newOffset);
        }

        offset += fde->getSize();
      }
    }

    sh_size = offset;
  }

  void writeDataTo(char *buffer) override {
    uint64_t offset = 0;
    for (const auto &cfi : cfis) {
      const uint64_t inc = cfi->writeDataTo(buffer + offset);
      offset += inc;
    }
  }

  void dumpData(std::ostream &oss) const override {
    for (size_t i = 0; i < cfis.size(); i++) {
      auto &cfi = cfis[i];
      oss << "CFI " << i << "==========\n";
      oss << cfi->toString();
    }
  }

  std::string dataToString() const override {
    std::stringstream oss;
    dumpData(oss);
    return oss.str();
  }

  void addCFI(const std::shared_ptr<CFI> &newCFI) { cfis.push_back(newCFI); }

  auto &getCFIs() const { return cfis; }
};

class DebugAbbrevSection final : public Section {
public:
  // Structure representing an attribute-form pair in an abbreviation declaration
  struct AttributeForm {
    uint64_t attr;                  // DWARF attribute code
    uint64_t form;                  // DWARF form code
    std::optional<int64_t> implicitConst; // Optional implicit constant value
  };

  // Structure representing an abbreviation declaration
  struct AbbreviationDecl {
    uint64_t code;                  // Abbreviation code
    uint64_t tag;                   // DWARF tag
    bool hasChildren;               // Whether this DIE has children
    std::vector<AttributeForm> attrForms; // List of attribute-form pairs
  };

  // Map of abbreviation tables (keyed by offset)
  std::map<uint64_t, std::map<uint64_t, AbbreviationDecl>> abbrevTables;

  DebugAbbrevSection(const llvm::object::ELF64LE::Shdr *shdr, const char *_data)
    : Section(SectionType::DebugAbbrev, shdr, _data) {
    // Ref: 7.5.3
    const uint8_t *start = reinterpret_cast<const uint8_t *>(data);
    const uint8_t *end = start + sh_size;
    const uint8_t *p = start;

    while (p < end) {
      uint64_t abbrevOffset = p - start;
      std::map<uint64_t, AbbreviationDecl> declMap;

      while (p < end) {
        unsigned bytesRead;
        uint64_t code = decodeULEB128(p, &bytesRead, end);
        p += bytesRead;
        if (code == 0)
          break;

        uint64_t tag = decodeULEB128(p, &bytesRead, end);
        p += bytesRead;
        uint8_t hasChildrenByte = *p++;

        AbbreviationDecl decl;
        decl.code = code;
        decl.tag = tag;
        decl.hasChildren = (hasChildrenByte == 1); // DW_CHILDREN_yes

        while (p < end) {
          uint64_t attr = decodeULEB128(p, &bytesRead, end);
          p += bytesRead;
          uint64_t form = decodeULEB128(p, &bytesRead, end);
          p += bytesRead;
          if (attr == 0 && form == 0)
            break;

          AttributeForm af = {attr, form};
          if (form == 0x21 /* DW_FORM_implicit_const */) {
            af.implicitConst = decodeSLEB128(p, &bytesRead, end);
            p += bytesRead;
          }

          decl.attrForms.push_back(af);
        }

        declMap[code] = std::move(decl);
      }

      if (!declMap.empty())
        abbrevTables[abbrevOffset] = std::move(declMap);
    }
  }

  void dumpData(std::ostream &oss) const override {
    oss << ".debug_abbrev contents:\n";
    for (const auto &[offset, decls] : abbrevTables) {
      oss << "Abbrev table for offset: 0x" << std::setw(8) << std::setfill('0')
          << std::hex << offset << "\n";
      for (const auto &[code, decl] : decls) {
        oss << std::dec << code << ". "
            << getTagName(decl.tag) << "\t"
            << (decl.hasChildren ? "DW_CHILDREN_yes" : "DW_CHILDREN_no") << "\n";

        for (const auto &af : decl.attrForms) {
          oss << "\t" << getAttrName(af.attr) << "\t" << getFormName(af.form);
          if (af.form == 0x21 && af.implicitConst.has_value()) {
            oss << " " << af.implicitConst.value();
          }
          oss << "\n";
        }
        oss << "\n";
      }
    }
  }

  void writeDataTo(char *buffer) override {
    std::vector<uint8_t> out;

    for (const auto &[offset, decls] : abbrevTables) {
      (void)offset;

      for (const auto &[code, decl] : decls) {
        encodeULEB128(code, out);
        encodeULEB128(decl.tag, out);
        out.push_back(decl.hasChildren ? 1 : 0);

        for (const auto &af : decl.attrForms) {
          encodeULEB128(af.attr, out);
          encodeULEB128(af.form, out);
          if (af.form == 0x21 && af.implicitConst.has_value()) {
            encodeSLEB128(af.implicitConst.value(), out);
          }
        }

        // Write attribute-form terminator (0, 0)
        encodeULEB128(0, out);
        encodeULEB128(0, out);
      }
      encodeULEB128(0, out);
    }

    // Copy to target buffer
    assert(out.size() <= sh_size && "Rewritten .debug_abbrev exceeds original section size");
    memcpy(buffer, out.data(), out.size());
  }

  // Get abbreviation declaration by offset and code
  const AbbreviationDecl* getAbbreviationDecl(uint64_t abbrevOffset, uint64_t code) const {
    auto abbrevTableIt = abbrevTables.find(abbrevOffset);
    if (abbrevTableIt == abbrevTables.end()) return nullptr;

    const auto& decls = abbrevTableIt->second;
    auto declIt = decls.find(code);
    if (declIt == decls.end()) return nullptr;

    return &declIt->second;
  }
};

class DebugStrSection final : public Section {
private:
  // Structure representing a string entry in .debug_str section
  struct StringEntry {
    uint64_t offset;    // Offset within the section
    std::string str;    // The string content
  };

  std::vector<StringEntry> strings; // List of string entries

public:
  // TODO fix: string overlap, according to debug str offset.
  DebugStrSection(const llvm::object::ELF64LE::Shdr *shdr, const char *_data)
      : Section(SectionType::DebugStr, shdr, _data) {
    const char *start = data;
    const char *end = data + sh_size;
    uint64_t offset = 0;

    while (start < end) {
      const char *s = start;
      size_t len = strlen(s);
      if (len == 0) {
        ++start;
        ++offset;
        continue;
      }
      strings.push_back({offset, std::string(s)});
      start += len + 1;
      offset += len + 1;
    }
  }

  void writeDataTo(char *buffer) override {
    std::vector<uint8_t> out;

    for (const auto &entry : strings) {
      for (char c : entry.str) {
        out.push_back(static_cast<uint8_t>(c));
      }
      out.push_back(0);
    }

    assert(out.size() <= sh_size && "Rewritten .debug_str larger than original");
    memcpy(buffer, out.data(), out.size());
  }

  void dumpData(std::ostream &oss) const override {
    oss << ".debug_str contents:\n";
    for (const auto &entry : strings) {
      oss << "0x" << intToHex(entry.offset, 8) << ": \"" << entry.str << "\"\n";
    }
  }

  // Get string by offset
  std::string getString(uint32_t offset) const {
    if (offset >= sh_size) return "<invalid offset>";
    return std::string(data + offset);
  }

};

class DebugLineStrSection final : public Section {
private:
  // Structure representing a string entry in .debug_str section
  struct StringEntry {
    uint64_t offset;    // Offset within the section
    std::string str;    // The string content
  };

  std::vector<StringEntry> strings; // List of string entries
public:
  DebugLineStrSection(const llvm::object::ELF64LE::Shdr *shdr, const char *_data)
      : Section(SectionType::DebugLineStr, shdr, _data) {
    const char *start = data;
    const char *end = data + sh_size;
    uint64_t offset = 0;

    while (start < end) {
      const char *s = start;
      size_t len = strlen(s);
      if (len == 0) {
        ++start;
        ++offset;
        continue;
      }
      strings.push_back({offset, std::string(s)});
      start += len + 1;
      offset += len + 1;
    }
  }

  void writeDataTo(char *buffer) override {
    std::vector<uint8_t> out;

    for (const auto &entry : strings) {
      for (char c : entry.str) {
        out.push_back(static_cast<uint8_t>(c));
      }
      out.push_back(0);
    }

    assert(out.size() <= sh_size && "Rewritten .debug_str larger than original");
    memcpy(buffer, out.data(), out.size());
  }

  void dumpData(std::ostream &oss) const override {
    oss << ".debug_line_str contents:\n";
    for (const auto &entry : strings) {
      oss << "0x" << intToHex(entry.offset, 8) << ": \"" << entry.str << "\"\n";
    }
  }

  // Get string by offset
  std::string getString(uint32_t offset) const {
    if (offset >= sh_size) return "<invalid offset>";
    return std::string(data + offset);
  }

};

class DebugAddrSection final : public Section {
private:
  // TODO: block struct -> rela. Note: debug_info -> block ref.
  // Structure representing an address table
  struct AddrTable {
    uint32_t unitLength;
    uint16_t version;      // DWARF version
    uint8_t addrSize;      // Address size in bytes
    uint8_t segSize;      // Segment selector size
  };

  std::vector<AddrTable> tables; // List of address tables
  std::vector<uint64_t> offsetBases;                  // 每个表的 offsetBase
  std::vector<uint64_t> sizes;                        // 每个表的 size（unitLength + 4）
  std::vector<uint64_t> headerSizes;                  // 每个表头部长度
  std::vector<std::vector<uint64_t>> allAddresses;    // 每个表的地址列表

public:

  DebugAddrSection(const llvm::object::ELF64LE::Shdr *shdr, const char *_data)
      : Section(SectionType::DebugAddr, shdr, _data) {
    // Ref: 7.27
    const uint8_t *ptr = reinterpret_cast<const uint8_t *>(data);
    const uint8_t *end = ptr + shdr->sh_size;

    while (ptr < end) {
      const uint8_t *tableStart = ptr;
      if (end - ptr < 8)
        break; // Invalid header

      const uint32_t unitLength = llvm::support::endian::read32le(ptr);
      ptr += 4;

      const uint8_t *tableEnd = ptr + unitLength;
      if (tableEnd > end) break;
      if (tableEnd - ptr < 4) break;

      AddrTable table;
      table.unitLength = unitLength;
      table.version = llvm::support::endian::read16le(ptr);
      ptr += 2;

      table.addrSize = *ptr++;
      table.segSize = *ptr++;
      tables.push_back(table);

      offsetBases.push_back(tableStart - reinterpret_cast<const uint8_t *>(data));
      sizes.push_back(unitLength + 4);
      headerSizes.push_back(4 + 2 + 1 + 1);

      std::vector<uint64_t> addresses;
      while (ptr + table.addrSize <= tableEnd) {
        uint64_t addr = 0;
        memcpy(&addr, ptr, table.addrSize);
        ptr += table.addrSize;
        addresses.push_back(addr);
      }
      allAddresses.push_back(std::move(addresses));
    }
  }

  // Apply relocations to address entries
  void applyRelocations(const std::vector<std::shared_ptr<Relocation>> &relocs) {
    for (const auto &rel : relocs) {
      uint64_t r_offset = rel->getROffset();

      for (size_t i = 0; i < tables.size(); ++i) {
        uint64_t tableStart = offsetBases[i] + headerSizes[i];
        uint64_t tableEnd = offsetBases[i] + sizes[i];

        if (r_offset < tableStart || r_offset >= tableEnd)
          continue;

        uint64_t offsetInTable = r_offset - tableStart;
        if (offsetInTable % tables[i].addrSize != 0)
          continue;

        size_t entryIndex = offsetInTable / tables[i].addrSize;
        if (entryIndex >= allAddresses[i].size())
          continue;

        allAddresses[i][entryIndex] = static_cast<uint64_t>(rel->getRAddend());
      }
    }
  }

  // Get address by table base offset and index
  uint64_t getAddressByIndex(uint64_t baseOffset, uint64_t index) const {
    for (size_t i = 0; i < tables.size(); ++i) {
      if (offsetBases[i] == baseOffset) {
        if (index < allAddresses[i].size())
          return allAddresses[i][index];
      }
    }
    return -1;
  }

  void writeDataTo(char *buffer) override {
    std::vector<uint8_t> out;

    for (size_t i = 0; i < tables.size(); ++i) {
      const auto &table = tables[i];
      const auto &addresses = allAddresses[i];
      size_t startOffset = out.size();

      // 1. Reserve space for unit_length (4 bytes)
      out.resize(out.size() + 4);

      // 2. Write header: version (2 bytes), addr_size (1), seg_size (1)
      out.push_back(table.version & 0xff);
      out.push_back((table.version >> 8) & 0xff);
      out.push_back(table.addrSize);
      out.push_back(table.segSize);

      // 3. Write address entries
      for (uint64_t addr : addresses) {
        if (table.addrSize == 4) {
          out.push_back(addr & 0xff);
          out.push_back((addr >> 8) & 0xff);
          out.push_back((addr >> 16) & 0xff);
          out.push_back((addr >> 24) & 0xff);
        } else if (table.addrSize == 8) {
          for (int i = 0; i < 8; ++i)
            out.push_back((addr >> (i * 8)) & 0xff);
        } else {
          assert(false && "Unsupported addr_size");
        }
      }

      // 4. Fill in unit_length (total size minus length field)
      uint32_t length = static_cast<uint32_t>(out.size() - startOffset - 4);
      out[startOffset + 0] = (length & 0xff);
      out[startOffset + 1] = (length >> 8) & 0xff;
      out[startOffset + 2] = (length >> 16) & 0xff;
      out[startOffset + 3] = (length >> 24) & 0xff;
    }

    // 5. Copy to target buffer
    assert(out.size() <= sh_size && "Rewritten .debug_addr larger than original");
    memcpy(buffer, out.data(), out.size());
  }

  void dumpData(std::ostream &oss) const override {
    for (size_t i = 0; i < tables.size(); ++i) {
      const auto &table = tables[i];
      const auto &addresses = allAddresses[i];
      oss << ".debug_addr contents:\n";

      oss << "Address table header: length = 0x"
          << std::hex << std::setw(8) << std::setfill('0') << table.unitLength
          << ", format = DWARF32, version = 0x"
          << std::setw(4) << table.version
          << ", addr_size = 0x"
          << std::setw(2) << static_cast<int>(table.addrSize)
          << ", seg_size = 0x"
          << std::setw(2) << static_cast<int>(table.segSize)
          << std::dec << "\n";

      oss << "Addrs: [\n";
      for (uint64_t addr : addresses) {
        if (table.addrSize == 4) {
          oss << "0x" << std::hex << std::setw(8) << std::setfill('0')
              << static_cast<uint32_t>(addr) << std::dec << "\n";
        } else if (table.addrSize == 8) {
          oss << "0x" << std::hex << std::setw(16) << std::setfill('0')
              << addr << std::dec << "\n";
        } else {
          oss << "# unsupported addr_size: " << static_cast<int>(table.addrSize) << "\n";
        }
      }
      oss << "]\n";
    }
  }
};

class DebugStrOffsetsSection final : public Section {
private:
  const DebugStrSection &debugStr;

  // TODO: block struct -> rela. Note: debug_info -> block ref.
  // Structure representing a string offsets table
  struct StringOffsetsTable {
    uint32_t unitLength;
    uint16_t version;
    uint16_t padding; // always 0
  };

  std::vector<StringOffsetsTable> tables;
  std::vector<uint64_t> offsetBases;
  std::vector<uint64_t> sizes;
  std::vector<uint64_t> headerSizes;
  std::vector<std::vector<uint32_t>> allOffsets;


public:
  DebugStrOffsetsSection(const llvm::object::ELF64LE::Shdr *shdr,
                         const char *_data, const DebugStrSection &strRef)
      : Section(SectionType::DebugStrOffsets, shdr, _data), debugStr(strRef) {
    // Ref 7.26
    const uint8_t *ptr = reinterpret_cast<const uint8_t *>(data);
    const uint8_t *end = ptr + sh_size;

    while (ptr < end) {
      const uint8_t *tableStart = ptr;
      if (end - ptr < 4)
        break;

      uint32_t unitLength = llvm::support::endian::read32le(ptr);
      ptr += 4;

      const uint8_t *tableEnd = ptr + unitLength;
      if (tableEnd > end)
        break;

      if (tableEnd - ptr < 4)
        break;

      uint16_t version = llvm::support::endian::read16le(ptr);
      ptr += 2;
      uint16_t padding = llvm::support::endian::read16le(ptr);
      ptr += 2;

      StringOffsetsTable table;
      table.unitLength = unitLength;
      table.version = version;
      table.padding = padding;
      tables.push_back(table);

      offsetBases.push_back(tableStart - reinterpret_cast<const uint8_t *>(data));
      sizes.push_back(unitLength + 4);
      headerSizes.push_back(8); // 4 (length) + 2 (version) + 2 (padding)

      std::vector<uint32_t> offsets;
      while (ptr + 4 <= tableEnd) {
        uint32_t offset = llvm::support::endian::read32le(ptr);
        offsets.push_back(offset);
        ptr += 4;
      }
      allOffsets.push_back(offsets);
    }
  }

  // Apply relocations to string offsets
  void
  applyRelocations(const std::vector<std::shared_ptr<Relocation>> &relocs) {
    for (const auto &rel : relocs) {
      uint64_t r_offset = rel->getROffset();

      for (size_t i = 0; i < tables.size(); ++i) {
        uint64_t tableStart = offsetBases[i] + headerSizes[i];
        uint64_t tableEnd = offsetBases[i] + sizes[i];

        if (r_offset < tableStart || r_offset >= tableEnd)
          continue;

        uint64_t offsetInTable = r_offset - tableStart;
        if (offsetInTable % 4 != 0) continue;

        size_t entryIndex = offsetInTable / 4;
        if (entryIndex >= allOffsets[i].size()) continue;

        allOffsets[i][entryIndex] = static_cast<uint32_t>(rel->getRAddend());
      }
    }
  }

  void writeDataTo(char *buffer) override {
    std::vector<uint8_t> out;

    for (size_t i = 0; i < tables.size(); ++i) {
      const auto &table = tables[i];
      const auto &offsets = allOffsets[i];
      size_t startOffset = out.size();
      // 1. Reserve space for unit_length (4 bytes)
      out.resize(out.size() + 4);

      // 2. Write header: version (2 bytes) + padding (2 bytes)
      out.push_back(table.version & 0xff);
      out.push_back((table.version >> 8) & 0xff);
      out.push_back(0); // padding byte 1
      out.push_back(0); // padding byte 2

      // 3. Write offsets, each is 4 bytes little endian
      for (uint32_t offset : offsets) {
        out.push_back(offset & 0xff);
        out.push_back((offset >> 8) & 0xff);
        out.push_back((offset >> 16) & 0xff);
        out.push_back((offset >> 24) & 0xff);
      }

      // 4. Fill in unit_length = total size minus length field
      uint32_t length = static_cast<uint32_t>(out.size() - startOffset - 4);
      out[startOffset + 0] = (length & 0xff);
      out[startOffset + 1] = (length >> 8) & 0xff;
      out[startOffset + 2] = (length >> 16) & 0xff;
      out[startOffset + 3] = (length >> 24) & 0xff;
    }

    // 5. Copy to target buffer
    assert(out.size() <= sh_size &&
           "Rewritten .debug_str_offset larger than original");
    memcpy(buffer, out.data(), out.size());
  }

  void dumpData(std::ostream &oss) const override {
    oss << ".debug_str_offsets contents:\n";
    for (size_t i = 0; i < tables.size(); ++i) {
      const auto &table = tables[i];
      const auto &offsets = allOffsets[i];

      oss << "0x" << std::hex << std::setw(8) << std::setfill('0')
          << offsetBases[i] << ": Contribution size = " << std::dec
          << sizes[i] << ", Format = DWARF32"
          << ", Version = " << table.version << "\n";

      for (size_t j = 0; j < offsets.size(); ++j) {
        uint64_t absOffset = offsetBases[i] + headerSizes[i] + j * 4;
        uint32_t strOffset = offsets[j];
        std::string str = debugStr.getString(strOffset);

        oss << "0x" << std::hex << std::setw(8) << std::setfill('0')
            << absOffset << ": " << std::setw(8) << std::setfill('0')
            << strOffset << " \"" << str << "\"\n";
      }
    }
  }

  // Get table index by base offset
  int getTableIndex(uint64_t baseOffset) const {
    for (size_t i = 0; i < offsetBases.size(); ++i) {
      if (offsetBases[i] == baseOffset)
        return static_cast<int>(i);
    }
    return -1;
  }

  // Get string offset by table index and string index
  uint32_t getStringOffset(int tableIndex, uint32_t strxIndex) const {
    if (tableIndex < 0 || static_cast<size_t>(tableIndex) >= tables.size())
      return 0;

    if (strxIndex >= allOffsets[tableIndex].size())
      return 0;

    return allOffsets[tableIndex][strxIndex];
  }

  // Get string by table index and string index
  std::string getStringFromStrx(int tableIndex, uint32_t strxIndex) const {
    uint32_t offset = getStringOffset(tableIndex, strxIndex);
    return debugStr.getString(offset);
  }
};

class DebugRnglistSection final : public Section {
private:
  // Ref : 7.28
  struct RnglistEntry {
    uint8_t kind;
    uint64_t value0;
    uint64_t value1;
  };

  struct RnglistHeader {
    uint32_t unit_length;
    uint16_t version;
    uint8_t addr_size;
    uint8_t seg_size;
    uint32_t offset_entry_count;
  };

  RnglistHeader header{};
  std::vector<uint32_t> offsets;
  std::map<uint32_t, std::vector<RnglistEntry>> rangeLists;
//  const DebugAddrSection &debugAddr;
  const DebugAddrSection *debugAddr;
  uint64_t baseOffset;
  mutable std::unordered_map<uint32_t, uint64_t> contextMap;
public:
  DebugRnglistSection(const llvm::object::ELF64LE::Shdr *shdr, const char *_data, const DebugAddrSection *addrRef)
      : Section(SectionType::DebugRnglists, shdr, _data), debugAddr(addrRef) {
    const uint8_t *p = reinterpret_cast<const uint8_t *>(data);
    const uint8_t *end = p + sh_size;

    header.unit_length = llvm::support::endian::read32le(p);
    p += 4;

    header.version = llvm::support::endian::read16le(p);
    p += 2;
    header.addr_size = *p++;
    header.seg_size = *p++;
    header.offset_entry_count = llvm::support::endian::read32le(p);
    p += 4;

    for (uint32_t i = 0; i < header.offset_entry_count; ++i) {
      uint32_t offset = llvm::support::endian::read32le(p);
      offsets.push_back(offset);
      p += 4;
    }

    uint32_t current_index = 0;

    while (p < end) {
      uint8_t kind = *p++;
      if (kind == 0x00) {
        rangeLists[current_index].push_back({kind, 0, 0});
        current_index++;
        continue;
      }

      RnglistEntry entry{kind, 0, 0};

      unsigned n;
      switch (kind) {
      case 0x01: // DW_RLE_base_addressx
        entry.value0 = decodeULEB128(p, &n);
        p += n;
        break;

      case 0x02: // DW_RLE_startx_endx
        entry.value0 = decodeULEB128(p, &n);
        p += n;
        entry.value1 = decodeULEB128(p, &n);
        p += n;
        break;

      case 0x03: // DW_RLE_startx_length
        entry.value0 = decodeULEB128(p, &n);
        p += n;
        entry.value1 = decodeULEB128(p, &n);
        p += n;
        break;

      case 0x04: // DW_RLE_offset_pair
        entry.value0 = decodeULEB128(p, &n);
        p += n;
        entry.value1 = decodeULEB128(p, &n);
        p += n;
        break;

      case 0x05: // DW_RLE_base_address
        // addr_size bytes
        entry.value0 = llvm::support::endian::read64le(p); // assume addr_size = 8
        baseOffset = entry.value0;
        p += header.addr_size;
        break;

      case 0x06: // DW_RLE_start_end
        entry.value0 = llvm::support::endian::read64le(p); // assume addr_size = 8
        p += header.addr_size;
        entry.value1 = llvm::support::endian::read64le(p);
        p += header.addr_size;
        break;

      case 0x07: // DW_RLE_start_length
        entry.value0 = llvm::support::endian::read64le(p); // address
        p += header.addr_size;
        entry.value1 = decodeULEB128(p, &n);               // length
        p += n;
        break;

      default:
        std::cerr << "Unknown rnglist kind: 0x" << std::hex << (int)kind << "\n";
        return;
        }
        rangeLists[current_index].push_back(entry);
    }
  }

  void dumpData(std::ostream &oss) const override {
    oss << ".debug_rnglists contents:\n";
    oss << "range list header: "
        << "length = 0x" << std::hex << std::setw(8) << std::setfill('0') << header.unit_length
        << ", format = DWARF32"
        << ", version = 0x" << std::setw(4) << header.version
        << ", addr_size = 0x" << std::setw(2) << static_cast<int>(header.addr_size)
        << ", seg_size = 0x" << std::setw(2) << static_cast<int>(header.seg_size)
        << ", offset_entry_count = 0x" << std::setw(8) << header.offset_entry_count << "\n";

    if (!offsets.empty()){
      oss << "offsets: [\n";
      for (auto offset : offsets) {
        oss << "0x" << std::hex << std::setw(8) << std::setfill('0') << offset << "\n";
      }
      oss << "]\n";
    }

    oss << "ranges:\n";
    for (auto &rangeList : rangeLists) {
      uint64_t cuBaseOffset = 0;

      auto it = contextMap.find(rangeList.first);
      if (it != contextMap.end()) {
        cuBaseOffset = it->second;
      }
      for (const auto &entry : rangeList.second) {
        if (entry.kind == 0x00) {
          oss << "<End of list>\n";
          break;
        }
        switch (entry.kind) {
        case 0x01: {
          assert(debugAddr != nullptr);
          uint64_t baseAddr =
              debugAddr->getAddressByIndex(cuBaseOffset, entry.value0);
          oss << "[0x" << std::hex << std::setw(16) << std::setfill('0')
              << baseAddr << ")\n";
          break;
        }
        case 0x02: {
          assert(debugAddr != nullptr);
          uint64_t startAddr =
              debugAddr->getAddressByIndex(cuBaseOffset, entry.value0);
          uint64_t endAddr =
              debugAddr->getAddressByIndex(cuBaseOffset, entry.value1);
          oss << "[0x" << std::hex << std::setw(16) << std::setfill('0')
              << startAddr << ", 0x" << std::setw(16) << endAddr << ")\n";
          break;
        }
        case 0x03: {
          assert(debugAddr != nullptr);
          uint64_t startAddr =
              debugAddr->getAddressByIndex(cuBaseOffset, entry.value0);
          oss << "[0x" << std::hex << std::setw(16) << std::setfill('0')
              << startAddr << ", 0x" << std::setw(16) << entry.value1 << ")\n";
          break;
        }
        case 0x04:
        case 0x06:
        case 0x07: {
          oss << "[0x" << std::hex << std::setw(16) << std::setfill('0')
              << baseOffset + entry.value0 << ", 0x" << std::setw(16) << baseOffset + entry.value1
              << ")\n";
          break;
        }
//        case 0x05: {
//          oss << "[0x" << std::hex << std::setw(16) << std::setfill('0')
//              << entry.value0 << ")\n";
//          break;
//        }
        }
      }
    }
  }

  void registerAddrBase(uint32_t index, uint64_t addrBaseOffset) const {
    contextMap[index] = addrBaseOffset;
  }

  void writeDataTo(char *buffer) override {
    std::vector<uint8_t> out;

    // unit_length 占位（DWARF32）
    size_t unit_len_pos = out.size();
    out.resize(out.size() + 4, 0);

    // header: version, addr_size, seg_size, offset_entry_count
    uint8_t b2[2];
    llvm::support::endian::write16le(b2, header.version);
    out.insert(out.end(), b2, b2 + 2);

    out.push_back(static_cast<uint8_t>(header.addr_size));
    out.push_back(static_cast<uint8_t>(header.seg_size));

    uint8_t b4[4];
    llvm::support::endian::write32le(b4, header.offset_entry_count);
    out.insert(out.end(), b4, b4 + 4);

    // offsets 表
    for (uint32_t off : offsets) {
      llvm::support::endian::write32le(b4, off);
      out.insert(out.end(), b4, b4 + 4);
    }

    // entries（按解析结果逐条写回）
    for (auto &rangeList : rangeLists) {
      for (const auto &e : rangeList.second) {
        out.push_back(static_cast<uint8_t>(e.kind));

        if (e.kind == 0x00)  // DW_RLE_end_of_list
          break;

        switch (e.kind) {
        case 0x01:  // DW_RLE_base_addressx: index (ULEB)
          encodeULEB128(e.value0, out);
          break;

        case 0x02:  // DW_RLE_startx_endx: start_index(ULEB), end_index(ULEB)
          encodeULEB128(e.value0, out);
          encodeULEB128(e.value1, out);
          break;

        case 0x03:  // DW_RLE_startx_length: start_index(ULEB), length(ULEB)
          encodeULEB128(e.value0, out);
          encodeULEB128(e.value1, out);
          break;

        case 0x04:  // DW_RLE_offset_pair: start_off(ULEB), end_off(ULEB)
          encodeULEB128(e.value0, out);
          encodeULEB128(e.value1, out);
          break;

        case 0x05: { // DW_RLE_base_address: base_addr (addr_size bytes)
          uint64_t v = e.value0;
          for (uint8_t i = 0; i < header.addr_size; ++i)
            out.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFF));
          break;
        }

        case 0x06: { // DW_RLE_start_end: start_addr(addr_size), end_addr(addr_size)
          uint64_t a0 = e.value0, a1 = e.value1;
          for (uint8_t i = 0; i < header.addr_size; ++i)
            out.push_back(static_cast<uint8_t>((a0 >> (i * 8)) & 0xFF));
          for (uint8_t i = 0; i < header.addr_size; ++i)
            out.push_back(static_cast<uint8_t>((a1 >> (i * 8)) & 0xFF));
          break;
        }

        case 0x07: { // DW_RLE_start_length: start_addr(addr_size), length(ULEB)
          uint64_t a = e.value0;
          for (uint8_t i = 0; i < header.addr_size; ++i)
            out.push_back(static_cast<uint8_t>((a >> (i * 8)) & 0xFF));
          encodeULEB128(e.value1, out);
          break;
        }

        default:
          std::cerr << "Unknown rnglist kind when writing: 0x"
                    << std::hex << static_cast<int>(e.kind) << "\n";
          return;
        }
      }
    }


    // 回填 unit_length（不含自身 4 字节）
    llvm::support::endian::write32le(out.data() + unit_len_pos,
                                     static_cast<uint32_t>(out.size() - 4));

    assert(out.size() <= sh_size &&
           "Rewritten .debug_str_offset larger than original");
    memcpy(buffer, out.data(), out.size());
  }

};

class DebugLineSection final : public Section {
private:
  // Ref : 6.2.4
  struct LineTableHeader {
    uint32_t unit_length;
    uint16_t version;
    uint8_t address_size;
    uint8_t segment_selector_size;
    uint32_t header_length;
    uint8_t min_inst_length;
    uint8_t max_ops_per_inst;
    uint8_t default_is_stmt;
    int8_t line_base;
    uint8_t line_range;
    uint8_t opcode_base;
    std::vector<uint8_t> standard_opcode_lengths;
    uint8_t dir_format_count;
    std::vector<std::pair<uint64_t, uint64_t>> dir_attrs;
    uint64_t directory_count;
    std::vector<FormValueRaw> directories;
    uint8_t file_name_entry_format_count;
    std::vector<std::pair<uint64_t, uint64_t>> file_attrs;
    uint64_t file_names_count;
    struct FileEntry {
      std::string name;
      uint64_t dir_index;
      std::array<uint8_t, 16> md5; // For DWARFv5
      FormValueRaw val;
    };
    std::vector<FileEntry> file_names;
  };

  struct Row {
    uint64_t address = 0;
    int32_t line = 1;
    uint32_t column = 0;
    uint32_t file = 1;
    uint32_t isa = 0;
    uint32_t discriminator = 0;
    uint32_t op_index = 0;
    bool is_stmt;
    bool basic_block = false;
    bool end_sequence = false;
    bool prologue_end = false;
    bool epilogue_begin = false;
  };

  LineTableHeader header;
  std::vector<Row> rows;

public:
  DebugLineSection(const llvm::object::ELF64LE::Shdr *shdr, const char *_data)
      : Section(SectionType::DebugLine, shdr, _data) {
    const uint8_t *p = reinterpret_cast<const uint8_t *>(data);

    // header
    header.unit_length = llvm::support::endian::read32le(p);
    p += 4;
    const uint8_t *end = p + header.unit_length;

    header.version = llvm::support::endian::read16le(p);
    p += 2;
    header.address_size = *p++;
    header.segment_selector_size = *p++;
    header.header_length = llvm::support::endian::read32le(p);
    p += 4;

    header.min_inst_length = *p++;
    header.max_ops_per_inst = *p++;
    header.default_is_stmt = *p++;
    header.line_base = *p++;
    header.line_range = *p++;
    header.opcode_base = *p++;

    header.standard_opcode_lengths.resize(header.opcode_base - 1);
    for (uint8_t &len : header.standard_opcode_lengths)
      len = *p++;

    // --- DWARFv5 include_directories_table ---
    unsigned n;
    header.dir_format_count = decodeULEB128(p, &n);
    p += n;
    for (uint64_t i = 0; i < header.dir_format_count; ++i) {
      uint64_t content_type = decodeULEB128(p, &n);
      p += n;
      uint64_t form = decodeULEB128(p, &n);
      p += n;
      header.dir_attrs.push_back(std::make_pair(content_type, form));
    }

    header.directory_count = decodeULEB128(p, &n);
    p += n;
    for (uint64_t i = 0; i < header.directory_count; ++i) {
      FormValueRaw value;
      for (const auto &[type, form] : header.dir_attrs) {
        if (type == 1) { // DW_LNCT_path
          value = parseFormValue(form, p, end);
        }
        else skipFormValue(form, p, end);
      }
      header.directories.push_back(value);
    }

    header.file_name_entry_format_count = decodeULEB128(p, &n);
    p += n;
    for (uint64_t i = 0; i < header.file_name_entry_format_count; ++i) {
      uint64_t content_type = decodeULEB128(p, &n);
      p += n;
      uint64_t form = decodeULEB128(p, &n);
      p += n;
      header.file_attrs.emplace_back(content_type, form);
    }

    header.file_names_count = decodeULEB128(p, &n);
    p += n;
    for (uint64_t i = 0; i < header.file_names_count; ++i) {
      LineTableHeader::FileEntry entry;
      for (const auto &[type, form] : header.file_attrs) {
        if (type == 1) { // DW_LNCT_path
          entry.val = parseFormValue(form, p, end);
          entry.name = entry.val.str;
        } else if (type == 2) { // DW_LNCT_directory_index
          entry.dir_index = parseFormValue(form, p, end).value;
        } else if (type == 5) { // DW_LNCT_md5
          for (int j = 0; j < 16; ++j)
            entry.md5[j] = *p++;
        } else skipFormValue(form, p, end);
      }
      header.file_names.push_back(entry);
    }

    // line table program
    Row state;
    state.is_stmt = header.default_is_stmt;

    while (p < end) {
      uint8_t opcode = *p++;
      if (opcode >= header.opcode_base) {
        uint8_t adj_opcode = opcode - header.opcode_base;
        uint64_t addr_inc = (adj_opcode / header.line_range) * header.min_inst_length;
        int64_t line_inc = header.line_base + (adj_opcode % header.line_range);
        state.address += addr_inc;
        state.line += line_inc;
        rows.push_back(state);
        state.basic_block = false;
        state.epilogue_begin = false;
        state.prologue_end = false;
        state.discriminator = 0;
        continue;
      }
      if (opcode == 0) {
        unsigned n;
        uint64_t len = decodeULEB128(p, &n); p += n;
        const uint8_t *ext_end = p + len;
        uint8_t sub = *p++;
        switch (sub) {
        case 1: // DW_LNE_end_sequence
          state.end_sequence = true;
          rows.push_back(state);
          state = {};
          state.line = 1;
          state.is_stmt = header.default_is_stmt;
          break;
        case 2: // DW_LNE_set_address
          if (header.address_size == 8)
            state.address = llvm::support::endian::read64le(p);
          else state.address = llvm::support::endian::read32le(p);
          p += header.address_size;
          break;
        case 3: // DW_LNE_set_prologue_end
          state.prologue_end = true;
          break;
        case 4: { // DW_LNE_set_discriminator
          uint64_t discrim = decodeULEB128(p, &n);
          p += n;
          state.discriminator = discrim;
          break;
        }
        default: break;
        }
        p = ext_end;
      } else {
        unsigned n;
        switch (opcode) {
        case 1:
          rows.push_back(state);
          state.basic_block = false;
          state.prologue_end = false;
          state.epilogue_begin = false;
          state.discriminator = 0;
          break; // DW_LNS_copy
        case 2: state.address += decodeULEB128(p, &n) * header.min_inst_length; p += n; break;
        case 3: state.line += decodeSLEB128(p, &n); p += n; break;
        case 4: state.file = decodeULEB128(p, &n); p += n; break;
        case 5: state.column = decodeULEB128(p, &n); p += n; break;
        case 6: state.is_stmt = !state.is_stmt; break;
        case 7: state.basic_block = true; break;
        case 8: { // DW_LNS_const_add_pc
          uint8_t adjusted = 255 - header.opcode_base;
          uint64_t addr_inc = (adjusted / header.line_range) * header.min_inst_length;
          state.address += addr_inc;
          break;
        }
        case 9: { // DW_LNS_fixed_advance_pc
          uint16_t advance = llvm::support::endian::read16le(p); p += 2;
          state.address += advance;
          break;
        }
        case 10: // DW_LNS_set_prologue_end
          state.prologue_end = true;
          break;
        case 11: // DW_LNS_set_epilogue_begin
          state.epilogue_begin = true;
          break;
        case 12: // DW_LNS_set_isa
          state.isa = decodeULEB128(p, &n); p += n;
          break;
        default:
          // Handle unknown opcode: skip operands
          if (opcode < header.standard_opcode_lengths.size() + 1) {
            for (uint8_t i = 0; i < header.standard_opcode_lengths[opcode - 1]; ++i)
              decodeULEB128(p, &n), p += n;
          }
          break;
        }
      }
    }
  }

  void dumpData(std::ostream &oss) const override {
    oss << ".debug_line contents:\n";
    oss << "debug_line[0x00000000]\n";
    oss << "Line table prologue:\n";
    oss << "    total_length: 0x" << std::hex << std::setw(8) << std::setfill('0') << header.unit_length << "\n";
    oss << "          format: DWARF32\n";
    oss << "         version: " << std::dec << header.version << "\n";
    oss << "    address_size: " << static_cast<int>(header.address_size) << "\n";
    oss << " seg_select_size: " << static_cast<int>(header.segment_selector_size) << "\n";
    oss << " prologue_length: 0x" << std::hex << std::setw(8) << std::setfill('0') << header.header_length << "\n";
    oss << " min_inst_length: " << std::dec << static_cast<int>(header.min_inst_length) << "\n";
    oss << "max_ops_per_inst: " << static_cast<int>(header.max_ops_per_inst) << "\n";
    oss << " default_is_stmt: " << static_cast<int>(header.default_is_stmt) << "\n";
    oss << "       line_base: " << static_cast<int>(header.line_base) << "\n";
    oss << "      line_range: " << static_cast<int>(header.line_range) << "\n";
    oss << "     opcode_base: " << static_cast<int>(header.opcode_base) << "\n";

    for (size_t i = 0; i < header.standard_opcode_lengths.size(); ++i)
      oss << "standard_opcode_lengths[DW_LNS_" << opcodeName(i+1) << "] = "
          << static_cast<int>(header.standard_opcode_lengths[i]) << "\n";

    for (size_t i = 0; i < header.directories.size(); ++i)
      oss << "include_directories[" << std::setw(3) << i << "] = \"" << std::hex
          << header.directories[i].value << "\"\n";

    for (size_t i = 0; i < header.file_names.size(); ++i) {
      const auto &f = header.file_names[i];
      oss << "file_names[" << std::setw(3) << i << "]:\n";
      oss << "           name: \"" << f.name << "\"\n";
      oss << "      dir_index: " << f.dir_index << "\n";
      if (header.file_names.size() <= 1)
      oss << "   md5_checksum: ";
      for (uint8_t b : f.md5)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
      oss << std::dec << "\n";
    }

    oss << "\nAddress             Line    Column  File    ISA  Discriminator  OpIndex  Flags\n";
    oss << "------------------  ------  ------  ------  ---  -------------  -------  -------------\n";
    for (const auto &row : rows) {
      oss << "0x" << std::hex << std::setw(16) << std::setfill('0') << row.address << "  ";
      oss << std::dec << std::setw(6) << row.line << "  ";
      oss << std::setw(6) << row.column << "  ";
      oss << std::setw(6) << row.file << "  ";
      oss << std::setw(3) << row.isa << "  ";
      oss << std::setw(13) << row.discriminator << "  ";
      oss << std::setw(7) << row.op_index << "  ";
      std::string flags;
      if (row.is_stmt) flags += "is_stmt ";
      if (row.basic_block) flags += "basic_block ";
      if (row.prologue_end) flags += "prologue_end ";
      if (row.end_sequence) flags += "end_sequence ";
      if (row.epilogue_begin) flags += "epilogue_begin ";
      oss << flags << "\n";
    }
  }

  void writeDataTo(char *buffer) override {
    std::vector<uint8_t> out;

    // --- 1. total_length 预留 ---
    size_t totalLengthOffset = out.size();
    out.resize(out.size() + 4); // DWARF32: total_length 占位

    // --- 2. Header (version, address size, etc) ---
    out.push_back(header.version & 0xff);
    out.push_back((header.version >> 8) & 0xff);
    out.push_back(header.address_size);
    out.push_back(header.segment_selector_size);

    size_t prologueLengthOffset = out.size();
    out.resize(out.size() + 4); // prologue_length 占位
    size_t prologueStart = out.size();

    out.push_back(header.min_inst_length);
    out.push_back(header.max_ops_per_inst);
    out.push_back(header.default_is_stmt);
    out.push_back(static_cast<uint8_t>(header.line_base));
    out.push_back(header.line_range);
    out.push_back(header.opcode_base);

    out.insert(out.end(), header.standard_opcode_lengths.begin(), header.standard_opcode_lengths.end());

    // attr_count
    encodeULEB128(header.dir_attrs.size(), out);
    // attr spec
    for (const auto &[content_type, form] : header.dir_attrs) {
      encodeULEB128(content_type, out);
      encodeULEB128(form, out);
    }
    // directory count
    encodeULEB128(header.directories.size(), out);

    // entries
    for (const auto &dir : header.directories) {
      for (const auto &[type, form] : header.dir_attrs) {
        if (type == 1 /* DW_LNCT_path */) {
          // DW_FORM_string
          writeFormValue(dir, out);
        }
      }
    }

    encodeULEB128(header.file_attrs.size(), out);

    // attr spec
    for (const auto &[content_type, form] : header.file_attrs) {
      encodeULEB128(content_type, out);
      encodeULEB128(form, out);
    }

    // file count
    encodeULEB128(header.file_names.size(), out);

    // entries
    for (const auto &f : header.file_names) {
      for (const auto &[type, form] : header.file_attrs) {
        if (type == 1 /* DW_LNCT_path */) {
          // DW_FORM_string
          writeFormValue(f.val, out);
        } else if (type == 2 /* DW_LNCT_directory_index */) {
          encodeULEB128(f.dir_index, out);
        } else if (type == 5 /* DW_LNCT_md5 */) {
          if (f.md5.size() != 16)
            llvm::errs() << "MD5 should be 16 bytes" << "\n";
          out.insert(out.end(), f.md5.begin(), f.md5.end());
        } else {
          llvm::errs() << "Unsupported file_attr type in write" << "\n";
        }
      }
    }

    // --- 5. 回填 prologue_length ---
    uint32_t prologueLength = static_cast<uint32_t>(out.size() - prologueStart);
    out[prologueLengthOffset + 0] = (prologueLength & 0xff);
    out[prologueLengthOffset + 1] = (prologueLength >> 8) & 0xff;
    out[prologueLengthOffset + 2] = (prologueLength >> 16) & 0xff;
    out[prologueLengthOffset + 3] = (prologueLength >> 24) & 0xff;

    // TODO : line number program
    // --- 6. line number program 写入 ---
    Row prev;
    prev.is_stmt = header.default_is_stmt;
    prev.end_sequence = true;
    uint64_t maxSpecialAddrDelta = (255 - header.opcode_base) / header.line_range;
    for (auto row : rows) {
      int64_t addrDelta = row.address - prev.address;
      if (row.end_sequence) {
        if (addrDelta >= 0 && static_cast<uint64_t>(addrDelta) == maxSpecialAddrDelta)
          out.push_back(8);
        else {
          out.push_back(2); // DW_LNS_advance_pc
          encodeULEB128(addrDelta / header.min_inst_length, out);
          prev.address = row.address;
        }
        out.push_back(0);
        encodeULEB128(1, out);
        out.push_back(1);
        prev = {};
        prev.line = 1;
        prev.is_stmt = header.default_is_stmt;
        prev.end_sequence = true;
        continue;
      }
      int64_t lineDelta = row.line - prev.line;

      if (row.file != prev.file) {
        out.push_back(4);
        encodeULEB128(row.file, out);
        prev.file = row.file;
      }

      if (row.column != prev.column) {
        out.push_back(5); // DW_LNS_set_column
        encodeULEB128(row.column, out);
        prev.column = row.column;
      }

      if (row.discriminator != prev.discriminator) {
        out.push_back(0); // extended opcode
        unsigned size = llvm::getULEB128Size(row.discriminator);
        encodeULEB128(size + 1, out);
        out.push_back(4);
        encodeULEB128(row.discriminator, out);
        prev.discriminator = row.discriminator;
      }

      if (row.isa != prev.isa) {
        out.push_back(12); // DW_LNS_set_isa
        encodeULEB128(row.isa, out);
        prev.isa = row.isa;
      }

      if (row.is_stmt != prev.is_stmt) {
        out.push_back(6); // DW_LNS_negate_stmt
        prev.is_stmt = row.is_stmt;
      }

      if (row.basic_block && !prev.basic_block) {
        out.push_back(7); // DW_LNS_set_basic_block
        prev.basic_block = true;
      }

      if (row.prologue_end && !prev.prologue_end) {
        out.push_back(10); // DW_LNS_set_prologue_end
        prev.prologue_end = true;
      }

      if (row.epilogue_begin && !prev.epilogue_begin) {
        out.push_back(11); // DW_LNS_set_epilogue_begin
        prev.epilogue_begin = true;
      }

      if (prev.end_sequence && !row.end_sequence) {
        prev.end_sequence = false;
        out.push_back(0); // extended opcode
        encodeULEB128(1 + header.address_size, out);
        out.push_back(2); // DW_LNE_SET_ADDRESS
        for (int i = 0; i < header.address_size; ++i)
          out.push_back((row.address >> (i * 8)) & 0xff);
        prev.address = row.address;
      }
      int64_t tempSigned;
      uint64_t Temp, Opcode;
      bool needCopy = false;

      tempSigned = lineDelta - header.line_base;

      if (tempSigned >= header.line_range || tempSigned + header.opcode_base > 255 || tempSigned < 0) {
        out.push_back(3);
        encodeSLEB128(lineDelta, out);
        lineDelta = 0;
        prev.line = row.line;
        tempSigned = 0 - header.line_base;
        needCopy = true;
      }

      if (lineDelta == 0 && addrDelta == 0) {
        out.push_back(1);
        continue;
      }

      Temp = static_cast<uint64_t>(tempSigned) + header.opcode_base;

      if (addrDelta >= 0 && static_cast<uint64_t>(addrDelta) < 256 + maxSpecialAddrDelta) {
        Opcode = Temp + addrDelta * header.line_range;
        if (Opcode <= 255) {
          out.push_back(Opcode);
          prev.discriminator = 0;
          prev.address = row.address;
          prev.line = row.line;
          prev.basic_block = false;
          prev.prologue_end = false;
          prev.epilogue_begin = false;
          continue;
        }

        // Try using DW_LNS_const_add_pc followed by special op.
        Opcode = Temp + (addrDelta - maxSpecialAddrDelta) * header.line_range;
        if (Opcode <= 255) {
          out.push_back(8);
          out.push_back(Opcode);
          prev.discriminator = 0;
          prev.address = row.address;
          prev.line = row.line;
          prev.basic_block = false;
          prev.prologue_end = false;
          prev.epilogue_begin = false;
          continue;
        }
      }

      out.push_back(2); // DW_LNS_advance_pc
      encodeULEB128(addrDelta / header.min_inst_length, out);

      if (needCopy)
        out.push_back(1);
      else {
        assert(Temp <= 255 && "Buggy special opcode encoding.");
        out.push_back(Temp);
      }
      prev.discriminator = 0;
      prev.address = row.address;
      prev.line = row.line;
    }

    uint32_t unitLength = static_cast<uint32_t>(out.size() - 4);
    for (int i = 0; i < 4; ++i)
      out[totalLengthOffset + i] = (unitLength >> (i * 8)) & 0xff;

    assert(out.size() <= sh_size &&
           "Rewritten .debug_line larger than original");
    memcpy(buffer, out.data(), out.size());
  }

  FormValueRaw parseFormValue(
      uint64_t form, const uint8_t *&p, const uint8_t *end,
      //                           const DebugInfoSection::CompileUnitHeader &cu,
      const DebugStrOffsetsSection *strOffsets = nullptr,
      const DebugStrSection *strSection = nullptr,
      const DebugAddrSection *addrSection = nullptr, uint64_t dieOffset = 0,
      int strOffsetsTableIndex = -1, uint64_t addrBaseOffset = 0,
      std::optional<int64_t> implicitConst = std::nullopt) {
    FormValueRaw result;
    result.form = form;
    const uint8_t *start = p;

    switch (form) {
    case 0x01: { // DW_FORM_addr
      if (end - p < 8) {
        llvm::errs() << "Error: DW_FORM_addr: not enough bytes left in buffer\n";
        p = end;
        break;
      }
      result.value = *reinterpret_cast<const uint64_t *>(p);
      p += 8;
      break;
    }
    case 0x03: { // DW_FORM_block2
      uint16_t len = *reinterpret_cast<const uint16_t *>(p);
      p += 2;
      result.blockData.insert(result.blockData.end(), p, p + len);
      p += len;
      break;
    }
    case 0x04: { // DW_FORM_block4
      uint32_t len = *reinterpret_cast<const uint32_t *>(p);
      p += 4;
      result.blockData.insert(result.blockData.end(), p, p + len);
      p += len;
      break;
    }
    case 0x05: { // DW_FORM_data2
      result.value = *reinterpret_cast<const uint16_t *>(p);
      p += 2;
      break;
    }
    case 0x06: { // DW_FORM_data4
      result.value = *reinterpret_cast<const uint32_t *>(p);
      p += 4;
      break;
    }
    case 0x07: { // DW_FORM_data8
      result.value = *reinterpret_cast<const uint64_t *>(p);
      p += 8;
      break;
    }
    case 0x08: { // DW_FORM_string
      result.str = std::string(reinterpret_cast<const char *>(p));
      p += result.str.size() + 1;
      break;
    }
    case 0x09: { // DW_FORM_block
      unsigned size = 0;
      uint64_t len = decodeULEB128(p, &size, end);
      p += size;
      std::ostringstream oss;
      result.blockData.insert(result.blockData.end(), p, p + len);
      p += len;
      break;
    }
    case 0x0a: { // DW_FORM_block1
      uint8_t len = *p++;
      result.blockData.insert(result.blockData.end(), p, p + len);
      p += len;
      break;
    }
    case 0x0b: { // DW_FORM_data1
      result.value = *p++;
      break;
    }
    case 0x0c: { // DW_FORM_flag
      result.flag = (*p++) != 0;
      break;
    }
    case 0x0d: { // DW_FORM_sdata
      unsigned size = 0;
      result.value = decodeSLEB128(p, &size, end);
      p += size;
      break;
    }
    case 0x0e: { // DW_FORM_strp
      result.value = *reinterpret_cast<const uint32_t *>(p);
      p += 4;
      if (strSection) {
        result.str = strSection->getString(result.value);
      } else {
        result.str = "";
      }
      break;
    }
    case 0x0f: { // DW_FORM_udata
      unsigned len = 0;
      result.value = decodeULEB128(p, &len, end);
      p += len;
      break;
    }
    case 0x10: // DW_FORM_ref_addr
    case 0x1c: // DW_FORM_ref_sup4
    case 0x24: // DW_FORM_ref_sup8
    case 0x20:
    case 0x14: { // DW_FORM_ref_sig8
      result.value = *reinterpret_cast<const uint64_t *>(p);
      p += 8;
      break;
    }
    case 0x11: {
      result.value = *p++;
      break;
    }
    case 0x12: {
      result.value = *reinterpret_cast<const uint16_t *>(p);
      p += 2;
      break;
    }
    case 0x13: {
      result.value = *reinterpret_cast<const uint32_t *>(p);
      p += 4;
      break;
    }
    case 0x15: { // DW_FORM_ref_udata
      unsigned len = 0;
      result.value = decodeULEB128(p, &len, end);
      p += len;
      break;
    }
    case 0x16: { // DW_FORM_indirect
      unsigned len = 0;
      uint64_t actualForm = decodeULEB128(p, &len, end);
      p += len;
      return parseFormValue(actualForm, p, end, strOffsets, strSection,
                            addrSection, dieOffset);
    }
    case 0x17: { // DW_FORM_sec_offset
      result.value = *reinterpret_cast<const uint32_t *>(p);
      p += 4;
      break;
    }
    case 0x18: { // DW_FORM_exprloc
      unsigned len = 0;
      uint64_t size = decodeULEB128(p, &len, end);
      p += len;
      result.blockData.insert(result.blockData.end(), p, p + size);
      p += size;
      break;
    }
    case 0x19: {
      result.flag = true;
      break;
    }
    case 0x1b: {
      unsigned len = 0;
      result.value = decodeULEB128(p, &len, end);
      p += len;
      break;
    }
    case 0x1f: {
      result.value = *reinterpret_cast<const uint32_t *>(p);
      p += 4;
      break;
    }
    case 0x1d: { // DW_FORM_strp_sup
      result.value = *reinterpret_cast<const uint32_t *>(p);
      result.str = std::to_string(result.value);
      p += 4;
      break;
    }
    case 0x1e: { // DW_FORM_data16
      result.blockData.insert(result.blockData.end(), p, p + 16);
      p += 16;
      break;
    }
    case 0x21: { // DW_FORM_implicit_const
      if (implicitConst.has_value())
        result.value = implicitConst.value();
      else
        llvm::errs() << "DW_FORM_implicit_const missing value at DIE offset 0x"
                     << intToHex(dieOffset, 8) << "\n";
      break;
    }
    case 0x22:
    case 0x23: { // DW_FORM_rnglistx
      unsigned len = 0;
      result.value = decodeULEB128(p, &len, end);
      p += len;
      break;
    }
    case 0x25: { // DW_FORM_strx1
      result.value = *p++;
      if (strOffsets && strSection && strOffsetsTableIndex >= 0) {
        result.str =
            strOffsets->getStringFromStrx(strOffsetsTableIndex, result.value);
      }
      break;
    }

    case 0x1a:   // DW_FORM_strx
    case 0x26:   // DW_FORM_strx2
    case 0x27:   // DW_FORM_strx3
    case 0x28: { // DW_FORM_strx4
      unsigned len = 0;
      result.value = decodeULEB128(p, &len, end);
      p += len;
      if (strOffsets && strSection && strOffsetsTableIndex >= 0) {
        result.str =
            strOffsets->getStringFromStrx(strOffsetsTableIndex, result.value);
      }
      break;
    }
    case 0x29:
    case 0x2a:
    case 0x2b:
    case 0x2c: { // DW_FORM_addrx[1-4]
      unsigned len = 0;
      result.value = decodeULEB128(p, &len, end);
      p += len;
      break;
    }
    default:
      llvm::errs() << "Unsupported form 0x" + intToHex(form, 2);
    }
    result.rawBytes.assign(start, p);
    return result;
  }
};


// Ref 7.5
class DebugInfoSection final : public Section {
private:
  // Structure representing a compile unit header
  struct CompileUnitHeader {
    uint64_t offset;               // Offset in section
    uint32_t unitLength;           // Unit length
    uint16_t version;              // DWARF version
    uint8_t unitType;              // Unit type
    uint8_t addrSize;              // Address size
    uint32_t abbrevOffset;         // Abbreviation offset
    std::optional<uint64_t> dwoId; // Optional DWO ID
  };

  // Structure representing a DIE (Debugging Information Entry)
  struct DIE {
    uint64_t offset;     // Offset in section
    uint64_t abbrevCode; // Abbreviation code
    const DebugAbbrevSection::AbbreviationDecl
        *abbrevDecl; // Abbreviation declaration
    std::vector<std::pair<uint64_t, FormValueRaw>> attributes; // Attributes
    std::vector<DIE> children;                                 // Child DIEs
  };

  std::vector<CompileUnitHeader> cuHeaders; // Compile unit headers
  std::vector<std::vector<DIE>> cuDIEs; // Array of top-level DIEs for each CU
  const DebugAbbrevSection &abbrev;
  const DebugStrSection *debugStr;
  const DebugStrOffsetsSection *debugStrOffset;
  const DebugAddrSection *debugAddr;
  const DebugRnglistSection *debugRnglist;

public:
  DebugInfoSection(const llvm::object::ELF64LE::Shdr *shdr, const char *_data,
                   const DebugAbbrevSection &abbrevRef,
                   const DebugStrSection *strRef,
                   const DebugStrOffsetsSection *strOffsetRef,
                   const DebugAddrSection *addrRef,
                   const DebugRnglistSection *rnglistRef)
      : Section(SectionType::DebugInfo, shdr, _data), abbrev(abbrevRef),
        debugStr(strRef), debugStrOffset(strOffsetRef), debugAddr(addrRef), debugRnglist(rnglistRef) {
    const uint8_t *start = reinterpret_cast<const uint8_t *>(data);
    const uint8_t *end = start + sh_size;
    const uint8_t *p = start;

    while (p + 12 <= end) {
      uint64_t offset = p - start;

      // Reference: "DWARF5", page 200.
//      uint32_t unitLength = *reinterpret_cast<const uint32_t *>(p);
      uint32_t unitLength = llvm::support::endian::read32le(p);
      p += 4;
      uint16_t version = llvm::support::endian::read16le(p);
      p += 2;
      uint8_t unitType = *p++;
      uint8_t addrSize = *p++;
      uint32_t abbrevOffset = llvm::support::endian::read32le(p);
      p += 4;

      std::optional<uint64_t> dwoId;
      if (unitType == 0x04 || unitType == 0x05) {
        if (p + 8 <= end) {
          dwoId = llvm::support::endian::read64le(p);
          p += 8;
        }
      }

      cuHeaders.push_back({offset, unitLength, version, unitType, addrSize,
                           abbrevOffset, dwoId});

      const uint8_t *cuEnd = start + offset + 4 + unitLength;

      // parse root DIE early to extract str_offsets_base
      int strOffsetsTableIndex = -1;
      uint64_t addrBaseOffset = 0;
      const uint8_t *tmp = p;

      uint64_t dieOffset = tmp - start;
      unsigned len = 0;
      uint64_t abbrevCode = decodeULEB128(tmp, &len, cuEnd);
      tmp += len;

      const auto *decl = abbrev.getAbbreviationDecl(abbrevOffset, abbrevCode);
      if (decl) {
        for (const auto &af : decl->attrForms) {
          if (af.attr == 0x72) {
            uint64_t val =
                parseFormValue(af.form, tmp, cuEnd, nullptr, nullptr, nullptr,
                               dieOffset, strOffsetsTableIndex, addrBaseOffset)
                    .value;
            assert(debugStrOffset != nullptr);
            strOffsetsTableIndex = debugStrOffset->getTableIndex(val);
          } else if (af.attr == 0x73) {
            addrBaseOffset =
                parseFormValue(af.form, tmp, cuEnd, nullptr, nullptr, nullptr,
                               dieOffset, strOffsetsTableIndex, addrBaseOffset)
                    .value;

          } else {
            skipFormValue(af.form, tmp, cuEnd); // Skip uninteresting attributes
          }
        }
      }

      std::vector<DIE> topLevelDIEs;
      while (p < cuEnd) {
        DIE die = parseDIE(p, start, cuEnd, abbrevOffset, strOffsetsTableIndex,
                           addrBaseOffset);
        if (die.abbrevDecl == nullptr)
          break;
        topLevelDIEs.push_back(std::move(die));
      }
      cuDIEs.push_back(std::move(topLevelDIEs));
      p = start + offset + 4 + unitLength;
    }
  }

  DIE parseDIE(const uint8_t *&p, const uint8_t *start, const uint8_t *end,
               uint32_t abbrevOffset, int strOffsetsTableIndex = -1,
               uint64_t addrBaseOffset = 0) {
    uint64_t offset = p - start;
    unsigned len = 0;
    uint64_t abbrevCode = decodeULEB128(p, &len, end);
    p += len;

    if (abbrevCode == 0)
      return {};

    const auto *decl = abbrev.getAbbreviationDecl(abbrevOffset, abbrevCode);
    if (!decl)
      return {};

    DIE die;
    die.offset = offset;
    die.abbrevCode = abbrevCode;
    die.abbrevDecl = decl;

    for (const auto &af : decl->attrForms) {
      FormValueRaw valueRaw;
      if (af.form == 0x21)
        valueRaw = parseFormValue(af.form, p, end, debugStrOffset, debugStr,
                                  debugAddr, die.offset, strOffsetsTableIndex,addrBaseOffset,
                                  af.implicitConst.value());
      else
        valueRaw = parseFormValue(af.form, p, end, debugStrOffset, debugStr,
                                  debugAddr, die.offset, strOffsetsTableIndex,
                                  addrBaseOffset);
      die.attributes.emplace_back(af.attr, valueRaw);

      if (af.attr == 0x55 /* DW_AT_ranges */) {
        uint32_t rnglistIndex = static_cast<uint32_t>(valueRaw.value);
        assert(debugRnglist != nullptr);
        debugRnglist->registerAddrBase(rnglistIndex, addrBaseOffset);
      }
    }

    if (decl->hasChildren) {
      while (true) {
        auto child =
            parseDIE(p, start, end, abbrevOffset, strOffsetsTableIndex);
        if (child.abbrevDecl == nullptr) // Empty DIE indicates end of children
          break;
        die.children.push_back(std::move(child));
      }
    }

    return die;
  }

  void dumpData(std::ostream &oss) const override {
    oss << ".debug_info contents:\n";
    // dump Compile Unit
    for (size_t i = 0; i < cuHeaders.size(); ++i) {
      const auto &cu = cuHeaders[i];
      const std::string unitTypeStr = getUnitType(cu.unitType);

      oss << std::hex << std::setfill('0');
      oss << "0x" << std::setw(8) << cu.offset << ": Compile Unit: ";
      oss << "length = 0x" << std::setw(8) << cu.unitLength << ", ";
      oss << "format = DWARF32, ";
      oss << "version = 0x" << std::setw(4) << cu.version << ", ";
      oss << "unit_type = " << unitTypeStr << ", ";
      oss << "abbr_offset = 0x" << std::setw(4) << cu.abbrevOffset << ", ";
      oss << "addr_size = 0x" << std::setw(2) << static_cast<int>(cu.addrSize)
          << " ";
      if (cu.dwoId.has_value())
        oss << ", DWO_id = " << std::setw(12) << cu.dwoId.value() << " ";
      // NextUnitOffset = Offset + Length + LengthFieldByteSize
      oss << "(next unit at 0x" << std::setw(8)
          << (cu.offset + 4 + cu.unitLength) << ")\n";
      oss << "\n";
      for (const auto &die : cuDIEs[i]) {
        dumpDIE(oss, die);
      }
    }
  }

  void dumpDIE(std::ostream &oss, const DIE &die) const {
    oss << "0x" << std::setw(8) << std::setfill('0') << std::hex << die.offset
        << ": " << getTagName(die.abbrevDecl->tag) << "\n";

    for (const auto &[attr, val] : die.attributes) {
      if (val.form == 0x08 || val.form == 0x25) {
        oss << std::string(14, ' ') << getAttrName(attr) << "\t(\"" << val.str
            << "\")\n";
      } else if (val.form == 0x03 || val.form == 0x04 || val.form == 0x09 ||
                 val.form == 0x0a || val.form == 0x18 || val.form == 0x1e) {
        oss << std::string(14, ' ') << getAttrName(attr)
            << "\t[BLOCK DATA, size=" << val.blockData.size() << "]: ";

        for (uint8_t byte : val.blockData) {
          oss << "0x" << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<int>(byte) << " ";
        }
        oss << std::dec << "\n";
      } else {
        oss << std::string(14, ' ') << getAttrName(attr) << "\t(" << val.value
            << ")\n";
      }
    }

    for (const auto &child : die.children)
      dumpDIE(oss, child);
  }

  void writeDataTo(char *buffer) override {
    std::vector<uint8_t> out;

    for (size_t i = 0; i < cuHeaders.size(); ++i) {
      const auto &cu = cuHeaders[i];
      size_t headerStart = out.size();

      // 1. Reserve space for unit_length (to be filled later)
      out.resize(out.size() + 4); // DWARF32 format

      // 2. Write header
      out.push_back(cu.version & 0xff);
      out.push_back((cu.version >> 8) & 0xff);
      out.push_back(cu.unitType);
      out.push_back(cu.addrSize);

      out.push_back(cu.abbrevOffset & 0xff);
      out.push_back((cu.abbrevOffset >> 8) & 0xff);
      out.push_back((cu.abbrevOffset >> 16) & 0xff);
      out.push_back((cu.abbrevOffset >> 24) & 0xff);

      if (cu.dwoId.has_value()) {
        uint64_t id = cu.dwoId.value();
        for (int j = 0; j < 8; ++j)
          out.push_back((id >> (j * 8)) & 0xff);
      }

      // 3. Write all top-level DIEs
      for (const auto &die : cuDIEs[i])
        writeDIE(die, out);

      // 4. Fill in unit_length
      uint32_t length = static_cast<uint32_t>(out.size() - headerStart - 4);
      out[headerStart + 0] = (length & 0xff);
      out[headerStart + 1] = (length >> 8) & 0xff;
      out[headerStart + 2] = (length >> 16) & 0xff;
      out[headerStart + 3] = (length >> 24) & 0xff;
    }

    // 5. Copy to target buffer
    assert(out.size() <= sh_size &&
           "Rewritten .debug_info larger than original");
    memcpy(buffer, out.data(), out.size());
  }

  void writeDIE(const DIE &die, std::vector<uint8_t> &out) const {
    encodeULEB128(die.abbrevCode, out);

    const auto *decl = die.abbrevDecl;
    if (!decl)
      return;

    for (size_t i = 0; i < decl->attrForms.size(); ++i) {
      const auto &val = die.attributes[i].second;
      writeFormValue(val, out);
    }

    if (decl->hasChildren) {
      for (const auto &child : die.children)
        writeDIE(child, out);

      out.push_back(0x00); // Null abbrev code for end of children
    }
  }

  FormValueRaw parseFormValue(
      uint64_t form, const uint8_t *&p, const uint8_t *end,
      //                           const DebugInfoSection::CompileUnitHeader &cu,
      const DebugStrOffsetsSection *strOffsets = nullptr,
      const DebugStrSection *strSection = nullptr,
      const DebugAddrSection *addrSection = nullptr, uint64_t dieOffset = 0,
      int strOffsetsTableIndex = -1, uint64_t addrBaseOffset = 0,
      std::optional<int64_t> implicitConst = std::nullopt) {
    FormValueRaw result;
    result.form = form;
    const uint8_t *start = p;

    switch (form) {
    case 0x01: { // DW_FORM_addr
      if (end - p < 8) {
        llvm::errs() << "Error: DW_FORM_addr: not enough bytes left in buffer\n";
        p = end;
        break;
      }
      result.value = *reinterpret_cast<const uint64_t *>(p);
      p += 8;
      break;
    }
    case 0x03: { // DW_FORM_block2
      uint16_t len = *reinterpret_cast<const uint16_t *>(p);
      p += 2;
      result.blockData.insert(result.blockData.end(), p, p + len);
      p += len;
      break;
    }
    case 0x04: { // DW_FORM_block4
      uint32_t len = *reinterpret_cast<const uint32_t *>(p);
      p += 4;
      result.blockData.insert(result.blockData.end(), p, p + len);
      p += len;
      break;
    }
    case 0x05: { // DW_FORM_data2
      result.value = *reinterpret_cast<const uint16_t *>(p);
      p += 2;
      break;
    }
    case 0x06: { // DW_FORM_data4
      result.value = *reinterpret_cast<const uint32_t *>(p);
      p += 4;
      break;
    }
    case 0x07: { // DW_FORM_data8
      result.value = *reinterpret_cast<const uint64_t *>(p);
      p += 8;
      break;
    }
    case 0x08: { // DW_FORM_string
      result.str = std::string(reinterpret_cast<const char *>(p));
      p += result.str.size() + 1;
      break;
    }
    case 0x09: { // DW_FORM_block
      unsigned size = 0;
      uint64_t len = decodeULEB128(p, &size, end);
      p += size;
      std::ostringstream oss;
      result.blockData.insert(result.blockData.end(), p, p + len);
      p += len;
      break;
    }
    case 0x0a: { // DW_FORM_block1
      uint8_t len = *p++;
      result.blockData.insert(result.blockData.end(), p, p + len);
      p += len;
      break;
    }
    case 0x0b: { // DW_FORM_data1
      result.value = *p++;
      break;
    }
    case 0x0c: { // DW_FORM_flag
      result.flag = (*p++) != 0;
      break;
    }
    case 0x0d: { // DW_FORM_sdata
      unsigned size = 0;
      result.value = decodeSLEB128(p, &size, end);
      p += size;
      break;
    }
    case 0x0e: { // DW_FORM_strp
      result.value = *reinterpret_cast<const uint32_t *>(p);
      p += 4;
      if (strSection) {
        result.str = strSection->getString(result.value);
      } else {
        result.str = "";
      }
      break;
    }
    case 0x0f: { // DW_FORM_udata
      unsigned len = 0;
      result.value = decodeULEB128(p, &len, end);
      p += len;
      break;
    }
    case 0x10: // DW_FORM_ref_addr
    case 0x1c: // DW_FORM_ref_sup4
    case 0x24: // DW_FORM_ref_sup8
    case 0x20:
    case 0x14: { // DW_FORM_ref_sig8
      result.value = *reinterpret_cast<const uint64_t *>(p);
      p += 8;
      break;
    }
    case 0x11: {
      result.value = *p++;
      break;
    }
    case 0x12: {
      result.value = *reinterpret_cast<const uint16_t *>(p);
      p += 2;
      break;
    }
    case 0x13: {
      result.value = *reinterpret_cast<const uint32_t *>(p);
      p += 4;
      break;
    }
    case 0x15: { // DW_FORM_ref_udata
      unsigned len = 0;
      result.value = decodeULEB128(p, &len, end);
      p += len;
      break;
    }
    case 0x16: { // DW_FORM_indirect
      unsigned len = 0;
      uint64_t actualForm = decodeULEB128(p, &len, end);
      p += len;
      return parseFormValue(actualForm, p, end, strOffsets, strSection,
                            addrSection, dieOffset);
    }
    case 0x17: { // DW_FORM_sec_offset
      result.value = *reinterpret_cast<const uint32_t *>(p);
      p += 4;
      break;
    }
    case 0x18: { // DW_FORM_exprloc
      unsigned len = 0;
      uint64_t size = decodeULEB128(p, &len, end);
      p += len;
      result.blockData.insert(result.blockData.end(), p, p + size);
      p += size;
      break;
    }
    case 0x19: {
      result.flag = true;
      break;
    }
    case 0x1b: {
      unsigned len = 0;
      result.value = decodeULEB128(p, &len, end);
      p += len;
      break;
    }
    case 0x1d:
    case 0x1f: { // DW_FORM_strp_sup
      result.value = *reinterpret_cast<const uint32_t *>(p);
      p += 4;
      break;
    }
    case 0x1e: { // DW_FORM_data16
      result.blockData.insert(result.blockData.end(), p, p + 16);
      p += 16;
      break;
    }
    case 0x21: { // DW_FORM_implicit_const
      assert(implicitConst.has_value());
      if (implicitConst.has_value())
        result.value = implicitConst.value();
      else
        llvm::errs() << "DW_FORM_implicit_const missing value at DIE offset 0x"
                     << intToHex(dieOffset, 8) << "\n";
      break;
    }
    case 0x22:
    case 0x23: { // DW_FORM_rnglistx
      unsigned len = 0;
      result.value = decodeULEB128(p, &len, end);
      p += len;
      break;
    }
    case 0x25: { // DW_FORM_strx1
      result.value = *p++;
      if (strOffsets && strSection && strOffsetsTableIndex >= 0) {
        result.str =
            strOffsets->getStringFromStrx(strOffsetsTableIndex, result.value);
      }
      break;
    }

    case 0x1a: { // DW_FORM_strx
      unsigned len = 0;
      result.value = decodeULEB128(p, &len, end);
      p += len;
      if (strOffsets && strSection && strOffsetsTableIndex >= 0) {
        result.str =
            strOffsets->getStringFromStrx(strOffsetsTableIndex, result.value);
      }
      break;
    }
    case 0x26: // strx2
      result.value = *reinterpret_cast<const uint16_t *>(p);
      p += 2;
      if (strOffsets && strSection && strOffsetsTableIndex >= 0) {
        result.str =
            strOffsets->getStringFromStrx(strOffsetsTableIndex, result.value);
      }
      break;
    case 0x27: // strx3
      result.value = (*reinterpret_cast<const uint32_t *>(p)) & 0xFFFFFFu;
      p += 3;
      if (strOffsets && strSection && strOffsetsTableIndex >= 0) {
        result.str =
            strOffsets->getStringFromStrx(strOffsetsTableIndex, result.value);
      }
      break;
    case 0x28: // strx4
      result.value = *reinterpret_cast<const uint32_t *>(p);
      p += 4;
      if (strOffsets && strSection && strOffsetsTableIndex >= 0) {
        result.str =
            strOffsets->getStringFromStrx(strOffsetsTableIndex, result.value);
      }
      break;
    case 0x29:
    case 0x2a:
    case 0x2b:
    case 0x2c: { // DW_FORM_addrx[1-4]
      unsigned len = 0;
      result.value = decodeULEB128(p, &len, end);
      p += len;
      break;
    }
    default:
      llvm::errs() << "Unsupported form 0x" + intToHex(form, 2);
    }
    result.rawBytes.assign(start, p);
    return result;
  }
};

class DebugArangeSection final : public Section {
private:
  struct ArangeEntry {
    uint64_t address;
    uint64_t length;
  };

  struct ArangeSet {
    uint32_t unit_length;
    uint16_t version;
    uint32_t debug_info_offset;
    uint8_t address_size;
    uint8_t segment_size;
    std::vector<ArangeEntry> ranges;
  };

  std::vector<ArangeSet> aranges;

public:
  DebugArangeSection(const llvm::object::ELF64LE::Shdr *shdr, const char *_data)
      : Section(SectionType::DebugAranges, shdr, _data) {
    const uint8_t *start = reinterpret_cast<const uint8_t *>(data);
    const uint8_t *end = start + sh_size;

    while (start < end) {
      ArangeSet set{};

      if (end - start < 4) break;
      set.unit_length = *reinterpret_cast<const uint32_t *>(start);
      start += 4;
      if (set.unit_length == 0) break; // 结束
      const uint8_t *set_end = start + set.unit_length;

      if (end - start < 2) break;
      set.version = *reinterpret_cast<const uint16_t *>(start);
      start += 2;

      if (end - start < 4) break;
      set.debug_info_offset = *reinterpret_cast<const uint32_t *>(start);
      start += 4;

      set.address_size = *start++;
      set.segment_size = *start++;

      // 对齐到 2*address_size 边界
      size_t header_size =
          4 + 2 + 4 + 1 + 1; // unit_length+version+debug_info_offset+addr_size+seg_size
      size_t padding = (2 * set.address_size) -
                       (header_size % (2 * set.address_size));
      if (padding != 2 * set.address_size) {
        start += padding;
      }

      // 解析地址范围 entries
      while (start < set_end) {
        uint64_t addr = 0, length = 0;

        if (set.address_size == 4) {
          addr = *reinterpret_cast<const uint32_t *>(start);
          start += 4;
          length = *reinterpret_cast<const uint32_t *>(start);
          start += 4;
        } else if (set.address_size == 8) {
          addr = *reinterpret_cast<const uint64_t *>(start);
          start += 8;
          length = *reinterpret_cast<const uint64_t *>(start);
          start += 8;
        } else {
          llvm::errs() << "Unsupported address_size in .debug_aranges";
        }

        if (addr == 0 && length == 0) break; // terminator
        set.ranges.push_back({addr, length});
      }

      aranges.push_back(std::move(set));
      start = set_end;
    }
  }

  void writeDataTo(char *buffer) override {
    std::vector<uint8_t> out;

    for (const auto &set : aranges) {
      // unit_length
      size_t headerStart = out.size();
      out.resize(out.size() + 4);
      // version
      out.insert(out.end(), reinterpret_cast<const uint8_t *>(&set.version),
                 reinterpret_cast<const uint8_t *>(&set.version) + 2);

      // debug_info_offset
      out.insert(out.end(),
                 reinterpret_cast<const uint8_t *>(&set.debug_info_offset),
                 reinterpret_cast<const uint8_t *>(&set.debug_info_offset) + 4);

      // addr_size, seg_size
      out.push_back(set.address_size);
      out.push_back(set.segment_size);

      // 对齐填充
      size_t header_size = 4 + 2 + 4 + 1 + 1;
      size_t padding = (2 * set.address_size) -
                       (header_size % (2 * set.address_size));
      if (padding != 2 * set.address_size) {
        out.insert(out.end(), padding, 0);
      }

      // ranges
      for (const auto &r : set.ranges) {
        if (set.address_size == 4) {
          uint32_t a = static_cast<uint32_t>(r.address);
          uint32_t l = static_cast<uint32_t>(r.length);
          out.insert(out.end(), reinterpret_cast<uint8_t *>(&a),
                     reinterpret_cast<uint8_t *>(&a) + 4);
          out.insert(out.end(), reinterpret_cast<uint8_t *>(&l),
                     reinterpret_cast<uint8_t *>(&l) + 4);
        } else {
          uint64_t a = r.address;
          uint64_t l = r.length;
          out.insert(out.end(), reinterpret_cast<uint8_t *>(&a),
                     reinterpret_cast<uint8_t *>(&a) + 8);
          out.insert(out.end(), reinterpret_cast<uint8_t *>(&l),
                     reinterpret_cast<uint8_t *>(&l) + 8);
        }
      }

      // terminator
      for (int i = 0; i < set.address_size * 2; i++)
        out.push_back(0);

      uint32_t length = static_cast<uint32_t>(out.size() - headerStart - 4);
      out[headerStart + 0] = (length & 0xff);
      out[headerStart + 1] = (length >> 8) & 0xff;
      out[headerStart + 2] = (length >> 16) & 0xff;
      out[headerStart + 3] = (length >> 24) & 0xff;
    }

    assert(out.size() <= sh_size &&
           "Rewritten .debug_aranges larger than original");
    memcpy(buffer, out.data(), out.size());
  }

  void dumpData(std::ostream &oss) const override {
    oss << ".debug_aranges contents:\n";
    for (size_t i = 0; i < aranges.size(); i++) {
      const auto &set = aranges[i];
      oss << "Arange Range Header:";
      oss << " length = " << set.unit_length << ", format = DWARF32";
      oss << ", version = 0x" << intToHex(set.version, 4);
      oss << ", cu_offset = 0x" << intToHex(set.debug_info_offset, 8);
      oss << ", addr_size = 0x" << intToHex(set.address_size, 2)
          << ", seg_size = 0x" << intToHex(set.segment_size, 2) << "\n";
      for (const auto &r : set.ranges) {
        oss << "    [0x" << intToHex(r.address, set.address_size * 2)
            << ", 0x" << intToHex(r.address + r.length, set.address_size * 2)
            << ")\n";
      }
    }
  }
};

class DebugLoclistsSection final : public Section {
private:
  struct Header {
    uint32_t  length;
    uint16_t  version;
    uint8_t   addrSize;
    uint8_t   segSize;
    uint32_t   offsetEntryCount;
  } header;

  std::vector<uint64_t > offsets;

  struct LocEntry {
    uint64_t  offset;
    uint8_t kind;
    std::vector<uint64_t> values;
    std::vector<uint8_t> expr;
  };

  std::vector<LocEntry> entries;

public:
  DebugLoclistsSection(const llvm::object::ELF64LE::Shdr *shdr, const char *_data)
      : Section(SectionType::DebugLoclists, shdr, _data) {
    // TODO : parse .debug_loclists
    const uint8_t *start = reinterpret_cast<const uint8_t *>(data);
    const uint8_t *end = start + sh_size;

    header.length = llvm::support::endian::read32le(start);
    start += 4;
    header.version = llvm::support::endian::read16le(start);
    start += 2;
    header.addrSize = *start++;
    header.segSize = *start++;
    header.offsetEntryCount = llvm::support::endian::read32le(start);
    start += 4;

    for (uint32_t i = 0; i < header.offsetEntryCount; i++) {
      uint32_t offset = llvm::support::endian::read32le(start);
      start += 4;
      offsets.push_back(offset);
    }

    unsigned n;
    while (start < end)
    {
      uint64_t entryoff = start - reinterpret_cast<const uint8_t *>(data);
      uint8_t kind = *start++;


      LocEntry entry;
      entry.offset = entryoff;
      entry.kind = kind;
      if (kind == 0) { // DW_LLE_end_of_list
        entries.push_back(entry);
        continue;
      }
      switch (kind) {
      case 0x01 : {
        uint64_t value = decodeULEB128(start, &n);
        start += n;
        entry.values.push_back(value);
        break;
      }
      case 0x02:
      case 0x03:
      case 0x04: {
        uint64_t value0 = decodeULEB128(start, &n);
        start += n;
        uint64_t value1 = decodeULEB128(start, &n);
        start += n;
        entry.values = {value0, value1};
        break;
      }
      case 0x06: {
        uint64_t addr = (header.addrSize == 8) ? llvm::support::endian::read64le(start) : llvm::support::endian::read32le(start);
        start += header.addrSize;
        entry.values.push_back(addr);
        break;
      }
      case 0x07: {
        uint64_t addrStart = (header.addrSize == 8) ? llvm::support::endian::read64le(start) : llvm::support::endian::read32le(start);
        start += header.addrSize;
        uint64_t addrEnd = (header.addrSize == 8) ? llvm::support::endian::read64le(start) : llvm::support::endian::read32le(start);
        start += header.addrSize;
        entry.values = {addrStart, addrEnd};
        break;
      }
      case 0x08: {
        uint64_t addr = (header.addrSize == 8) ? llvm::support::endian::read64le(start) : llvm::support::endian::read32le(start);
        start += header.addrSize;
        uint64_t length = decodeULEB128(start, &n);
        start += n;
        entry.values = {addr, length};
        break;
      }
      default: {
        assert(false && "Unhandled loclist entry kind");
        break;
      }
      }

      if (kind != 0x01 && kind != 0x06) {
        uint64_t exprLength = decodeULEB128(start, &n);
        start += n;
        entry.expr.insert(entry.expr.end(), start, start + exprLength);
        start += exprLength;
      }
      entries.push_back(std::move(entry));
    }
  }

  void writeDataTo(char *buffer) override {
    std::vector<uint8_t> out;

    uint32_t len = (uint32_t)header.length;
    out.push_back(len & 0xff);
    out.push_back((len >> 8) & 0xff);
    out.push_back((len >> 16) & 0xff);
    out.push_back((len >> 24) & 0xff);

    out.push_back(header.version & 0xff);
    out.push_back((header.version >> 8) & 0xff);

    out.push_back(header.addrSize);
    out.push_back(header.segSize);

    uint32_t oc = header.offsetEntryCount;
    out.push_back(oc & 0xff);
    out.push_back((oc >> 8) & 0xff);
    out.push_back((oc >> 16) & 0xff);
    out.push_back((oc >> 24) & 0xff);

    for (auto off : offsets) {
      uint32_t val = (uint32_t)off;
      out.push_back(val & 0xff);
      out.push_back((val >> 8) & 0xff);
      out.push_back((val >> 16) & 0xff);
      out.push_back((val >> 24) & 0xff);
    }

    for (auto &e : entries) {
      out.push_back(e.kind);

      switch (e.kind) {
      case 0x01:
        encodeULEB128(e.values[0], out);
        break;
      case 0x02:
      case 0x03:
      case 0x04:
        encodeULEB128(e.values[0], out);
        encodeULEB128(e.values[1], out);
        break;
      case 0x06: {
        uint64_t v = e.values[0];
        for (int i = 0; i < header.addrSize; i++)
          out.push_back((v >> (i * 8)) & 0xff);
        break;
      }
      case 0x07: {
        uint64_t v1 = e.values[0];
        uint64_t v2 = e.values[1];
        for (int i = 0; i < header.addrSize; i++)
          out.push_back((v1 >> (i * 8)) & 0xff);
        for (int i = 0; i < header.addrSize; i++)
          out.push_back((v2 >> (i * 8)) & 0xff);
        break;
      }
      case 0x08: { // start_length
        uint64_t v1 = e.values[0];
        for (int i = 0; i < header.addrSize; i++)
          out.push_back((v1 >> (i * 8)) & 0xff);
        encodeULEB128(e.values[1], out);
        break;
      }
      default:
        break;
      }
      if (!e.expr.empty()) {
        encodeULEB128(e.expr.size(), out);  // 先写长度
        out.insert(out.end(), e.expr.begin(), e.expr.end());
      }
    }

    assert(out.size() <= sh_size &&
           "Rewritten .debug_aranges larger than original");
    memcpy(buffer, out.data(), out.size());
  }

  void dumpData(std::ostream &oss) const override {
    oss << ".debug_loclists contents:\n";
    oss << "locations list header: length = 0x"
        << intToHex(header.length, 8)
        << ", format = DWARF32"
        << ", version = 0x" << intToHex(header.version, 4)
        << ", addr_size = 0x" << intToHex(header.addrSize, 2)
        << ", seg_size = 0x" << intToHex(header.segSize, 2)
        << ", offset_entry_count = 0x" << intToHex(header.offsetEntryCount, 8)
        << "\n";

    oss << "offsets: [\n";
    for (auto off : offsets)
      oss << "0x" << intToHex(off, 8) << "\n";
    oss << "]\n";

    for (auto &e : entries) {
      if (e.kind == 0x00) continue;
      if (e.kind == 0x01 || e.kind == 0x06)
      oss << "0x" << intToHex(e.offset, 8) << ": \n";
      oss << "            " << getLLEName(e.kind) << "(0x";
      switch (e.kind) {
        case 0x01:
        case 0x06:
          oss << intToHex(e.values[0], 16) << ")";
          break;
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x07:
          oss << intToHex(e.values[0], 16) << ", 0x"
          << intToHex(e.values[1], 16) << ")";
          break;
        case 0x08:
          oss << intToHex(e.values[0], 16) << ", len="
              << e.values[1] << ")";
          break;
        default:
          break;
      }
      if (!e.expr.empty()) {
        oss << ":";
        for (auto op : e.expr)
        oss << " 0x" << intToHex(op, 1);
      }
      oss << "\n";

    }
  }
};


// TODO .debug_rnglists, 7.28, clang, lld version 17.0.6
// int test() {
// int x = 1;
// x += 2;
// return x;
// }
//
// int main() {
//   int x = 1;
//   x += test();
//   return x;
// }


} // namespace elf
} // namespace funcv
} // namespace iclang

#endif // ICLANG_SECTION_HPP

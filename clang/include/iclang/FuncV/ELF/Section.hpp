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

  uint64_t originalOffset = 0;


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

  void setOriginalOffset(uint64_t off) { originalOffset = off; }

  uint64_t getOriginalOffset() const { return originalOffset; }

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

    if (!relaEhFrame) {
      // 对于可执行文件，如果没有重定位表是正常的，直接返回即可
      return;
    }
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

} // namespace elf
} // namespace funcv
} // namespace iclang

#endif // ICLANG_SECTION_HPP

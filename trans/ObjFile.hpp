//===--- ObjFile.hpp - ELF obj file --------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// Workflow:
// * Parse ELF header.
// * Parse section header table (Note: e_shnum, e_shstrndx).
// * Parse string table.
// * Parse .symtab_shndx, symbol table.
// * Parse other sections.
// * Parse references.
// * Create necessary sections.
// * Update section layout.
// * Update section header and content (Note: e_shnum and e_shstrndx).
// * Write.
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_OBJFILE_HPP
#define ICLANG_OBJFILE_HPP

#include "iclang/FuncV/ELF/Section.hpp"

namespace iclang {
namespace funcv {
namespace elf {

class ObjFile {
private:
  const BinFile &binFile;

  unsigned char e_ident[llvm::ELF::EI_NIDENT] = {};
  uint16_t e_type = 0;
  uint16_t e_machine = 0;
  uint32_t e_version = 0;
  uint64_t e_entry = 0;
  uint64_t e_phoff = 0;
  // Update while writing.
  uint64_t e_shoff = 0;
  uint32_t e_flags = 0;
  uint16_t e_ehsize = 0;
  uint16_t e_phentsize = 0;
  uint64_t e_phnum = 0;
  uint16_t e_shentsize = 0;
  // Update while writing.
  uint64_t e_shnum = 0;
  uint64_t e_shstrndx = 0;
  // e_shstrndx.
  std::shared_ptr<StringTableSection> shstrTab = nullptr;

  std::vector<std::shared_ptr<Section>> sections;

  std::shared_ptr<StringTableSection> strTab = nullptr;
  std::shared_ptr<SymbolTableSection> symTab = nullptr;
  std::shared_ptr<EhFrameSection> ehFrame = nullptr;
  std::shared_ptr<RelocationSection> relaEhFrame = nullptr;

  std::shared_ptr<DebugAbbrevSection> debugAbbrev;
  std::shared_ptr<DebugStrSection> debugStr;
  std::shared_ptr<DebugLineStrSection> debugLineStr;
  std::shared_ptr<DebugInfoSection> debugInfoSection;
  std::shared_ptr<DebugStrOffsetsSection> debugStrOff;
  std::shared_ptr<DebugAddrSection> debugAddr;
  std::shared_ptr<DebugRnglistSection> debugRnglist;
  std::shared_ptr<RelocationSection> relaDebugStrOffsets;
  std::shared_ptr<RelocationSection> relaDebugAddr;

  static bool getIsRela(const uint16_t m) {
    return m == llvm::ELF::EM_AARCH64 || m == llvm::ELF::EM_AMDGPU ||
           m == llvm::ELF::EM_HEXAGON || m == llvm::ELF::EM_PPC ||
           m == llvm::ELF::EM_PPC64 || m == llvm::ELF::EM_RISCV ||
           m == llvm::ELF::EM_X86_64;
  }

  void parseHeader() {
    const auto &logger = Logger::getInstance();

    using Elf_Ehdr = llvm::object::ELF64LE::Ehdr;

    const char *object = binFile.readBytes(0);
    const auto *ehdr = reinterpret_cast<const Elf_Ehdr *>(object);

    for (size_t i = 0; i < llvm::ELF::EI_NIDENT; i++) {
      e_ident[i] = ehdr->e_ident[i];
    }
    e_type = ehdr->e_type;
    e_machine = ehdr->e_machine;
    e_version = ehdr->e_version;
    e_entry = ehdr->e_entry;
    e_phoff = ehdr->e_phoff;
    e_shoff = ehdr->e_shoff;
    e_flags = ehdr->e_flags;
    e_ehsize = ehdr->e_ehsize;
    e_phentsize = ehdr->e_phentsize;
    e_phnum = ehdr->e_phnum;
    e_shentsize = ehdr->e_shentsize;
    e_shnum = ehdr->e_shnum;
    e_shstrndx = ehdr->e_shstrndx;

    // We do not support 32bit architecture.
    if (!getIsRela(e_machine)) {
      logger.fatal("We do not support rel");
    }
  }

  void parseSections() {
    const auto &logger = Logger::getInstance();

    using Elf_Shdr = llvm::object::ELF64LE::Shdr;

    const char *object = binFile.readBytes(0);
    std::vector<const Elf_Shdr *> shdrs;
    shdrs.reserve(e_shnum);

    if (e_shentsize != sizeof(Elf_Shdr)) {
      logger.fatal("Invalid ELF file: e_shentsize != sizeof(Elf_Shdr)");
    }

    // 1. Parse shdrs.
    // 1.1. Add the first shdr.
    const auto *firstShdr =
        reinterpret_cast<const Elf_Shdr *>(object + e_shoff);
    shdrs.push_back(firstShdr);
    // The ELF header can only store numbers up to SHN_LORESERVE in the e_shnum
    // and e_shstrndx fields. When the value of one of these fields exceeds
    // SHN_LORESERVE ELF requires us to put sentinel values in the ELF header
    // and use fields in the section header at index 0 to store the value. The
    // sentinel values and fields are: e_shnum = 0, SHdrs[0].sh_size = number of
    // sections. e_shstrndx = SHN_XINDEX, SHdrs[0].sh_link = .shstrtab section
    // index.
    if (e_shnum == 0) {
      e_shnum = shdrs[0]->sh_size;
    }
    if (e_shstrndx == llvm::ELF::SHN_XINDEX) {
      e_shstrndx = shdrs[0]->sh_link;
    }
    // 1.2. Add other shdrs.
    for (uint64_t i = 1; i < e_shnum; i++) {
      const auto *shdr = reinterpret_cast<const Elf_Shdr *>(object + e_shoff +
                                                            i * e_shentsize);
      shdrs.push_back(shdr);
    }

    // 2. Parse shdrs to sections. (depend on 1)
    sections.resize(shdrs.size(), nullptr);
    // 2.1. Parse string table and symbol table.
    std::shared_ptr<SymtabShndxSection> symTabShndx = nullptr;
    for (size_t i = 0; i < sections.size(); i++) {
      switch (shdrs[i]->sh_type) {
      case llvm::ELF::SHT_STRTAB:
        sections[i] = std::make_shared<StringTableSection>(
            shdrs[i], object + shdrs[i]->sh_offset);
        break;
      case llvm::ELF::SHT_SYMTAB:
        if (symTab != nullptr) {
          logger.fatal("multiple symbol tables");
        }
        symTab = std::make_shared<SymbolTableSection>(
            shdrs[i], object + shdrs[i]->sh_offset);
        sections[i] = symTab;
        break;
      case llvm::ELF::SHT_SYMTAB_SHNDX:
        if (symTabShndx != nullptr) {
          logger.fatal("multiple symtab shndx sections");
        }
        symTabShndx = std::make_shared<SymtabShndxSection>(
            shdrs[i], object + shdrs[i]->sh_offset);
        sections[i] = symTabShndx;
        break;
      default:
        break;
      }
    }
    if (symTab == nullptr) {
      logger.fatal("missing symbol table");
    }
    symTab->setSymtabShndx(symTabShndx);
    shstrTab =
        std::static_pointer_cast<StringTableSection>(sections[e_shstrndx]);
    if (shstrTab->getType() != SectionType::StrTab) {
      logger.fatal("invalid shstrtab");
    }
    strTab = std::static_pointer_cast<StringTableSection>(
        sections[symTab->getShLink()]);
    if (strTab->getType() != SectionType::StrTab) {
      logger.fatal("invalid strtab");
    }

    // 2.2. Parse other sections (depend on 2.1).

    const llvm::object::ELF64LE::Shdr* debugInfoShdr = nullptr;
    const char* debugInfoData = nullptr;
    size_t debugInfoIndex = -1;

    const llvm::object::ELF64LE::Shdr* debugStrOffShdr = nullptr;
    const char* debugStrOffData = nullptr;
    size_t debugStrOffIndex = -1;

    const llvm::object::ELF64LE::Shdr* debugAddrShdr = nullptr;
    const char* debugAddrData = nullptr;
    size_t debugAddrIndex = -1;

    const llvm::object::ELF64LE::Shdr* debugRnglistShdr = nullptr;
    const char* debugRnglistData = nullptr;
    size_t debugRnglistIndex = -1;

    for (size_t i = 0; i < sections.size(); i++) {
      const auto secName =
        shstrTab->parseOriginalIndex(shdrs[i]->sh_name);
      const auto secNameStr = secName->getValue();

      if (secNameStr == ".eh_frame") {
        sections[i] = std::make_shared<EhFrameSection>(
            shdrs[i], object + shdrs[i]->sh_offset);
        ehFrame = std::static_pointer_cast<EhFrameSection>(sections[i]);
        continue;
      }

      if (secNameStr == ".debug_abbrev") {
        debugAbbrev = std::make_shared<DebugAbbrevSection>(shdrs[i], object + shdrs[i]->sh_offset);
        sections[i] = debugAbbrev; // 保存进 sections 映射
        continue;
      }

      if (secNameStr == ".debug_info") {
        // 延迟构造 .debug_info，暂时记录必要信息
        debugInfoShdr = shdrs[i];
        debugInfoData = object + shdrs[i]->sh_offset;
        debugInfoIndex = i;
        continue;
      }

      if (secNameStr == ".debug_str_offsets") {
        debugStrOffShdr = shdrs[i];
        debugStrOffData = object + shdrs[i]->sh_offset;
        debugStrOffIndex = i;
        continue;
      }

      if (secNameStr == ".debug_str") {
        debugStr = std::make_shared<DebugStrSection>(shdrs[i], object + shdrs[i]->sh_offset);
        sections[i] = debugStr;
        continue;
      }

      if (secNameStr == ".debug_line_str") {
        debugLineStr = std::make_shared<DebugLineStrSection>(shdrs[i], object + shdrs[i]->sh_offset);
        sections[i] = debugLineStr;
        continue;
      }

      if (secNameStr == ".rela.debug_str_offsets") {
        relaDebugStrOffsets = std::static_pointer_cast<RelocationSection>(
            sections[i] = std::make_shared<RelocationSection>(shdrs[i], object + shdrs[i]->sh_offset));
        continue;
      }

      if (secNameStr == ".debug_addr") {
        debugAddrShdr = shdrs[i];
        debugAddrData = object + shdrs[i]->sh_offset;
        debugAddrIndex = i;
        continue;
      }

      if (secNameStr == ".rela.debug_addr") {
        relaDebugAddr = std::static_pointer_cast<RelocationSection>(
            sections[i] = std::make_shared<RelocationSection>(shdrs[i], object + shdrs[i]->sh_offset));
        continue;
      }

      if (secNameStr == ".debug_line") {
        sections[i] = std::make_shared<DebugLineSection>(shdrs[i], object + shdrs[i]->sh_offset);
        continue;
      }

      if (secNameStr == ".debug_rnglists") {
        debugRnglistShdr = shdrs[i];
        debugRnglistData = object + shdrs[i]->sh_offset;
        debugRnglistIndex = i;
        continue;
      }

      if (secNameStr == ".debug_loclists") {
        sections[i] = std::make_shared<DebugLoclistsSection>(shdrs[i], object + shdrs[i]->sh_offset);
        continue;
      }

      if (secNameStr == ".debug_aranges") {
        sections[i] = std::make_shared<DebugArangeSection>(shdrs[i], object + shdrs[i]->sh_offset);
        continue;
      }

      switch (shdrs[i]->sh_type) {
      case llvm::ELF::SHT_STRTAB:
      case llvm::ELF::SHT_SYMTAB:
        continue;
      case llvm::ELF::SHT_RELA:
        sections[i] = std::make_shared<RelocationSection>(
            shdrs[i], object + shdrs[i]->sh_offset);
        if (secName->getValue() == ".rela.eh_frame") {
          relaEhFrame =
              std::static_pointer_cast<RelocationSection>(sections[i]);
        }
        break;
      case llvm::ELF::SHT_GROUP:
        sections[i] = std::make_shared<GroupSection>(
            shdrs[i], object + shdrs[i]->sh_offset);
        break;
      default:
        sections[i] = std::make_shared<OrdinarySection>(
            shdrs[i], object + shdrs[i]->sh_offset);
        break;
      }
    }

//    assert(ehFrame != nullptr && relaEhFrame != nullptr);

    // TODO Handle rela after 3.4.
    if (debugStrOffShdr) {
      assert(debugStr != nullptr);
      debugStrOff = std::make_shared<DebugStrOffsetsSection>(debugStrOffShdr, debugStrOffData, *debugStr);
      sections[debugStrOffIndex] = debugStrOff;

      if (relaDebugStrOffsets){
        debugStrOff->applyRelocations(relaDebugStrOffsets->getRelocations());
      }
    }

    if (debugAddrShdr){
      debugAddr = std::make_shared<DebugAddrSection>(debugAddrShdr, debugAddrData);
      sections[debugAddrIndex] = debugAddr;

      if (relaDebugAddr){
        debugAddr->applyRelocations(relaDebugAddr->getRelocations());
      }
    }

    if (debugRnglistShdr) {
//      assert(debugAddr != nullptr);
      debugRnglist = std::make_shared<DebugRnglistSection>(debugRnglistShdr, debugRnglistData, debugAddr ? debugAddr.get() : nullptr);
      sections[debugRnglistIndex] = debugRnglist;
    }

    if (debugInfoShdr) {
      assert(debugAbbrev != nullptr);
      debugInfoSection = std::make_shared<DebugInfoSection>(
          debugInfoShdr, debugInfoData, *debugAbbrev, debugStr ? debugStr.get() : nullptr , debugStrOff ? debugStrOff.get() : nullptr, debugAddr ? debugAddr.get() : nullptr, debugRnglist ? debugRnglist.get() : nullptr);
      sections[debugInfoIndex] = debugInfoSection;
    }

    // 3. Parse references. (depend on 2)
    // 3.1 Parse section references.
    for (size_t i = 0; i < sections.size(); i++) {
      const auto section = sections[i];
      // Name offset -> strRef.
      section->setName(shstrTab->parseOriginalIndex(section->getShName()));
      // idx -> idxRef.
      section->setIdx(std::make_shared<IdxRef>(i));
    }
    // 3.2 Parse symbol references (depend on 3.1).
    symTab->parseReferences(sections, strTab);
    // 3.3 Parse section link, info references (depend on 3.1, 3.2).
    for (size_t i = 0; i < sections.size(); i++) {
      const auto section = sections[i];
      // link idx -> section idx.
      if (i != 0 && section->getShLink() != 0) {
        section->setLink(sections[section->getShLink()]->getIdx());
      }
      // Parse sh_info.
      // sh_info for rela. (we do not consider SHT_REL)
      if (section->getShType() == llvm::ELF::SHT_RELA) {
        section->setInfoLink(sections[section->getShInfo()]->getIdx());
      }
      // sh_info for group.
      if (section->getShType() == llvm::ELF::SHT_GROUP) {
        section->setInfoLink(symTab->getSymbol(section->getShInfo())->getIdx());
      }
    }
    // 3.4 Parse other references: relocation, group, eh_frame.
    // (depend on 3.1, 3.2).
    for (const auto &section : sections) {
      if (section->getType() == SectionType::RelaTab) {
        const auto relaSec = std::static_pointer_cast<RelocationSection>(section);
        relaSec->parseReferences(symTab);
      }
    }
    // Note that eh_frame depends on relocation.
    for (const auto &section : sections) {
      if (section->getType() == SectionType::Group) {
        const auto groupSec = std::static_pointer_cast<GroupSection>(section);
        groupSec->parseReferences(sections);
      } else if (section->getType() == SectionType::EhFrame) {
        const auto ehFrameSec = std::static_pointer_cast<EhFrameSection>(section);
        ehFrameSec->parseReferences(relaEhFrame);
      }
    }
  }

  std::shared_ptr<SymtabShndxSection> createSymtabShndx() {
    using Elf_Shdr = llvm::object::ELF64LE::Shdr;

    Elf_Shdr shdr;
    shdr.sh_name = 0;
    shdr.sh_type = llvm::ELF::SHT_SYMTAB_SHNDX;
    shdr.sh_flags = 0;
    shdr.sh_addr = 0;
    shdr.sh_offset = 0;
    shdr.sh_size = 0;
    shdr.sh_link = 0;
    shdr.sh_info = 0;
    shdr.sh_addralign = sizeof(llvm::object::ELF64LE::Word);
    shdr.sh_entsize = sizeof(llvm::object::ELF64LE::Word);

    // Create new section.
    const auto newSection =
      std::make_shared<SymtabShndxSection>(&shdr, nullptr);

    // Create new idx ref.
    newSection->setIdx(std::make_shared<IdxRef>(sections.size()));

    // Create new str ref.
    const std::string name = ".symtab_shndx";
    newSection->setName(shstrTab->push_back(name));

    // Update linkage.
    newSection->setLink(symTab->getIdx());

    sections.push_back(newSection);

    return newSection;
  }

  static std::size_t alignOffset(const std::uint64_t offset,
                                 const std::uint64_t sh_addralign) {
    const auto &logger = Logger::getInstance();

    if (sh_addralign == 0) {
      logger.fatal("sh_addralign cannot be zero.");
    }
    if (offset % sh_addralign == 0) {
      return offset;
    }
    return (offset + sh_addralign - 1) & ~(sh_addralign - 1);
  }

  void layout() {
    // Update idx and size.
    int idx = 0;
    for (const auto &section : sections) {
      section->getIdx()->setValue(idx++);
      section->layout();
    }

    // Update offset.
    uint64_t secOff = e_ehsize;
    // Layout SHF_ALLOC sections before non-SHF_ALLOC sections. A non-SHF_ALLOC
    // will not occupy file offsets contained by a PT_LOAD.
    for (size_t i = 1; i < sections.size(); i++) {
      const auto section = sections[i];
      if (!(section->getShFlags() & llvm::ELF::SHF_ALLOC)) {
        continue;
      }
      secOff = alignOffset(secOff, section->getShAddralign());
      section->setShOffset(secOff);
      if (section->getShType() != llvm::ELF::SHT_NOBITS) {
        secOff += section->getShSize();
      }
    }
    // Layout non-SHF_ALLOC sections.
    for (size_t i = 1; i < sections.size(); i++) {
      const auto section = sections[i];
      if (section->getShFlags() & llvm::ELF::SHF_ALLOC) {
        continue;
      }
      secOff = alignOffset(secOff, section->getShAddralign());
      section->setShOffset(secOff);
      if (section->getShType() != llvm::ELF::SHT_NOBITS) {
        secOff += section->getShSize();
      }
    }
    // Update section header table offset.
    secOff = alignOffset(secOff, Config::getInstance().wordSize);
    e_shoff = secOff;
  }

public:
  explicit ObjFile(const BinFile &_binFile) : binFile(_binFile) {}

  void init() {
    parseHeader();
    parseSections();
  }

  void fini() {
    // 1. Create necessary sections if needed.
    if (symTab->needSymtabShNdx() && symTab->getSymtabShndx() == nullptr) {
      symTab->setSymtabShndx(createSymtabShndx());
    }

    // 2. Layout.
    layout();

    // 3. Fini each section.
    for (const auto &section : sections) {
      section->fini();
      // Update name.
      section->setShName(section->getName()->getOffset());
      // Update link.
      const auto &link = section->getLink();
      if (section->getIdxValue() != 0 && link != nullptr) {
        section->setShLink(link->getValue());
      }
      // Update info.
      const auto &infoLink = section->getInfoLink();
      if (infoLink != nullptr) {
        section->setShInfo(infoLink->getValue());
      }
    }

    // update e_shnum
    e_shnum = sections.size();

    // Note: Update the first section.
    // The ELF header can only store numbers up to SHN_LORESERVE in the e_shnum
    // and e_shstrndx fields. When the value of one of these fields exceeds
    // SHN_LORESERVE ELF requires us to put sentinel values in the ELF header
    // and use fields in the section header at index 0 to store the value. The
    // sentinel values and fields are: e_shnum = 0, SHdrs[0].sh_size = number of
    // sections. e_shstrndx = SHN_XINDEX, SHdrs[0].sh_link = .shstrtab section
    // index.
    if (e_shnum >= llvm::ELF::SHN_LORESERVE) {
      sections[0]->setShSize(e_shnum);
      e_shnum = 0;
    }
    if (e_shstrndx >= llvm::ELF::SHN_LORESERVE) {
      sections[0]->setShLink(e_shstrndx);
      e_shstrndx = llvm::ELF::SHN_XINDEX;
    }
  }

  void save(const std::string &outputPath) const {
    const auto &logger = Logger::getInstance();

    // Note that e_shnum may be set to 0, here we should use sections.size().
    const uint64_t fileSize = e_shoff + sections.size() * e_shentsize;
    auto *buffer = new char[fileSize];
    memset(buffer, 0, fileSize);

    // 1. Write ELF header.
    using Elf_Ehdr = llvm::object::ELF64LE::Ehdr;
    auto *ehdr = reinterpret_cast<Elf_Ehdr *>(buffer);

    for (size_t i = 0; i < llvm::ELF::EI_NIDENT; i++) {
      ehdr->e_ident[i] = e_ident[i];
    }
    ehdr->e_type = e_type;
    ehdr->e_machine = e_machine;
    ehdr->e_version = e_version;
    ehdr->e_entry = e_entry;
    ehdr->e_phoff = e_phoff;
    ehdr->e_shoff = e_shoff;
    ehdr->e_flags = e_flags;
    ehdr->e_ehsize = e_ehsize;
    ehdr->e_phentsize = e_phentsize;
    ehdr->e_phnum = e_phnum;
    ehdr->e_shentsize = e_shentsize;
    ehdr->e_shnum = e_shnum;
    ehdr->e_shstrndx = e_shstrndx;

    // 2. Write Section data.
    // skip section 0 (NULL).
    for (size_t i = 1; i < sections.size(); i++) {
      const auto section = sections[i];
      if (section->getShType() != llvm::ELF::SHT_NOBITS) {
        section->writeDataTo(buffer + section->getShOffset());
      }
    }

    // 3. Write Section header table.
    using Elf_Shdr = llvm::object::ELF64LE::Shdr;
    auto *shdr = reinterpret_cast<Elf_Shdr *>(buffer + e_shoff);
    for (const auto &section : sections) {
      section->writeHeaderTo(shdr);
      ++shdr;
    }

    // 4. Write buffer.
    std::ofstream outputFile(outputPath, std::ios::binary);
    if (!outputFile) {
      logger.fatal("can not open " + outputPath);
    }
    outputFile.write(buffer, fileSize);
    outputFile.close();

    delete[] buffer;
  }

  void dump(std::ostream &oss) const {
    oss << "==================== Ehdr ====================\n";
    oss << "[Magic]";
    oss << std::hex;
    for (const unsigned char i : e_ident) {
      oss << " " << static_cast<int>(i);
    }
    oss << std::endl;
    oss << std::dec;
    oss << "[e_machine] " << TypeToString::elfMachineToString(e_machine)
        << "\n";
    oss << "[e_shoff] " << e_shoff << " (bytes into file)\n";
    oss << "[e_ehsize] " << e_ehsize << " (bytes)\n";
    oss << "[e_shentsize] " << e_shentsize << " (bytes)\n";
    oss << "[e_shnum] " << e_shnum << "(" << sections.size() << ")\n";
    oss << "[e_shstrndx] "
        << (shstrTab == nullptr ? -1 : shstrTab->getIdxValue()) << "\n";

    oss << "==================== Shdr ====================\n";

    oss << std::setfill(' ');
    oss << "[" << std::setw(5) << "Nr" << "] ";
    oss << std::setw(20) << "Name" << " ";
    oss << std::setw(20) << "Type" << " ";
    oss << std::setw(6) << "Off" << " ";
    oss << std::setw(6) << "Size" << " ";
    oss << std::setw(4) << "ES" << " ";
    oss << std::setw(4) << "Flg" << " ";
    oss << std::setw(5) << "Lk" << " ";
    oss << std::setw(4) << "Inf" << " ";
    oss << std::setw(4) << "Al";
    oss << "\n";

    for (const auto &section : sections) {
      section->dumpHeader(oss);
      oss << "\n";
    }

    oss << "==================== Sections' Data ====================\n";
    for (const auto &section : sections) {
      oss << "[" << section->getIdxValue() << "] " << section->getNameValue()
          << "\n";
      section->dumpData(oss);
      oss << "\n";
    }
  }

  std::string toString() const {
    std::stringstream oss;
    dump(oss);
    return oss.str();
  }

  std::shared_ptr<StringTableSection> &getStrTab() { return strTab; }

  std::shared_ptr<StringTableSection> &getShstrTab() { return shstrTab; }

  std::shared_ptr<SymbolTableSection> &getSymTab() { return symTab; }

  std::shared_ptr<EhFrameSection> &getEhFrame() { return ehFrame; }

  std::shared_ptr<RelocationSection> &getRelaEhFrame() { return relaEhFrame; }

  std::vector<std::shared_ptr<Section>> &getSections() { return sections; }
};

} // namespace elf
} // namespace funcv
} // namespace iclang

#endif // ICLANG_OBJFILE_HPP

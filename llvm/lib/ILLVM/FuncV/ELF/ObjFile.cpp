#include "illvm/FuncV/ELF/ObjFile.h"

#include "illvm/FuncV/ELF/NormalSection.h"

#include <iomanip>
#include <sstream>

#include "illvm/Support/Diagnostics.h"

namespace illvm {
namespace funcv {
namespace elf {

static bool getIs64(const uint16_t m) {
  return m == llvm::ELF::EM_AARCH64 || m == llvm::ELF::EM_X86_64;
}

void ObjFile::parseHeader() {
  const uint8_t *object = binFile.readBytes(0);
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

  ILLVM_FCHECK(getIs64(e_machine), "only support aarch64 / x86_64");
}

void ObjFile::parseNecessarySections(
    const uint8_t *object, const std::vector<const Elf_Shdr *> &shdrs) {
  for (size_t i = 0; i < shdrs.size(); i++) {
    switch (shdrs[i]->sh_type) {
    case llvm::ELF::SHT_STRTAB: {
      ILLVM_FCHECK(sections[i].get() == nullptr, "multiple str tables");
      ILLVM_FCHECK(i == e_shstrndx, "do not support gcc .shstrtab");
      auto _strTabSec =
          StringTableSection::create(shdrs[i], i, object + shdrs[i]->sh_offset);
      strTabSec = _strTabSec.borrow();
      sections[i] = _strTabSec.moveTo<Section>();
      break;
    }
    case llvm::ELF::SHT_SYMTAB: {
      ILLVM_FCHECK(sections[i].get() == nullptr, "multiple symbol tables");
      auto _symTabSec =
          SymbolTableSection::create(shdrs[i], i, object + shdrs[i]->sh_offset);
      symTabSec = _symTabSec.borrow();
      sections[i] = _symTabSec.moveTo<Section>();
      break;
    }
    default:
      break;
    }
  }

  ILLVM_FCHECK(strTabSec.get() != nullptr, "missing str table");
  ILLVM_FCHECK(symTabSec.get() != nullptr, "missing symbol table");
}

void ObjFile::parseOtherSections(const uint8_t *object,
                                 const std::vector<const Elf_Shdr *> &shdrs) {
  for (size_t i = 0; i < sections.size(); i++) {
    const auto secName = strTabSec->getStr(shdrs[i]->sh_name)->getValue();

    if (secName == ".eh_frame") {
      ILLVM_FCHECK(sections[i].get() == nullptr, "multiple eh_frame sections");
      auto _ehFrameSec =
          EhFrameSection::create(shdrs[i], i, object + shdrs[i]->sh_offset);
      ehFrameSec = _ehFrameSec.borrow();
      sections[i] = _ehFrameSec.moveTo<Section>();
      continue;
    }

    switch (shdrs[i]->sh_type) {
    case llvm::ELF::SHT_STRTAB:
    case llvm::ELF::SHT_SYMTAB:
      continue;
    case llvm::ELF::SHT_SYMTAB_SHNDX: {
      ILLVM_FCHECK(sections[i].get() == nullptr,
                   "multiple symtab shndx sections");
      sections[i] =
          ExtSymTabSection::create(shdrs[i], i, object + shdrs[i]->sh_offset)
              .moveTo<Section>();
      break;
    }
    case llvm::ELF::SHT_RELA:
      sections[i] =
          RelocationSection::create(shdrs[i], i, object + shdrs[i]->sh_offset)
              .moveTo<Section>();
      break;
    case llvm::ELF::SHT_GROUP:
      sections[i] =
          GroupSection::create(shdrs[i], i, object + shdrs[i]->sh_offset)
              .moveTo<Section>();
      break;
    default:
      sections[i] =
          NormalSection::create(shdrs[i], i, object + shdrs[i]->sh_offset)
              .moveTo<Section>();
      break;
    }
  }
}

void ObjFile::parseSections() {
  ILLVM_FCHECK(e_shentsize == sizeof(Elf_Shdr), "");

  const uint8_t *object = binFile.readBytes(0);
  std::vector<const Elf_Shdr *> shdrs;
  if (e_shnum != 0) {
    shdrs.reserve(e_shnum);
  } else {
    shdrs.reserve(llvm::ELF::SHN_LORESERVE);
  }

  // 1. Parse shdrs.
  // 1.1. Add the first shdr.
  const auto *firstShdr = reinterpret_cast<const Elf_Shdr *>(object + e_shoff);
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
    const auto *shdr =
        reinterpret_cast<const Elf_Shdr *>(object + e_shoff + i * e_shentsize);
    shdrs.push_back(shdr);
  }

  // 2. Parse shdrs to sections.
  sections.resize(e_shnum);
  // 2.1. Parse string table and symbol table.
  parseNecessarySections(object, shdrs);
  // 2.2. Parse other sections.
  parseOtherSections(object, shdrs);
}

llvm::Error ObjFile::initRef() {
  // 1. Init name ref.
  int extSymTabSecIdx = -1;
  for (size_t i = 0; i < sections.size(); i++) {
    auto &section = sections[i];
    // Name offset -> strRef.
    section->initNameRef(strTabSec->getStr(section->getShName()));
    if (section->getType() == SectionType::SymTabShNdx) {
      extSymTabSecIdx = i;
    }
  }

  // 2. Init symbol table ref.
  std::optional<OPtr<ExtSymTabSection>> extSymTabSection = std::nullopt;
  if (extSymTabSecIdx != -1) {
    extSymTabSection = sections[extSymTabSecIdx].moveTo<ExtSymTabSection>();
  }
  ILLVM_ETRANS(
      symTabSec->initRef(strTabSec, sections, std::move(extSymTabSection)));

  // 3. Init rela ref.
  for (size_t i = 0; i < sections.size(); i++) {
    if (sections[i].isReleased()) {
      continue;
    }
    auto section = sections[i].borrow();
    if (section->getType() != SectionType::RelaTab) {
      continue;
    }

    auto relaSection = section.copyTo<RelocationSection>();
    const uint32_t targetSecIdx = relaSection->getShInfo();
    ILLVM_FCHECK(targetSecIdx < sections.size() && !sections[targetSecIdx].isReleased(), "");
    auto targetSection = sections[targetSecIdx].borrow();

    relaSection->initRef(symTabSec.constCopy(), targetSection.constCopy());

    if (targetSection->getType() == SectionType::Normal) {
      auto normalTargetSec = targetSection.copyTo<NormalSection>();
      normalTargetSec->initRef(sections[i].moveTo<RelocationSection>());
    } else if (targetSection->getType() == SectionType::EhFrame) {
      auto ehFrameTargetSec = targetSection.copyTo<EhFrameSection>();
      ehFrameTargetSec->initRef(sections[i].moveTo<RelocationSection>());
    } else {
      ILLVM_ECHECK(false, "Invalid target section type of rela: " +
                              std::to_string(
                                  static_cast<int>(targetSection->getType())));
    }
  }

  // 4. Init other ref.
  for (size_t i = 0; i < sections.size(); i++) {
    if (sections[i].isReleased()) {
      continue;
    }
    auto section = sections[i].borrow();
    if (section->getType() == SectionType::Group) {
      auto groupSection = section.copyTo<GroupSection>();
      groupSection->initRef(sections, symTabSec.constCopy());
    }
  }

  // 5. Refactoring sections.
  std::vector<OPtr<Section>> tempSections;
  for (size_t i = 0; i < sections.size(); i++) {
    if (sections[i].isReleased()) {
      continue;
    }
    tempSections.emplace_back(std::move(sections[i]));
  }
  sections = std::move(tempSections);

  return llvm::Error::success();
}

ObjFile::ObjFile(const BinFile &_binFile, llvm::Error &err) : binFile(_binFile) {
  llvm::ErrorAsOutParameter EAO(&err);
  parseHeader();
  parseSections();
  if (auto err2 = initRef()) {
    err = std::move(err2);
    return;
  }
}

void ObjFile::layout() {
  // 1. Create necessary sections if needed.
  std::vector<OPtr<Section>> tempSections;
  for (size_t i = 0; i < sections.size(); i++) {
    auto section = sections[i].borrow();
    if (section->getType() == SectionType::Normal) {
      auto normalSec = section.copyTo<NormalSection>();
      auto relaSec = normalSec->extractRelaSection();
      if (relaSec.has_value()) {
        tempSections.emplace_back(relaSec.value().moveTo<Section>());
      }
    } else if (section->getType() == SectionType::EhFrame) {
      auto _ehFrameSec = section.copyTo<EhFrameSection>();
      auto relaSec = _ehFrameSec->extractRelaSection();
      if (relaSec.has_value()) {
        tempSections.emplace_back(relaSec.value().moveTo<Section>());
      }
    } else if (section->getType() == SectionType::SymTab) {
      auto _symTabSec = section.copyTo<SymbolTableSection>();
      auto extSymTabSec = _symTabSec->extractExtSymTabSection(strTabSec);
      if (extSymTabSec.has_value()) {
        tempSections.emplace_back(extSymTabSec.value().moveTo<Section>());
      }
    }
    tempSections.emplace_back(std::move(sections[i]));
  }
  sections = std::move(tempSections);

  // 2. Layout.
  for (size_t i = 0; i < sections.size(); i++) {
    sections[i]->layoutIdx(i);
    sections[i]->layout();
  }
  uint64_t secOff = e_ehsize;
  // Layout SHF_ALLOC sections before non-SHF_ALLOC sections. A non-SHF_ALLOC
  // will not occupy file offsets contained by a PT_LOAD.
  for (size_t i = 1; i < sections.size(); i++) {
    auto section = sections[i].borrow();
    if (!section->isAllocable()) {
      continue;
    }
    secOff = section->layoutOffset(secOff);
  }
  // Layout non-SHF_ALLOC sections.
  for (size_t i = 1; i < sections.size(); i++) {
    auto section = sections[i].borrow();
    if (section->isAllocable()) {
      continue;
    }
    secOff = section->layoutOffset(secOff);
  }

  // 3. Update ELF header.
  // Update section header table offset.
  secOff = MemoryUtils::alignOffset(secOff, sizeof(Elf_Word));
  e_shoff = secOff;
  // Note: Update the first section.
  // The ELF header can only store numbers up to SHN_LORESERVE in the e_shnum
  // and e_shstrndx fields. When the value of one of these fields exceeds
  // SHN_LORESERVE ELF requires us to put sentinel values in the ELF header
  // and use fields in the section header at index 0 to store the value. The
  // sentinel values and fields are: e_shnum = 0, SHdrs[0].sh_size = number of
  // sections. e_shstrndx = SHN_XINDEX, SHdrs[0].sh_link = .shstrtab section
  // index.
  e_shnum = sections.size();
  e_shstrndx = strTabSec->getIdx()->getValue();
  if (e_shnum >= llvm::ELF::SHN_LORESERVE) {
    sections[0]->layoutFirstNullSecSize(e_shnum);
    e_shnum = 0;
  }
  if (e_shstrndx >= llvm::ELF::SHN_LORESERVE) {
    sections[0]->layoutFirstNullSecLink(e_shstrndx);
    e_shstrndx = llvm::ELF::SHN_XINDEX;
  }
}

void ObjFile::write(const std::string &outputPath) const {
  // Note that e_shnum may be set to 0, here we should use sections.size().
  const uint64_t fileSize = e_shoff + sections.size() * e_shentsize;
  auto *buffer = new uint8_t[fileSize];
  memset(buffer, 0, fileSize);

  // 1. Write ELF header.
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
  for (const auto &section : sections) {
    section->writeData(buffer);
  }

  // 3. Write Section header table.
  auto *shdr = reinterpret_cast<Elf_Shdr *>(buffer + e_shoff);
  for (const auto &section : sections) {
    section->writeHeader(shdr);
    ++shdr;
  }

  // 4. Write buffer.
  std::ofstream outputFile(outputPath, std::ios::binary);
  ILLVM_FCHECK(outputFile, "Can not open " + outputPath);
  outputFile.write(reinterpret_cast<char *>(buffer), fileSize);
  outputFile.close();

  delete[] buffer;
}

void ObjFile::dump(std::ostream &oss) const {
  oss << "==================== Ehdr ====================" << std::endl;
  oss << "[Magic]";
  oss << std::hex;
  for (const unsigned char i : e_ident) {
    oss << " " << static_cast<int>(i);
  }
  oss << std::endl;
  oss << std::dec;
  // http://www.uxsglobal.com/developers/gabi/latest/ch4.eheader.html
  oss << "[e_machine] " << e_machine << std::endl;
  oss << "[e_shoff] " << e_shoff << " (bytes into file)" << std::endl;
  oss << "[e_ehsize] " << e_ehsize << " (bytes)" << std::endl;
  oss << "[e_shentsize] " << e_shentsize << " (bytes)" << std::endl;
  oss << "[e_shnum] " << e_shnum << "(" << sections.size() << ")" << std::endl;
  oss << "[e_shstrndx] " << strTabSec->getIdx()->getValue() << std::endl;

  oss << "==================== Shdr ====================" << std::endl;

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
  oss << std::endl;

  for (const auto &section : sections) {
    section->dumpHeader(oss);
    oss << std::endl;
  }

  oss << "==================== Sections' Data ===================="
      << std::endl;
  for (const auto &section : sections) {
    oss << "[" << section->getIdx()->getValue() << "] " << section->getName()
        << std::endl;
    section->dumpData(oss);
    oss << std::endl;
  }
}

std::string ObjFile::toString() const {
  std::stringstream oss;
  dump(oss);
  return oss.str();
}

} // namespace elf
} // namespace funcv
} // namespace illvm

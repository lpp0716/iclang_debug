#include "illvm/FuncV/ELF/GroupSection.h"

#include <iomanip>
#include <sstream>

#include "illvm/Support/Diagnostics.h"

namespace illvm {
namespace funcv {
namespace elf {

GroupSection::GroupSection(const Elf_Shdr *shdr, const uint8_t _idx,
                           const uint8_t *data)
    : Section(SectionType::Group, shdr, _idx) {
  ILLVM_FCHECK(shdr->sh_entsize == sizeof(uint32_t), "");
  ILLVM_FCHECK(shdr->sh_size % shdr->sh_entsize == 0, "");

  const size_t num = sh_size / sh_entsize;

  const auto *_secIdxes = reinterpret_cast<const uint32_t *>(data);
  // [0]: GRP_COMDAT
  for (size_t i = 1; i < num; i++) {
    forDataRef.secIdxes.push_back(_secIdxes[i]);
  }
}

void GroupSection::initRef(
    const std::vector<OPtr<Section>> &sections,
    const BPtr<const SymbolTableSection> &symTab) {
  link = symTab->getIdx();
  const auto &symbols = symTab->getSymbols();
  ILLVM_FCHECK(forHeaderRef.sh_info < symbols.size(), "");
  info = symbols[forHeaderRef.sh_info]->getIdx();

  groupSections.resize(forDataRef.secIdxes.size());
  for (size_t i = 0; i < forDataRef.secIdxes.size(); i++) {
    ILLVM_FCHECK(forDataRef.secIdxes[i] < sections.size() &&
                     !sections[forDataRef.secIdxes[i]].isReleased(),
                 "");
    groupSections[i] = sections[forDataRef.secIdxes[i]].constBorrow();
  }
}

void GroupSection::layout() {
  sh_size = groupSections.size() * sh_entsize + sh_entsize;
}

void GroupSection::writeDataImpl(uint8_t *buffer) const {
  uint32_t comdatFlag = llvm::ELF::GRP_COMDAT;
  MemoryUtils::write(buffer, &comdatFlag, sizeof(uint32_t));
  for (const auto &sec : groupSections) {
    uint32_t secIdx = sec->getIdx()->getValue();
    MemoryUtils::write(buffer, &secIdx, sizeof(uint32_t));
  }
}

void GroupSection::dumpData(std::ostream &oss) const {
  for (const auto &sec : groupSections) {
    oss << "section: " << sec->getIdx()->getValue() << " " << sec->getName()
        << std::endl;
  }
}

} // namespace elf
} // namespace funcv
} // namespace illvm

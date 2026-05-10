#include "illvm/FuncV/ELF/RelocationSection.h"

#include "illvm/Support/Diagnostics.h"

#include <iomanip>
#include <sstream>

#include "illvm/Support/Logger.h"

#include "llvm/Support/FormatVariadic.h"

namespace illvm {
namespace funcv {
namespace elf {

RelocationSection::RelocationSection(const Elf_Shdr *shdr, const uint64_t _idx,
                                     const uint8_t *data)
    : RelocationSection(shdr, _idx) {
  ILLVM_FCHECK(shdr->sh_entsize == sizeof(Elf_Rela), "");
  ILLVM_FCHECK(shdr->sh_size % shdr->sh_entsize == 0, "");

  const size_t relocationNum = shdr->sh_size / shdr->sh_entsize;
  forDataRef.relocations.resize(relocationNum);

  for (size_t i = 0; i < relocationNum; i++) {
    const auto *rela =
        reinterpret_cast<const Elf_Rela *>(data + i * shdr->sh_entsize);
    forDataRef.relocations[i] = make_owner<Relocation>(rela);
  }
}

void RelocationSection::layout() {
  sh_size = relocations.size() * sh_entsize;
}

void RelocationSection::writeDataImpl(uint8_t *buffer) const {
  for (const auto &relocation : relocations) {
    auto *rela = reinterpret_cast<Elf_Rela *>(buffer);
    relocation->write(rela);
    buffer += sh_entsize;
  }
}

void RelocationSection::dumpData(std::ostream &oss) const {
  oss << std::setfill(' ');
  oss << std::setw(16) << "Offset" << " ";
  oss << std::setw(16) << "Info" << " ";
  oss << std::setw(20) << "Type" << " ";
  oss << std::setw(20) << "SymbolName" << " ";
  oss << "Addend" << " ";
  oss << std::endl;

  for (const auto &relocation : relocations) {
    relocation->dump(oss);
    oss << std::endl;
  }
}

} // namespace elf
} // namespace funcv
} // namespace illvm

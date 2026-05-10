#include "illvm/FuncV/ELF/Section.h"

#include <iomanip>
#include <sstream>

#include "llvm/Support/FormatVariadic.h"

#include "illvm/Support/Diagnostics.h"

namespace illvm {
namespace funcv {
namespace elf {

uint64_t Section::layoutOffset(const uint64_t baseOffset) {
  sh_offset = MemoryUtils::alignOffset(baseOffset, sh_addralign);

  if (sh_type != llvm::ELF::SHT_NOBITS && sh_type != llvm::ELF::SHT_NULL) {
    return sh_offset + sh_size;
  }
  return sh_offset;
}

void Section::writeHeader(Elf_Shdr *shdr) const {
  shdr->sh_name = name->getOffset();
  shdr->sh_type = sh_type;
  shdr->sh_flags = sh_flags;
  shdr->sh_addr = sh_addr;
  shdr->sh_offset = sh_offset;
  shdr->sh_size = sh_size;
  shdr->sh_link = std::holds_alternative<uint64_t>(link)
                      ? std::get<uint64_t>(link)
                      : std::get<BPtr<const IntRef>>(link)->getValue();
  shdr->sh_info = std::holds_alternative<uint64_t>(info)
                      ? std::get<uint64_t>(info)
                      : std::get<BPtr<const IntRef>>(info)->getValue();
  shdr->sh_addralign = sh_addralign;
  shdr->sh_entsize = sh_entsize;
}

void Section::dumpHeader(std::ostream &oss) const {
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
  oss << std::setw(5)
      << (std::holds_alternative<uint64_t>(link)
              ? std::get<uint64_t>(link)
              : std::get<BPtr<const IntRef>>(link)->getValue())
      << " ";
  oss << std::setw(4)
      << (std::holds_alternative<uint64_t>(info)
              ? std::get<uint64_t>(info)
              : std::get<BPtr<const IntRef>>(info)->getValue())
      << " ";
  oss << std::setw(4) << sh_addralign;
}

std::string Section::headerToString() const {
  std::stringstream oss;
  dumpHeader(oss);
  return oss.str();
}

std::string Section::dataToString() const {
  std::stringstream oss;
  dumpData(oss);
  return oss.str();
}

} // namespace elf
} // namespace funcv
} // namespace illvm

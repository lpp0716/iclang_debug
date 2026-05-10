#include "illvm/FuncV/ELF/Relocation.h"

#include "illvm/Support/Diagnostics.h"

#include <iomanip>
#include <sstream>

namespace illvm {
namespace funcv {
namespace elf {

void Relocation::initRef(const BPtr<const SymbolTableSection> &symTabSec) {
  const auto &symbols = symTabSec->getSymbols();
  ILLVM_FCHECK(forRef.symInfo < symbols.size(), "");
  sym = symbols[forRef.symInfo].constBorrow();
}

void Relocation::write(Elf_Rela *rela) const {
  rela->r_offset = r_offset;
  rela->r_info = typeInfo;
  rela->r_info &= 0xffffffff;
  rela->r_info |= (sym->getIdx()->getValue() << 32);
  rela->r_addend = r_addend;
}

void Relocation::dump(std::ostream &oss) const {
  oss << std::hex;
  oss << std::setfill('0');
  oss << std::setw(16) << r_offset << " ";

  uint64_t r_info = typeInfo;
  r_info &= 0xffffffff;
  r_info |= (sym->getIdx()->getValue() << 32);
  oss << std::setw(16) << r_info << " ";

  oss << std::setfill(' ');
  oss << std::setw(20) << TypeToString::relaTypeToString(typeInfo)
      << " ";

  oss << std::setfill(' ');
  oss << std::setw(20) << sym->getName() << " ";

  if (r_addend < 0) {
    oss << "- " << -r_addend;
  } else {
    oss << "+ " << r_addend;
  }

  oss << std::dec;
}

std::string Relocation::toString() const {
  std::stringstream oss;
  dump(oss);
  return oss.str();
}

} // namespace elf
} // namespace funcv
} // namespace illvm

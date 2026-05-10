#include "illvm/FuncV/ELF/NormalSection.h"

namespace illvm {
namespace funcv {
namespace elf {

std::optional<OPtr<RelocationSection>> NormalSection::extractRelaSection() {
  if (!relaSection.has_value()) {
    return std::nullopt;
  }
  assert(relocations.has_value());
  std::vector<BPtr<const Relocation>> _relocations;
  _relocations.reserve(relocations->size());
  for (const auto &relocation : relocations.value()) {
    _relocations.emplace_back(relocation.constBorrow());
  }
  OPtr<RelocationSection> res = std::move(relaSection.value());
  relaSection = std::nullopt;
  res->initRef(std::move(_relocations));
  return res;
}

void NormalSection::writeDataImpl(uint8_t *buffer) const {
  if (sh_type == llvm::ELF::SHT_NOBITS || sh_type == llvm::ELF::SHT_NULL) {
    return;
  }
  MemoryUtils::write(buffer, data, sh_size);
}

} // namespace elf
} // namespace funcv
} // namespace illvm

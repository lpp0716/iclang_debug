#include "illvm/FuncV/ELF/EhFrame.h"

#include <iomanip>
#include <sstream>

namespace illvm {
namespace funcv {
namespace elf {

void FDE::write(uint8_t *&buffer) const {
  MemoryUtils::write(buffer, &length, sizeof(uint32_t));
  if (length == 0xffffffff) {
    MemoryUtils::write(buffer, &extLength, sizeof(uint64_t));
  }
  MemoryUtils::write(buffer, &ciePointer, sizeof(uint32_t));
  MemoryUtils::write(buffer, otherData,
                     length == 0xffffffff ? extLength - sizeof(uint32_t)
                                          : length - sizeof(uint32_t));
}

void FDE::dump(std::ostream &oss) const {
  oss << length << "(" << extLength << ") " << ciePointer << ": ";
  if (pcBeginRelaEntry.has_value()) {
    oss << pcBeginRelaEntry.value()->toString();
  } else {
    oss << "NULL";
  }
  oss << std::endl << "[otherRelaEntries]:" << std::endl;
  for (const auto &entry : otherRelaEntries) {
    entry->dump(oss);
    oss << std::endl;
  }
}

std::string FDE::toString() const {
  std::stringstream oss;
  dump(oss);
  return oss.str();
}

void CIE::write(uint8_t *&buffer) const {
  MemoryUtils::write(buffer, &length, sizeof(uint32_t));
  if (length == 0xffffffff) {
    MemoryUtils::write(buffer, &extLength, sizeof(uint64_t));
  }
  MemoryUtils::write(buffer, &cieID, sizeof(uint32_t));
  MemoryUtils::write(buffer, otherData,
                     length == 0xffffffff ? extLength - sizeof(uint32_t)
                                          : length - sizeof(uint32_t));
}

void CIE::dump(std::ostream &oss) const {
  oss << length << "(" << extLength << ")";
  oss << std::endl << "[relaEntries]:" << std::endl;
  for (const auto &entry : relaEntries) {
    entry->dump(oss);
    oss << std::endl;
  }
}

std::string CIE::toString() const {
  std::stringstream oss;
  dump(oss);
  return oss.str();
}

void CFI::write(uint8_t *&buffer) const {
  cie->write(buffer);
  for (const auto &fde : fdes) {
    fde->write(buffer);
  }
}

void CFI::dump(std::ostream &oss) const {
  oss << "CIE: " << cie->toString() << std::endl;
  for (size_t i = 0; i < fdes.size(); i++) {
    auto &fde = fdes[i];
    oss << "FDE" << i << ": " << fde->toString() << std::endl;
  }
}

std::string CFI::toString() const {
  std::stringstream oss;
  dump(oss);
  return oss.str();
}

} // namespace elf
} // namespace funcv
} // namespace illvm
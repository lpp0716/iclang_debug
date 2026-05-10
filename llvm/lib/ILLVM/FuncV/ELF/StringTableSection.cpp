#include "illvm/FuncV/ELF/StringTableSection.h"

#include <iomanip>
#include <sstream>

#include "illvm/Support/Diagnostics.h"

namespace illvm {
namespace funcv {
namespace elf {

StringTableSection::StringTableSection(const Elf_Shdr *shdr, const uint8_t _idx,
                                       const uint8_t *_data)
    : Section(SectionType::StrTab, shdr, _idx) {
  forDataRef.data = reinterpret_cast<const char *>(_data);
  auto &firstStrRef = strs.emplace_back(make_owner<StrRef>(forDataRef.data, 0));
  ILLVM_FCHECK(firstStrRef->getValue().empty(),
                  "The first entry of strtab is not empty");
  forDataRef.offsetStrMap[0] = firstStrRef.constBorrow();
  ILLVM_FCHECK(forDataRef.data[sh_size - 1] == '\0', "Broken strtab");
  for (uint64_t i = 0; i < sh_size; i++) {
    if (forDataRef.data[i] == '\0' && i + 1 < sh_size) {
      auto &strRef =
          strs.emplace_back(make_owner<StrRef>(forDataRef.data + i + 1, i + 1));
      forDataRef.offsetStrMap[i + 1] = strRef.constBorrow();
    }
  }
}

BPtr<const StrRef> StringTableSection::getStr(uint64_t strOff) {
  ILLVM_FCHECK(strOff < sh_size, "Invalid strtab offset");
  const auto it = forDataRef.offsetStrMap.find(strOff);
  if (it != forDataRef.offsetStrMap.end()) {
    return it->second.constCopy();
  }
  // Handle string overlap (compression).
  const auto &strRef =
      strs.emplace_back(make_owner<StrRef>(forDataRef.data + strOff, strOff));
  forDataRef.offsetStrMap[strOff] = strRef.constBorrow();
  return strRef.constBorrow();
}

BPtr<const StrRef> StringTableSection::addStr(const std::string &str) {
  const auto &strRef = strs.emplace_back(make_owner<StrRef>(str, 0));
  return strRef.constBorrow();
}

void StringTableSection::layout() {
  uint64_t strOff = 0;
  for (auto &strRef : strs) {
    strRef->setOffset(strOff);
    strOff += strRef->getLength() + 1;
  }
  sh_size = strOff;
}

void StringTableSection::writeDataImpl(uint8_t *buffer) const {
  for (const auto &strRef : strs) {
    MemoryUtils::write(buffer, strRef->getValue().data(),
                       strRef->getLength() + 1);
  }
}

void StringTableSection::dumpData(std::ostream &oss) const {
  for (size_t i = 0; i < strs.size(); i++) {
    oss << "[" << i << "] \"";
    strs[i]->dump(oss);
    oss << "\"" << std::endl;
  }
}

} // namespace elf
} // namespace funcv
} // namespace illvm

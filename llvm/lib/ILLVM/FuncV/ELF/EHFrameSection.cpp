#include "illvm/FuncV/ELF/EHFrameSection.h"

#include <iomanip>
#include <sstream>

#include "illvm/Support/Diagnostics.h"

namespace illvm {
namespace funcv {
namespace elf {

bool EhFrameSection::loadEntry(const uint8_t *&data, uint32_t &length,
                               uint64_t &extLength, uint32_t &entryFlag,
                               const uint8_t *&otherData) {
  uint64_t actLength = 0;
  length = 0;
  extLength = 0;
  MemoryUtils::read(&length, data, sizeof(uint32_t));

  if (length == 0) {
    return false;
  }

  if (length == 0xffffffff) {
    MemoryUtils::read(&extLength, data, sizeof(uint64_t));
    actLength = extLength;
  } else {
    actLength = length;
  }

  MemoryUtils::read(&entryFlag, data, sizeof(uint32_t));
  actLength -= sizeof(uint32_t);

  otherData = data;
  data += actLength;

  return true;
}

void EhFrameSection::linkRelaToCIEFDE(
    std::vector<OPtr<Relocation>> &&relaEntries) {
  size_t relaIdx = 0;
  uint64_t offset = 0;

  const size_t totalNum = relaEntries.size();
  size_t usedNum = 0;

  auto handleCIE = [&relaEntries, &relaIdx,
                    &offset, &usedNum](OPtr<CIE> &cie) {
    std::vector<OPtr<Relocation>> tempRelaSlots;
    while (relaIdx < relaEntries.size() &&
           offset <= relaEntries[relaIdx]->getROffset() &&
           relaEntries[relaIdx]->getROffset() < offset + cie->getSize()) {
      tempRelaSlots.emplace_back(std::move(relaEntries[relaIdx]));
      relaIdx += 1;
    }

    usedNum += tempRelaSlots.size();
    cie->initRef(offset, std::move(tempRelaSlots));
  };

  auto handleFDE = [&relaEntries, &relaIdx,
                    &offset, &usedNum](OPtr<FDE> &fde) {
    std::optional<OPtr<Relocation>> pcBeginRelaSlot;
    std::vector<OPtr<Relocation>> otherRelaSlots;
    while (relaIdx < relaEntries.size() &&
           offset <= relaEntries[relaIdx]->getROffset() &&
           relaEntries[relaIdx]->getROffset() < offset + fde->getSize()) {
      if (relaEntries[relaIdx]->getROffset() ==
          offset + fde->getPCBeginPreFilling()) {
        // Handle PCBegin rela.
        pcBeginRelaSlot = std::move(relaEntries[relaIdx]);
      } else {
        // Handle other rela.
        otherRelaSlots.emplace_back(std::move(relaEntries[relaIdx]));
      }
      relaIdx += 1;
      usedNum += 1;
    }

    fde->initRef(offset, std::move(pcBeginRelaSlot), std::move(otherRelaSlots));
  };

  for (auto &cfi : cfis) {
    handleCIE(cfi->cie);
    offset += cfi->cie->getSize();
    for (auto &fde : cfi->fdes) {
      handleFDE(fde);
      offset += fde->getSize();
    }
  }

  ILLVM_FCHECK(usedNum == totalNum, "");
}

EhFrameSection::EhFrameSection(const Elf_Shdr *shdr, const uint64_t _idx,
                               const uint8_t *data)
    : Section(SectionType::EhFrame, shdr, _idx) {
  // ref:
  // https://refspecs.linuxfoundation.org/LSB_3.0.0/LSB-Core-generic/LSB-Core-generic/ehframechpt.html
  const uint8_t *dataEnd = data + sh_size;
  uint32_t length = 0;
  uint64_t extLength = 0;
  uint32_t entryFlag = 0;
  const uint8_t *otherData = nullptr;

  // Load the first CIE.
  if (!loadEntry(data, length, extLength, entryFlag, otherData)) {
    return;
  }

  ILLVM_FCHECK(entryFlag != 0, "");

  cfis.emplace_back(make_owner<CFI>(
      make_owner<CIE>(length, extLength, entryFlag, otherData)));
  while (data < dataEnd &&
         loadEntry(data, length, extLength, entryFlag, otherData)) {
    if (entryFlag == 0) {
      // Load CIE.
      cfis.emplace_back(make_owner<CFI>(
          make_owner<CIE>(length, extLength, entryFlag, otherData)));
    } else {
      // Load FDE.
      cfis.back()->fdes.emplace_back(
          make_owner<FDE>(length, extLength, entryFlag, otherData));
    }
  }

  ILLVM_FCHECK(data == dataEnd, "");
}

void EhFrameSection::initRef(OPtr<RelocationSection> &&_relaSection) {
  relaSection = std::move(_relaSection);

  std::vector<OPtr<Relocation>> relaEntries =
      relaSection.value()->extractRelocations();
  std::sort(relaEntries.begin(), relaEntries.end(),
            [](const OPtr<Relocation> &r1, const OPtr<Relocation> &r2) {
              return r1->getROffset() < r2->getROffset();
            });

  // Link rela to CIE/FDE.
  linkRelaToCIEFDE(std::move(relaEntries));
}

std::optional<OPtr<RelocationSection>> EhFrameSection::extractRelaSection() {
  if (!relaSection.has_value()) {
    return std::nullopt;
  }
  std::vector<BPtr<const Relocation>> _relocations;
  for (auto &cfi : cfis) {
    cfi->cie->getRelocations(_relocations);
    for (auto &fde : cfi->fdes) {
      fde->getRelocations(_relocations);
    }
  }
  OPtr<RelocationSection> res = std::move(relaSection.value());
  relaSection = std::nullopt;
  res->initRef(std::move(_relocations));
  return res;
}

void EhFrameSection::layout() {
  sh_size = 0;
  uint64_t offset = 0;

  for (auto &cfi : cfis) {
    cfi->cie->layout(offset);
    const uint64_t cieBaseOffset = offset;
    offset += cfi->cie->getSize();

    for (auto &fde : cfi->fdes) {
      fde->layout(cieBaseOffset, offset);
      offset += fde->getSize();
    }
  }

  sh_size = offset;
}

void EhFrameSection::writeDataImpl(uint8_t *buffer) const {
  for (const auto &cfi : cfis) {
    cfi->write(buffer);
  }
}

void EhFrameSection::dumpData(std::ostream &oss) const {
  for (size_t i = 0; i < cfis.size(); i++) {
    auto &cfi = cfis[i];
    oss << "CFI " << i << "==========" << std::endl;
    oss << cfi->toString();
  }
}

} // namespace elf
} // namespace funcv
} // namespace illvm

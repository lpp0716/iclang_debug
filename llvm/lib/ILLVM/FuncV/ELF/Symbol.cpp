#include "illvm/FuncV/ELF/Symbol.h"

#include <iomanip>
#include <sstream>

#include "illvm/Support/Diagnostics.h"

namespace illvm {
namespace funcv {
namespace elf {

llvm::Error
Symbol::initRef(BPtr<StringTableSection> &strTabSec,
                const std::vector<OPtr<Section>> &sections,
                const std::optional<std::vector<Elf_Word>> &indexes) {
  name = strTabSec->getStr(forRef.st_name);
  int64_t secIdx = -1;
  if (forRef.st_shndx == llvm::ELF::SHN_UNDEF ||
      (llvm::ELF::SHN_LORESERVE <= forRef.st_shndx &&
       forRef.st_shndx < llvm::ELF::SHN_XINDEX)) {
    shndx = forRef.st_shndx;
  } else if (forRef.st_shndx == llvm::ELF::SHN_XINDEX) {
    const auto _idx = idx->getValue();
    ILLVM_FCHECK(indexes.has_value() && _idx < indexes.value().size(), "");
    secIdx = indexes.value()[_idx];
  } else {
    secIdx = forRef.st_shndx;
  }
  if (secIdx >= 0) {
    ILLVM_FCHECK(static_cast<size_t>(secIdx) < sections.size() &&
                     !sections[secIdx].isReleased(),
                 "");
    auto &sec = sections[secIdx];
    ILLVM_ECHECK(sec->getType() == SectionType::Normal, "");
    shndx = sec.constBorrow();
  }
  return llvm::Error::success();
}

void Symbol::layout(const uint64_t _idx) { idx->setValue(_idx); }

std::optional<uint64_t> Symbol::getXSecIdx() const {
  if (std::holds_alternative<BPtr<const Section>>(shndx)) {
    const uint64_t secIdx =
        std::get<BPtr<const Section>>(shndx)->getIdx()->getValue();
    if (secIdx >= llvm::ELF::SHN_LORESERVE) {
      return secIdx;
    }
  }
  return std::nullopt;
}

void Symbol::write(Elf_Sym *sym) const {
  if (st_type == llvm::ELF::STT_SECTION) {
    sym->st_name = 0;
  } else {
    sym->st_name = name->getOffset();
  }
  sym->st_value = st_value;
  sym->st_size = st_size;
  sym->st_info = getStInfo();
  sym->st_other = st_other;
  if (std::holds_alternative<BPtr<const Section>>(shndx)) {
    const uint64_t secIdx =
        std::get<BPtr<const Section>>(shndx)->getIdx()->getValue();
    if (secIdx >= llvm::ELF::SHN_LORESERVE) {
      sym->st_shndx = llvm::ELF::SHN_XINDEX;
    } else {
      sym->st_shndx = secIdx;
    }
  } else {
    sym->st_shndx = std::get<uint64_t>(shndx);
  }
}

void Symbol::dump(std::ostream &oss) const {
  oss << std::setfill(' ');
  oss << "[" << std::setw(5) << idx->getValue() << "] ";

  oss << std::hex;
  oss << std::setfill('0');
  oss << std::setw(16) << st_value << " ";

  oss << std::dec;
  oss << std::setfill(' ');
  oss << std::setw(6) << st_size << " ";

  oss << std::setw(8) << TypeToString::symbolTypeToString(st_type) << " ";
  oss << std::setw(8) << TypeToString::symbolBindToString(st_bind) << " ";
  oss << std::setw(8) << TypeToString::symbolVisToString(st_other) << " ";

  if (std::holds_alternative<BPtr<const Section>>(shndx)) {
    oss << std::setw(8)
        << std::get<BPtr<const Section>>(shndx)->getIdx()->getValue()
        << " ";
  } else {
    oss << std::setw(8)
        << TypeToString::symbolShndxToString(std::get<uint64_t>(shndx)) << " ";
  }

  oss << std::setw(20) << name->getValue() << " ";
}

std::string Symbol::toString() const {
  std::stringstream oss;
  dump(oss);
  return oss.str();
}

} // namespace elf
} // namespace funcv
} // namespace illvm
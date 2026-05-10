#include "illvm/FuncV/ELF/Reference.h"

#include <iomanip>
#include <sstream>

namespace illvm {
namespace funcv {
namespace elf {

void StrRef::dump(std::ostream &oss) const {
  oss << value << "(" << offset << ")";
}

std::string StrRef::toString() const {
  std::stringstream oss;
  dump(oss);
  return oss.str();
}

} // namespace elf
} // namespace funcv
} // namespace illvm

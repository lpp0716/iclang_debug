#ifndef ICLANG_DEBUG_STR_SECTION_HPP
#define ICLANG_DEBUG_STR_SECTION_HPP

#include "iclang/FuncV/ELF/Section.hpp"

namespace iclang {
namespace funcv {
namespace elf {

class DebugStrSection final : public Section {
private:
  // Structure representing a string entry in .debug_str section
  struct StringEntry {
    uint64_t offset; // Offset within the section
    std::string str; // The string content
  };

  std::vector<StringEntry> strings; // List of string entries

public:
  // TODO fix: string overlap, according to debug str offset.
  DebugStrSection(const llvm::object::ELF64LE::Shdr *shdr, const char *_data)
      : Section(SectionType::DebugStr, shdr, _data) {
    const char *start = data;
    const char *end = data + sh_size;
    uint64_t offset = 0;

    while (start < end) {
      const char *s = start;
      size_t len = strlen(s);
      if (len == 0) {
        ++start;
        ++offset;
        continue;
      }
      strings.push_back({offset, std::string(s)});
      start += len + 1;
      offset += len + 1;
    }
  }

  void writeDataTo(char *buffer) override {
    std::vector<uint8_t> out;

    for (const auto &entry : strings) {
      for (char c : entry.str) {
        out.push_back(static_cast<uint8_t>(c));
      }
      out.push_back(0);
    }

    assert(out.size() <= sh_size &&
           "Rewritten .debug_str larger than original");
    memcpy(buffer, out.data(), out.size());
  }

  void dumpData(std::ostream &oss) const override {
    oss << ".debug_str contents:\n";
    for (const auto &entry : strings) {
      oss << "0x" << intToHex(entry.offset, 8) << ": \"" << entry.str << "\"\n";
    }
  }

  // Get string by offset
  std::string getString(uint32_t offset) const {
    if (offset >= sh_size)
      return "<invalid offset>";
    return std::string(data + offset);
  }
};

} // namespace elf
} // namespace funcv
} // namespace iclang
#endif

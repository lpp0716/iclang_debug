#ifndef ICLANG_DEBUG_STR_OFFSETS_SECTION_HPP
#define ICLANG_DEBUG_STR_OFFSETS_SECTION_HPP

#include "iclang/FuncV/ELF/Section.hpp"
#include "iclang/FuncV/ELF/Debug/DebugStr.hpp"

namespace iclang {
namespace funcv {
namespace elf {

class DebugStrOffsetsSection final : public Section {
private:
  const DebugStrSection &debugStr;

  // TODO: block struct -> rela. Note: debug_info -> block ref.
  // Structure representing a string offsets table
  struct StringOffsetsTable {
    uint32_t unitLength;
    uint16_t version;
    uint16_t padding; // always 0
  };

  std::vector<StringOffsetsTable> tables;
  std::vector<uint64_t> offsetBases;
  std::vector<uint64_t> sizes;
  std::vector<uint64_t> headerSizes;
  std::vector<std::vector<uint32_t>> allOffsets;


public:
  DebugStrOffsetsSection(const llvm::object::ELF64LE::Shdr *shdr,
                         const char *_data, const DebugStrSection &strRef)
      : Section(SectionType::DebugStrOffsets, shdr, _data), debugStr(strRef) {
    // Ref 7.26
    const uint8_t *ptr = reinterpret_cast<const uint8_t *>(data);
    const uint8_t *end = ptr + sh_size;

    while (ptr < end) {
      const uint8_t *tableStart = ptr;
      if (end - ptr < 4)
        break;

      uint32_t unitLength = llvm::support::endian::read32le(ptr);
      ptr += 4;

      const uint8_t *tableEnd = ptr + unitLength;
      if (tableEnd > end)
        break;

      if (tableEnd - ptr < 4)
        break;

      uint16_t version = llvm::support::endian::read16le(ptr);
      ptr += 2;
      uint16_t padding = llvm::support::endian::read16le(ptr);
      ptr += 2;

      StringOffsetsTable table;
      table.unitLength = unitLength;
      table.version = version;
      table.padding = padding;
      tables.push_back(table);

      offsetBases.push_back(tableStart - reinterpret_cast<const uint8_t *>(data));
      sizes.push_back(unitLength + 4);
      headerSizes.push_back(8); // 4 (length) + 2 (version) + 2 (padding)

      std::vector<uint32_t> offsets;
      while (ptr + 4 <= tableEnd) {
        uint32_t offset = llvm::support::endian::read32le(ptr);
        offsets.push_back(offset);
        ptr += 4;
      }
      allOffsets.push_back(offsets);
    }
  }

  // Apply relocations to string offsets
  void
  applyRelocations(const std::vector<std::shared_ptr<Relocation>> &relocs) {
    for (const auto &rel : relocs) {
      uint64_t r_offset = rel->getROffset();

      for (size_t i = 0; i < tables.size(); ++i) {
        uint64_t tableStart = offsetBases[i] + headerSizes[i];
        uint64_t tableEnd = offsetBases[i] + sizes[i];

        if (r_offset < tableStart || r_offset >= tableEnd)
          continue;

        uint64_t offsetInTable = r_offset - tableStart;
        if (offsetInTable % 4 != 0) continue;

        size_t entryIndex = offsetInTable / 4;
        if (entryIndex >= allOffsets[i].size()) continue;

        allOffsets[i][entryIndex] = static_cast<uint32_t>(rel->getRAddend());
      }
    }
  }

  void writeDataTo(char *buffer) override {
    std::vector<uint8_t> out;

    for (size_t i = 0; i < tables.size(); ++i) {
      const auto &table = tables[i];
      const auto &offsets = allOffsets[i];
      size_t startOffset = out.size();
      // 1. Reserve space for unit_length (4 bytes)
      out.resize(out.size() + 4);

      // 2. Write header: version (2 bytes) + padding (2 bytes)
      out.push_back(table.version & 0xff);
      out.push_back((table.version >> 8) & 0xff);
      out.push_back(0); // padding byte 1
      out.push_back(0); // padding byte 2

      // 3. Write offsets, each is 4 bytes little endian
      for (uint32_t offset : offsets) {
        out.push_back(offset & 0xff);
        out.push_back((offset >> 8) & 0xff);
        out.push_back((offset >> 16) & 0xff);
        out.push_back((offset >> 24) & 0xff);
      }

      // 4. Fill in unit_length = total size minus length field
      uint32_t length = static_cast<uint32_t>(out.size() - startOffset - 4);
      out[startOffset + 0] = (length & 0xff);
      out[startOffset + 1] = (length >> 8) & 0xff;
      out[startOffset + 2] = (length >> 16) & 0xff;
      out[startOffset + 3] = (length >> 24) & 0xff;
    }

    // 5. Copy to target buffer
    assert(out.size() <= sh_size &&
           "Rewritten .debug_str_offset larger than original");
    memcpy(buffer, out.data(), out.size());
  }

  void dumpData(std::ostream &oss) const override {
    oss << ".debug_str_offsets contents:\n";
    for (size_t i = 0; i < tables.size(); ++i) {
      const auto &table = tables[i];
      const auto &offsets = allOffsets[i];

      oss << "0x" << std::hex << std::setw(8) << std::setfill('0')
          << offsetBases[i] << ": Contribution size = " << std::dec
          << sizes[i] << ", Format = DWARF32"
          << ", Version = " << table.version << "\n";

      for (size_t j = 0; j < offsets.size(); ++j) {
        uint64_t absOffset = offsetBases[i] + headerSizes[i] + j * 4;
        uint32_t strOffset = offsets[j];
        std::string str = debugStr.getString(strOffset);

        oss << "0x" << std::hex << std::setw(8) << std::setfill('0')
            << absOffset << ": " << std::setw(8) << std::setfill('0')
            << strOffset << " \"" << str << "\"\n";
      }
    }
  }

  // Get table index by base offset
  int getTableIndex(uint64_t baseOffset) const {
    for (size_t i = 0; i < offsetBases.size(); ++i) {
      if (offsetBases[i] + headerSizes[i] == baseOffset)
        return static_cast<int>(i);
    }
    return -1;
  }

  // Get string offset by table index and string index
  uint32_t getStringOffset(int tableIndex, uint32_t strxIndex) const {
    if (tableIndex < 0 || static_cast<size_t>(tableIndex) >= tables.size())
      return 0;

    if (strxIndex >= allOffsets[tableIndex].size())
      return 0;

    return allOffsets[tableIndex][strxIndex];
  }

  // Get string by table index and string index
  std::string getStringFromStrx(int tableIndex, uint32_t strxIndex) const {
    uint32_t offset = getStringOffset(tableIndex, strxIndex);
    return debugStr.getString(offset);
  }
};

} // namespace elf
} // namespace funcv
} // namespace iclang
#endif

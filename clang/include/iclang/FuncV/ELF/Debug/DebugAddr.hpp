#ifndef ICLANG_DEBUG_ADDR_SECTION_HPP
#define ICLANG_DEBUG_ADDR_SECTION_HPP

#include "iclang/FuncV/ELF/Section.hpp"

namespace iclang {
namespace funcv {
namespace elf {

class DebugAddrSection final : public Section {
private:
  // TODO: block struct -> rela. Note: debug_info -> block ref.
  // Structure representing an address table
  struct AddrTable {
    uint32_t unitLength;
    uint16_t version;      // DWARF version
    uint8_t addrSize;      // Address size in bytes
    uint8_t segSize;      // Segment selector size
  };

  std::vector<AddrTable> tables; // List of address tables
  std::vector<uint64_t> offsetBases;                  // 每个表的 offsetBase
  std::vector<uint64_t> sizes;                        // 每个表的 size（unitLength + 4）
  std::vector<uint64_t> headerSizes;                  // 每个表头部长度
  std::vector<std::vector<uint64_t>> allAddresses;    // 每个表的地址列表

public:

  DebugAddrSection(const llvm::object::ELF64LE::Shdr *shdr, const char *_data)
      : Section(SectionType::DebugAddr, shdr, _data) {
    // Ref: 7.27
    const uint8_t *ptr = reinterpret_cast<const uint8_t *>(data);
    const uint8_t *end = ptr + shdr->sh_size;

    while (ptr < end) {
      const uint8_t *tableStart = ptr;
      if (end - ptr < 8)
        break; // Invalid header

      const uint32_t unitLength = llvm::support::endian::read32le(ptr);
      ptr += 4;

      const uint8_t *tableEnd = ptr + unitLength;
      if (tableEnd > end) break;
      if (tableEnd - ptr < 4) break;

      AddrTable table;
      table.unitLength = unitLength;
      table.version = llvm::support::endian::read16le(ptr);
      ptr += 2;

      table.addrSize = *ptr++;
      table.segSize = *ptr++;
      tables.push_back(table);

      offsetBases.push_back(tableStart - reinterpret_cast<const uint8_t *>(data));
      sizes.push_back(unitLength + 4);
      headerSizes.push_back(4 + 2 + 1 + 1);

      std::vector<uint64_t> addresses;
      while (ptr + table.addrSize <= tableEnd) {
        uint64_t addr = 0;
        memcpy(&addr, ptr, table.addrSize);
        ptr += table.addrSize;
        addresses.push_back(addr);
      }
      allAddresses.push_back(std::move(addresses));
    }
  }

  // Apply relocations to address entries
  void applyRelocations(const std::vector<std::shared_ptr<Relocation>> &relocs) {
    for (const auto &rel : relocs) {
      uint64_t r_offset = rel->getROffset();

      for (size_t i = 0; i < tables.size(); ++i) {
        uint64_t tableStart = offsetBases[i] + headerSizes[i];
        uint64_t tableEnd = offsetBases[i] + sizes[i];

        if (r_offset < tableStart || r_offset >= tableEnd)
          continue;

        uint64_t offsetInTable = r_offset - tableStart;
        if (offsetInTable % tables[i].addrSize != 0)
          continue;

        size_t entryIndex = offsetInTable / tables[i].addrSize;
        if (entryIndex >= allAddresses[i].size())
          continue;

        allAddresses[i][entryIndex] = static_cast<uint64_t>(rel->getRAddend());
      }
    }
  }

  // Get address by table base offset and index
  uint64_t getAddressByIndex(uint64_t baseOffset, uint64_t index) const {
    for (size_t i = 0; i < tables.size(); ++i) {
      if (offsetBases[i] == baseOffset) {
        if (index < allAddresses[i].size())
          return allAddresses[i][index];
      }
    }
    return -1;
  }

  void writeDataTo(char *buffer) override {
    std::vector<uint8_t> out;

    for (size_t i = 0; i < tables.size(); ++i) {
      const auto &table = tables[i];
      const auto &addresses = allAddresses[i];
      size_t startOffset = out.size();

      // 1. Reserve space for unit_length (4 bytes)
      out.resize(out.size() + 4);

      // 2. Write header: version (2 bytes), addr_size (1), seg_size (1)
      out.push_back(table.version & 0xff);
      out.push_back((table.version >> 8) & 0xff);
      out.push_back(table.addrSize);
      out.push_back(table.segSize);

      // 3. Write address entries
      for (uint64_t addr : addresses) {
        if (table.addrSize == 4) {
          out.push_back(addr & 0xff);
          out.push_back((addr >> 8) & 0xff);
          out.push_back((addr >> 16) & 0xff);
          out.push_back((addr >> 24) & 0xff);
        } else if (table.addrSize == 8) {
          for (int i = 0; i < 8; ++i)
            out.push_back((addr >> (i * 8)) & 0xff);
        } else {
          assert(false && "Unsupported addr_size");
        }
      }

      // 4. Fill in unit_length (total size minus length field)
      uint32_t length = static_cast<uint32_t>(out.size() - startOffset - 4);
      out[startOffset + 0] = (length & 0xff);
      out[startOffset + 1] = (length >> 8) & 0xff;
      out[startOffset + 2] = (length >> 16) & 0xff;
      out[startOffset + 3] = (length >> 24) & 0xff;
    }

    // 5. Copy to target buffer
    assert(out.size() <= sh_size && "Rewritten .debug_addr larger than original");
    memcpy(buffer, out.data(), out.size());
  }

  void dumpData(std::ostream &oss) const override {
    for (size_t i = 0; i < tables.size(); ++i) {
      const auto &table = tables[i];
      const auto &addresses = allAddresses[i];
      oss << ".debug_addr contents:\n";

      oss << "Address table header: length = 0x"
          << std::hex << std::setw(8) << std::setfill('0') << table.unitLength
          << ", format = DWARF32, version = 0x"
          << std::setw(4) << table.version
          << ", addr_size = 0x"
          << std::setw(2) << static_cast<int>(table.addrSize)
          << ", seg_size = 0x"
          << std::setw(2) << static_cast<int>(table.segSize)
          << std::dec << "\n";

      oss << "Addrs: [\n";
      for (uint64_t addr : addresses) {
        if (table.addrSize == 4) {
          oss << "0x" << std::hex << std::setw(8) << std::setfill('0')
              << static_cast<uint32_t>(addr) << std::dec << "\n";
        } else if (table.addrSize == 8) {
          oss << "0x" << std::hex << std::setw(16) << std::setfill('0')
              << addr << std::dec << "\n";
        } else {
          oss << "# unsupported addr_size: " << static_cast<int>(table.addrSize) << "\n";
        }
      }
      oss << "]\n";
    }
  }
};

} // namespace elf
} // namespace funcv
} // namespace iclang
#endif

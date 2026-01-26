#ifndef ICLANG_DEBUG_RNG_SECTION_HPP
#define ICLANG_DEBUG_RNG_SECTION_HPP

#include "iclang/FuncV/ELF/Section.hpp"

namespace iclang {
namespace funcv {
namespace elf {

class DebugRnglistSection final : public Section {
private:
  // Ref : 7.28
  struct RnglistEntry {
    uint8_t kind;
    uint64_t value0;
    uint64_t value1;
  };

  struct RnglistHeader {
    uint32_t unit_length;
    uint16_t version;
    uint8_t addr_size;
    uint8_t seg_size;
    uint32_t offset_entry_count;
  };

  RnglistHeader header{};
  std::vector<uint32_t> offsets;
  std::map<uint32_t, std::vector<RnglistEntry>> rangeLists;
  //  const DebugAddrSection &debugAddr;
  const DebugAddrSection *debugAddr;
  uint64_t baseOffset;
  mutable std::unordered_map<uint32_t, uint64_t> contextMap;
public:
  DebugRnglistSection(const llvm::object::ELF64LE::Shdr *shdr, const char *_data, const DebugAddrSection *addrRef)
      : Section(SectionType::DebugRnglists, shdr, _data), debugAddr(addrRef) {
    const uint8_t *p = reinterpret_cast<const uint8_t *>(data);
    const uint8_t *end = p + sh_size;

    header.unit_length = llvm::support::endian::read32le(p);
    p += 4;

    header.version = llvm::support::endian::read16le(p);
    p += 2;
    header.addr_size = *p++;
    header.seg_size = *p++;
    header.offset_entry_count = llvm::support::endian::read32le(p);
    p += 4;

    for (uint32_t i = 0; i < header.offset_entry_count; ++i) {
      uint32_t offset = llvm::support::endian::read32le(p);
      offsets.push_back(offset);
      p += 4;
    }

    uint32_t current_index = 0;

    while (p < end) {
      uint8_t kind = *p++;
      if (kind == 0x00) {
        rangeLists[current_index].push_back({kind, 0, 0});
        current_index++;
        continue;
      }

      RnglistEntry entry{kind, 0, 0};

      unsigned n;
      switch (kind) {
      case 0x01: // DW_RLE_base_addressx
        entry.value0 = decodeULEB128(p, &n);
        p += n;
        break;

      case 0x02: // DW_RLE_startx_endx
        entry.value0 = decodeULEB128(p, &n);
        p += n;
        entry.value1 = decodeULEB128(p, &n);
        p += n;
        break;

      case 0x03: // DW_RLE_startx_length
        entry.value0 = decodeULEB128(p, &n);
        p += n;
        entry.value1 = decodeULEB128(p, &n);
        p += n;
        break;

      case 0x04: // DW_RLE_offset_pair
        entry.value0 = decodeULEB128(p, &n);
        p += n;
        entry.value1 = decodeULEB128(p, &n);
        p += n;
        break;

      case 0x05: // DW_RLE_base_address
        // addr_size bytes
        entry.value0 = llvm::support::endian::read64le(p); // assume addr_size = 8
        baseOffset = entry.value0;
        p += header.addr_size;
        break;

      case 0x06: // DW_RLE_start_end
        entry.value0 = llvm::support::endian::read64le(p); // assume addr_size = 8
        p += header.addr_size;
        entry.value1 = llvm::support::endian::read64le(p);
        p += header.addr_size;
        break;

      case 0x07: // DW_RLE_start_length
        entry.value0 = llvm::support::endian::read64le(p); // address
        p += header.addr_size;
        entry.value1 = decodeULEB128(p, &n);               // length
        p += n;
        break;

      default:
        std::cerr << "Unknown rnglist kind: 0x" << std::hex << (int)kind << "\n";
        return;
      }
      rangeLists[current_index].push_back(entry);
    }
  }

  void dumpData(std::ostream &oss) const override {
    oss << ".debug_rnglists contents:\n";
    oss << "range list header: "
        << "length = 0x" << std::hex << std::setw(8) << std::setfill('0') << header.unit_length
        << ", format = DWARF32"
        << ", version = 0x" << std::setw(4) << header.version
        << ", addr_size = 0x" << std::setw(2) << static_cast<int>(header.addr_size)
        << ", seg_size = 0x" << std::setw(2) << static_cast<int>(header.seg_size)
        << ", offset_entry_count = 0x" << std::setw(8) << header.offset_entry_count << "\n";

    if (!offsets.empty()){
      oss << "offsets: [\n";
      for (auto offset : offsets) {
        oss << "0x" << std::hex << std::setw(8) << std::setfill('0') << offset << "\n";
      }
      oss << "]\n";
    }

    oss << "ranges:\n";
    for (auto &rangeList : rangeLists) {
      uint64_t cuBaseOffset = 0;

      auto it = contextMap.find(rangeList.first);
      if (it != contextMap.end()) {
        cuBaseOffset = it->second;
      }
      for (const auto &entry : rangeList.second) {
        if (entry.kind == 0x00) {
          oss << "<End of list>\n";
          break;
        }
        switch (entry.kind) {
        case 0x01: {
          assert(debugAddr != nullptr);
          uint64_t baseAddr =
              debugAddr->getAddressByIndex(cuBaseOffset, entry.value0);
          oss << "[0x" << std::hex << std::setw(16) << std::setfill('0')
              << baseAddr << ")\n";
          break;
        }
        case 0x02: {
          assert(debugAddr != nullptr);
          uint64_t startAddr =
              debugAddr->getAddressByIndex(cuBaseOffset, entry.value0);
          uint64_t endAddr =
              debugAddr->getAddressByIndex(cuBaseOffset, entry.value1);
          oss << "[0x" << std::hex << std::setw(16) << std::setfill('0')
              << startAddr << ", 0x" << std::setw(16) << endAddr << ")\n";
          break;
        }
        case 0x03: {
          assert(debugAddr != nullptr);
          uint64_t startAddr =
              debugAddr->getAddressByIndex(cuBaseOffset, entry.value0);
          oss << "[0x" << std::hex << std::setw(16) << std::setfill('0')
              << startAddr << ", 0x" << std::setw(16) << entry.value1 << ")\n";
          break;
        }
        case 0x04:
        case 0x06:
        case 0x07: {
          oss << "[0x" << std::hex << std::setw(16) << std::setfill('0')
              << baseOffset + entry.value0 << ", 0x" << std::setw(16) << baseOffset + entry.value1
              << ")\n";
          break;
        }
        }
      }
    }
  }

  void registerAddrBase(uint32_t index, uint64_t addrBaseOffset) const {
    contextMap[index] = addrBaseOffset;
  }

  void writeDataTo(char *buffer) override {
    std::vector<uint8_t> out;

    // unit_length 占位（DWARF32）
    size_t unit_len_pos = out.size();
    out.resize(out.size() + 4, 0);

    // header: version, addr_size, seg_size, offset_entry_count
    uint8_t b2[2];
    llvm::support::endian::write16le(b2, header.version);
    out.insert(out.end(), b2, b2 + 2);

    out.push_back(static_cast<uint8_t>(header.addr_size));
    out.push_back(static_cast<uint8_t>(header.seg_size));

    uint8_t b4[4];
    llvm::support::endian::write32le(b4, header.offset_entry_count);
    out.insert(out.end(), b4, b4 + 4);

    // offsets 表
    for (uint32_t off : offsets) {
      llvm::support::endian::write32le(b4, off);
      out.insert(out.end(), b4, b4 + 4);
    }

    // entries（按解析结果逐条写回）
    for (auto &rangeList : rangeLists) {
      for (const auto &e : rangeList.second) {
        out.push_back(static_cast<uint8_t>(e.kind));

        if (e.kind == 0x00)  // DW_RLE_end_of_list
          break;

        switch (e.kind) {
        case 0x01:  // DW_RLE_base_addressx: index (ULEB)
          encodeULEB128(e.value0, out);
          break;

        case 0x02:  // DW_RLE_startx_endx: start_index(ULEB), end_index(ULEB)
          encodeULEB128(e.value0, out);
          encodeULEB128(e.value1, out);
          break;

        case 0x03:  // DW_RLE_startx_length: start_index(ULEB), length(ULEB)
          encodeULEB128(e.value0, out);
          encodeULEB128(e.value1, out);
          break;

        case 0x04:  // DW_RLE_offset_pair: start_off(ULEB), end_off(ULEB)
          encodeULEB128(e.value0, out);
          encodeULEB128(e.value1, out);
          break;

        case 0x05: { // DW_RLE_base_address: base_addr (addr_size bytes)
          uint64_t v = e.value0;
          for (uint8_t i = 0; i < header.addr_size; ++i)
            out.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFF));
          break;
        }

        case 0x06: { // DW_RLE_start_end: start_addr(addr_size), end_addr(addr_size)
          uint64_t a0 = e.value0, a1 = e.value1;
          for (uint8_t i = 0; i < header.addr_size; ++i)
            out.push_back(static_cast<uint8_t>((a0 >> (i * 8)) & 0xFF));
          for (uint8_t i = 0; i < header.addr_size; ++i)
            out.push_back(static_cast<uint8_t>((a1 >> (i * 8)) & 0xFF));
          break;
        }

        case 0x07: { // DW_RLE_start_length: start_addr(addr_size), length(ULEB)
          uint64_t a = e.value0;
          for (uint8_t i = 0; i < header.addr_size; ++i)
            out.push_back(static_cast<uint8_t>((a >> (i * 8)) & 0xFF));
          encodeULEB128(e.value1, out);
          break;
        }

        default:
          std::cerr << "Unknown rnglist kind when writing: 0x"
                    << std::hex << static_cast<int>(e.kind) << "\n";
          return;
        }
      }
    }


    // 回填 unit_length（不含自身 4 字节）
    llvm::support::endian::write32le(out.data() + unit_len_pos,
                                     static_cast<uint32_t>(out.size() - 4));

    assert(out.size() <= sh_size &&
           "Rewritten .debug_str_offset larger than original");
    memcpy(buffer, out.data(), out.size());
  }

};

} // namespace elf
} // namespace funcv
} // namespace iclang
#endif

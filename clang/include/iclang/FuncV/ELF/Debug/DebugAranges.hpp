#ifndef ICLANG_DEBUG_ARANGES_SECTION_HPP
#define ICLANG_DEBUG_ARANGES_SECTION_HPP

#include "iclang/FuncV/ELF/Section.hpp"

namespace iclang {
namespace funcv {
namespace elf {

class DebugArangeSection final : public Section {
private:
  struct ArangeEntry {
    uint64_t address;
    uint64_t length;
  };

  struct ArangeSet {
    uint32_t unit_length;
    uint16_t version;
    uint32_t debug_info_offset;
    uint8_t address_size;
    uint8_t segment_size;
    std::vector<ArangeEntry> ranges;
  };

  std::vector<ArangeSet> aranges;

public:
  DebugArangeSection(const llvm::object::ELF64LE::Shdr *shdr, const char *_data)
      : Section(SectionType::DebugAranges, shdr, _data) {
    const uint8_t *start = reinterpret_cast<const uint8_t *>(data);
    const uint8_t *end = start + sh_size;

    while (start < end) {
      ArangeSet set{};

      if (end - start < 4) break;
      set.unit_length = *reinterpret_cast<const uint32_t *>(start);
      start += 4;
      if (set.unit_length == 0) break; // 结束
      const uint8_t *set_end = start + set.unit_length;

      if (end - start < 2) break;
      set.version = *reinterpret_cast<const uint16_t *>(start);
      start += 2;

      if (end - start < 4) break;
      set.debug_info_offset = *reinterpret_cast<const uint32_t *>(start);
      start += 4;

      set.address_size = *start++;
      set.segment_size = *start++;

      // 对齐到 2*address_size 边界
      size_t header_size =
          4 + 2 + 4 + 1 + 1; // unit_length+version+debug_info_offset+addr_size+seg_size
      size_t padding = (2 * set.address_size) -
                       (header_size % (2 * set.address_size));
      if (padding != 2 * set.address_size) {
        start += padding;
      }

      // 解析地址范围 entries
      while (start < set_end) {
        uint64_t addr = 0, length = 0;

        if (set.address_size == 4) {
          addr = *reinterpret_cast<const uint32_t *>(start);
          start += 4;
          length = *reinterpret_cast<const uint32_t *>(start);
          start += 4;
        } else if (set.address_size == 8) {
          addr = *reinterpret_cast<const uint64_t *>(start);
          start += 8;
          length = *reinterpret_cast<const uint64_t *>(start);
          start += 8;
        } else {
          llvm::errs() << "Unsupported address_size in .debug_aranges";
        }

        if (addr == 0 && length == 0) break; // terminator
        set.ranges.push_back({addr, length});
      }

      aranges.push_back(std::move(set));
      start = set_end;
    }
  }

  void writeDataTo(char *buffer) override {
    std::vector<uint8_t> out;

    for (const auto &set : aranges) {
      // unit_length
      size_t headerStart = out.size();
      out.resize(out.size() + 4);
      // version
      out.insert(out.end(), reinterpret_cast<const uint8_t *>(&set.version),
                 reinterpret_cast<const uint8_t *>(&set.version) + 2);

      // debug_info_offset
      out.insert(out.end(),
                 reinterpret_cast<const uint8_t *>(&set.debug_info_offset),
                 reinterpret_cast<const uint8_t *>(&set.debug_info_offset) + 4);

      // addr_size, seg_size
      out.push_back(set.address_size);
      out.push_back(set.segment_size);

      // 对齐填充
      size_t header_size = 4 + 2 + 4 + 1 + 1;
      size_t padding = (2 * set.address_size) -
                       (header_size % (2 * set.address_size));
      if (padding != 2 * set.address_size) {
        out.insert(out.end(), padding, 0);
      }

      // ranges
      for (const auto &r : set.ranges) {
        if (set.address_size == 4) {
          uint32_t a = static_cast<uint32_t>(r.address);
          uint32_t l = static_cast<uint32_t>(r.length);
          out.insert(out.end(), reinterpret_cast<uint8_t *>(&a),
                     reinterpret_cast<uint8_t *>(&a) + 4);
          out.insert(out.end(), reinterpret_cast<uint8_t *>(&l),
                     reinterpret_cast<uint8_t *>(&l) + 4);
        } else {
          uint64_t a = r.address;
          uint64_t l = r.length;
          out.insert(out.end(), reinterpret_cast<uint8_t *>(&a),
                     reinterpret_cast<uint8_t *>(&a) + 8);
          out.insert(out.end(), reinterpret_cast<uint8_t *>(&l),
                     reinterpret_cast<uint8_t *>(&l) + 8);
        }
      }

      // terminator
      for (int i = 0; i < set.address_size * 2; i++)
        out.push_back(0);

      uint32_t length = static_cast<uint32_t>(out.size() - headerStart - 4);
      out[headerStart + 0] = (length & 0xff);
      out[headerStart + 1] = (length >> 8) & 0xff;
      out[headerStart + 2] = (length >> 16) & 0xff;
      out[headerStart + 3] = (length >> 24) & 0xff;
    }

    assert(out.size() <= sh_size &&
           "Rewritten .debug_aranges larger than original");
    memcpy(buffer, out.data(), out.size());
  }

  void dumpData(std::ostream &oss) const override {
    oss << ".debug_aranges contents:\n";
    for (size_t i = 0; i < aranges.size(); i++) {
      const auto &set = aranges[i];
      oss << "Arange Range Header:";
      oss << " length = " << set.unit_length << ", format = DWARF32";
      oss << ", version = 0x" << intToHex(set.version, 4);
      oss << ", cu_offset = 0x" << intToHex(set.debug_info_offset, 8);
      oss << ", addr_size = 0x" << intToHex(set.address_size, 2)
          << ", seg_size = 0x" << intToHex(set.segment_size, 2) << "\n";
      for (const auto &r : set.ranges) {
        oss << "    [0x" << intToHex(r.address, set.address_size * 2)
            << ", 0x" << intToHex(r.address + r.length, set.address_size * 2)
            << ")\n";
      }
    }
  }
};

} // namespace elf
} // namespace funcv
} // namespace iclang
#endif

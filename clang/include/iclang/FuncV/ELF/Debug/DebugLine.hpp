#ifndef ICLANG_DEBUG_LINE_SECTION_HPP
#define ICLANG_DEBUG_LINE_SECTION_HPP

#include "iclang/FuncV/ELF/Section.hpp"
#include "iclang/FuncV/ELF/Debug/DebugStrOffsets.hpp"

namespace iclang {
namespace funcv {
namespace elf {

class DebugLineSection final : public Section {
private:
  // Ref : 6.2.4
  struct LineTableHeader {
    uint32_t unit_length;
    uint16_t version;
    uint8_t address_size;
    uint8_t segment_selector_size;
    uint32_t header_length;
    uint8_t min_inst_length;
    uint8_t max_ops_per_inst;
    uint8_t default_is_stmt;
    int8_t line_base;
    uint8_t line_range;
    uint8_t opcode_base;
    std::vector<uint8_t> standard_opcode_lengths;
    uint8_t dir_format_count;
    std::vector<std::pair<uint64_t, uint64_t>> dir_attrs;
    uint64_t directory_count;
    std::vector<FormValueRaw> directories;
    uint8_t file_name_entry_format_count;
    std::vector<std::pair<uint64_t, uint64_t>> file_attrs;
    uint64_t file_names_count;
    struct FileEntry {
      std::string name;
      uint64_t dir_index;
      std::array<uint8_t, 16> md5; // For DWARFv5
      FormValueRaw val;
    };
    std::vector<FileEntry> file_names;
  };

  struct Row {
    uint64_t address = 0;
    int32_t line = 1;
    uint32_t column = 0;
    uint32_t file = 1;
    uint32_t isa = 0;
    uint32_t discriminator = 0;
    uint32_t op_index = 0;
    bool is_stmt;
    bool basic_block = false;
    bool end_sequence = false;
    bool prologue_end = false;
    bool epilogue_begin = false;
  };

  LineTableHeader header;
  std::vector<Row> rows;

public:
  DebugLineSection(const llvm::object::ELF64LE::Shdr *shdr, const char *_data)
      : Section(SectionType::DebugLine, shdr, _data) {
    const uint8_t *p = reinterpret_cast<const uint8_t *>(data);

    // header
    header.unit_length = llvm::support::endian::read32le(p);
    p += 4;
    const uint8_t *end = p + header.unit_length;

    header.version = llvm::support::endian::read16le(p);
    p += 2;
    header.address_size = *p++;
    header.segment_selector_size = *p++;
    header.header_length = llvm::support::endian::read32le(p);
    p += 4;

    header.min_inst_length = *p++;
    header.max_ops_per_inst = *p++;
    header.default_is_stmt = *p++;
    header.line_base = *p++;
    header.line_range = *p++;
    header.opcode_base = *p++;

    header.standard_opcode_lengths.resize(header.opcode_base - 1);
    for (uint8_t &len : header.standard_opcode_lengths)
      len = *p++;

    // --- DWARFv5 include_directories_table ---
    unsigned n;
    header.dir_format_count = decodeULEB128(p, &n);
    p += n;
    for (uint64_t i = 0; i < header.dir_format_count; ++i) {
      uint64_t content_type = decodeULEB128(p, &n);
      p += n;
      uint64_t form = decodeULEB128(p, &n);
      p += n;
      header.dir_attrs.push_back(std::make_pair(content_type, form));
    }

    header.directory_count = decodeULEB128(p, &n);
    p += n;
    for (uint64_t i = 0; i < header.directory_count; ++i) {
      FormValueRaw value;
      for (const auto &[type, form] : header.dir_attrs) {
        if (type == 1) { // DW_LNCT_path
          value = parseFormValue(form, p, end);
        }
        else skipFormValue(form, p, end);
      }
      header.directories.push_back(value);
    }

    header.file_name_entry_format_count = decodeULEB128(p, &n);
    p += n;
    for (uint64_t i = 0; i < header.file_name_entry_format_count; ++i) {
      uint64_t content_type = decodeULEB128(p, &n);
      p += n;
      uint64_t form = decodeULEB128(p, &n);
      p += n;
      header.file_attrs.emplace_back(content_type, form);
    }

    header.file_names_count = decodeULEB128(p, &n);
    p += n;
    for (uint64_t i = 0; i < header.file_names_count; ++i) {
      LineTableHeader::FileEntry entry;
      for (const auto &[type, form] : header.file_attrs) {
        if (type == 1) { // DW_LNCT_path
          entry.val = parseFormValue(form, p, end);
          entry.name = entry.val.str;
        } else if (type == 2) { // DW_LNCT_directory_index
          entry.dir_index = parseFormValue(form, p, end).value;
        } else if (type == 5) { // DW_LNCT_md5
          for (int j = 0; j < 16; ++j)
            entry.md5[j] = *p++;
        } else skipFormValue(form, p, end);
      }
      header.file_names.push_back(entry);
    }

    // line table program
    Row state;
    state.is_stmt = header.default_is_stmt;

    while (p < end) {
      uint8_t opcode = *p++;
      if (opcode >= header.opcode_base) {
        uint8_t adj_opcode = opcode - header.opcode_base;
        uint64_t addr_inc = (adj_opcode / header.line_range) * header.min_inst_length;
        int64_t line_inc = header.line_base + (adj_opcode % header.line_range);
        state.address += addr_inc;
        state.line += line_inc;
        rows.push_back(state);
        state.basic_block = false;
        state.epilogue_begin = false;
        state.prologue_end = false;
        state.discriminator = 0;
        continue;
      }
      if (opcode == 0) {
        unsigned n;
        uint64_t len = decodeULEB128(p, &n); p += n;
        const uint8_t *ext_end = p + len;
        uint8_t sub = *p++;
        switch (sub) {
        case 1: // DW_LNE_end_sequence
          state.end_sequence = true;
          rows.push_back(state);
          state = {};
          state.line = 1;
          state.is_stmt = header.default_is_stmt;
          break;
        case 2: // DW_LNE_set_address
          if (header.address_size == 8)
            state.address = llvm::support::endian::read64le(p);
          else state.address = llvm::support::endian::read32le(p);
          p += header.address_size;
          break;
        case 3: // DW_LNE_set_prologue_end
          state.prologue_end = true;
          break;
        case 4: { // DW_LNE_set_discriminator
          uint64_t discrim = decodeULEB128(p, &n);
          p += n;
          state.discriminator = discrim;
          break;
        }
        default: break;
        }
        p = ext_end;
      } else {
        unsigned n;
        switch (opcode) {
        case 1:
          rows.push_back(state);
          state.basic_block = false;
          state.prologue_end = false;
          state.epilogue_begin = false;
          state.discriminator = 0;
          break; // DW_LNS_copy
        case 2: state.address += decodeULEB128(p, &n) * header.min_inst_length; p += n; break;
        case 3: state.line += decodeSLEB128(p, &n); p += n; break;
        case 4: state.file = decodeULEB128(p, &n); p += n; break;
        case 5: state.column = decodeULEB128(p, &n); p += n; break;
        case 6: state.is_stmt = !state.is_stmt; break;
        case 7: state.basic_block = true; break;
        case 8: { // DW_LNS_const_add_pc
          uint8_t adjusted = 255 - header.opcode_base;
          uint64_t addr_inc = (adjusted / header.line_range) * header.min_inst_length;
          state.address += addr_inc;
          break;
        }
        case 9: { // DW_LNS_fixed_advance_pc
          uint16_t advance = llvm::support::endian::read16le(p); p += 2;
          state.address += advance;
          break;
        }
        case 10: // DW_LNS_set_prologue_end
          state.prologue_end = true;
          break;
        case 11: // DW_LNS_set_epilogue_begin
          state.epilogue_begin = true;
          break;
        case 12: // DW_LNS_set_isa
          state.isa = decodeULEB128(p, &n); p += n;
          break;
        default:
          // Handle unknown opcode: skip operands
          if (opcode < header.standard_opcode_lengths.size() + 1) {
            for (uint8_t i = 0; i < header.standard_opcode_lengths[opcode - 1]; ++i)
              decodeULEB128(p, &n), p += n;
          }
          break;
        }
      }
    }
  }

  void dumpData(std::ostream &oss) const override {
    oss << ".debug_line contents:\n";
    oss << "debug_line[0x00000000]\n";
    oss << "Line table prologue:\n";
    oss << "    total_length: 0x" << std::hex << std::setw(8) << std::setfill('0') << header.unit_length << "\n";
    oss << "          format: DWARF32\n";
    oss << "         version: " << std::dec << header.version << "\n";
    oss << "    address_size: " << static_cast<int>(header.address_size) << "\n";
    oss << " seg_select_size: " << static_cast<int>(header.segment_selector_size) << "\n";
    oss << " prologue_length: 0x" << std::hex << std::setw(8) << std::setfill('0') << header.header_length << "\n";
    oss << " min_inst_length: " << std::dec << static_cast<int>(header.min_inst_length) << "\n";
    oss << "max_ops_per_inst: " << static_cast<int>(header.max_ops_per_inst) << "\n";
    oss << " default_is_stmt: " << static_cast<int>(header.default_is_stmt) << "\n";
    oss << "       line_base: " << static_cast<int>(header.line_base) << "\n";
    oss << "      line_range: " << static_cast<int>(header.line_range) << "\n";
    oss << "     opcode_base: " << static_cast<int>(header.opcode_base) << "\n";

    for (size_t i = 0; i < header.standard_opcode_lengths.size(); ++i)
      oss << "standard_opcode_lengths[DW_LNS_" << opcodeName(i+1) << "] = "
          << static_cast<int>(header.standard_opcode_lengths[i]) << "\n";

    for (size_t i = 0; i < header.directories.size(); ++i)
      oss << "include_directories[" << std::setw(3) << i << "] = \"" << std::hex
          << header.directories[i].value << "\"\n";

    for (size_t i = 0; i < header.file_names.size(); ++i) {
      const auto &f = header.file_names[i];
      oss << "file_names[" << std::setw(3) << i << "]:\n";
      oss << "           name: \"" << f.name << "\"\n";
      oss << "      dir_index: " << f.dir_index << "\n";
      if (header.file_names.size() <= 1)
        oss << "   md5_checksum: ";
      for (uint8_t b : f.md5)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
      oss << std::dec << "\n";
    }

    oss << "\nAddress             Line    Column  File    ISA  Discriminator  OpIndex  Flags\n";
    oss << "------------------  ------  ------  ------  ---  -------------  -------  -------------\n";
    for (const auto &row : rows) {
      oss << "0x" << std::hex << std::setw(16) << std::setfill('0') << row.address << "  ";
      oss << std::dec << std::setw(6) << row.line << "  ";
      oss << std::setw(6) << row.column << "  ";
      oss << std::setw(6) << row.file << "  ";
      oss << std::setw(3) << row.isa << "  ";
      oss << std::setw(13) << row.discriminator << "  ";
      oss << std::setw(7) << row.op_index << "  ";
      std::string flags;
      if (row.is_stmt) flags += "is_stmt ";
      if (row.basic_block) flags += "basic_block ";
      if (row.prologue_end) flags += "prologue_end ";
      if (row.end_sequence) flags += "end_sequence ";
      if (row.epilogue_begin) flags += "epilogue_begin ";
      oss << flags << "\n";
    }
  }

  void writeDataTo(char *buffer) override {
    std::vector<uint8_t> out;

    // --- 1. total_length 预留 ---
    size_t totalLengthOffset = out.size();
    out.resize(out.size() + 4); // DWARF32: total_length 占位

    // --- 2. Header (version, address size, etc) ---
    out.push_back(header.version & 0xff);
    out.push_back((header.version >> 8) & 0xff);
    out.push_back(header.address_size);
    out.push_back(header.segment_selector_size);

    size_t prologueLengthOffset = out.size();
    out.resize(out.size() + 4); // prologue_length 占位
    size_t prologueStart = out.size();

    out.push_back(header.min_inst_length);
    out.push_back(header.max_ops_per_inst);
    out.push_back(header.default_is_stmt);
    out.push_back(static_cast<uint8_t>(header.line_base));
    out.push_back(header.line_range);
    out.push_back(header.opcode_base);

    out.insert(out.end(), header.standard_opcode_lengths.begin(), header.standard_opcode_lengths.end());

    // attr_count
    encodeULEB128(header.dir_attrs.size(), out);
    // attr spec
    for (const auto &[content_type, form] : header.dir_attrs) {
      encodeULEB128(content_type, out);
      encodeULEB128(form, out);
    }
    // directory count
    encodeULEB128(header.directories.size(), out);

    // entries
    for (const auto &dir : header.directories) {
      for (const auto &[type, form] : header.dir_attrs) {
        if (type == 1 /* DW_LNCT_path */) {
          // DW_FORM_string
          writeFormValue(dir, out);
        }
      }
    }

    encodeULEB128(header.file_attrs.size(), out);

    // attr spec
    for (const auto &[content_type, form] : header.file_attrs) {
      encodeULEB128(content_type, out);
      encodeULEB128(form, out);
    }

    // file count
    encodeULEB128(header.file_names.size(), out);

    // entries
    for (const auto &f : header.file_names) {
      for (const auto &[type, form] : header.file_attrs) {
        if (type == 1 /* DW_LNCT_path */) {
          // DW_FORM_string
          writeFormValue(f.val, out);
        } else if (type == 2 /* DW_LNCT_directory_index */) {
          encodeULEB128(f.dir_index, out);
        } else if (type == 5 /* DW_LNCT_md5 */) {
          if (f.md5.size() != 16)
            llvm::errs() << "MD5 should be 16 bytes" << "\n";
          out.insert(out.end(), f.md5.begin(), f.md5.end());
        } else {
          llvm::errs() << "Unsupported file_attr type in write" << "\n";
        }
      }
    }

    // --- 5. 回填 prologue_length ---
    uint32_t prologueLength = static_cast<uint32_t>(out.size() - prologueStart);
    out[prologueLengthOffset + 0] = (prologueLength & 0xff);
    out[prologueLengthOffset + 1] = (prologueLength >> 8) & 0xff;
    out[prologueLengthOffset + 2] = (prologueLength >> 16) & 0xff;
    out[prologueLengthOffset + 3] = (prologueLength >> 24) & 0xff;

    // TODO : line number program
    // --- 6. line number program 写入 ---
    Row prev;
    prev.is_stmt = header.default_is_stmt;
    prev.end_sequence = true;
    uint64_t maxSpecialAddrDelta = (255 - header.opcode_base) / header.line_range;
    for (auto row : rows) {
      int64_t addrDelta = row.address - prev.address;
      if (row.end_sequence) {
        if (addrDelta >= 0 && static_cast<uint64_t>(addrDelta) == maxSpecialAddrDelta)
          out.push_back(8);
        else {
          out.push_back(2); // DW_LNS_advance_pc
          encodeULEB128(addrDelta / header.min_inst_length, out);
          prev.address = row.address;
        }
        out.push_back(0);
        encodeULEB128(1, out);
        out.push_back(1);
        prev = {};
        prev.line = 1;
        prev.is_stmt = header.default_is_stmt;
        prev.end_sequence = true;
        continue;
      }
      int64_t lineDelta = row.line - prev.line;

      if (row.file != prev.file) {
        out.push_back(4);
        encodeULEB128(row.file, out);
        prev.file = row.file;
      }

      if (row.column != prev.column) {
        out.push_back(5); // DW_LNS_set_column
        encodeULEB128(row.column, out);
        prev.column = row.column;
      }

      if (row.discriminator != prev.discriminator) {
        out.push_back(0); // extended opcode
        unsigned size = llvm::getULEB128Size(row.discriminator);
        encodeULEB128(size + 1, out);
        out.push_back(4);
        encodeULEB128(row.discriminator, out);
        prev.discriminator = row.discriminator;
      }

      if (row.isa != prev.isa) {
        out.push_back(12); // DW_LNS_set_isa
        encodeULEB128(row.isa, out);
        prev.isa = row.isa;
      }

      if (row.is_stmt != prev.is_stmt) {
        out.push_back(6); // DW_LNS_negate_stmt
        prev.is_stmt = row.is_stmt;
      }

      if (row.basic_block && !prev.basic_block) {
        out.push_back(7); // DW_LNS_set_basic_block
        prev.basic_block = true;
      }

      if (row.prologue_end && !prev.prologue_end) {
        out.push_back(10); // DW_LNS_set_prologue_end
        prev.prologue_end = true;
      }

      if (row.epilogue_begin && !prev.epilogue_begin) {
        out.push_back(11); // DW_LNS_set_epilogue_begin
        prev.epilogue_begin = true;
      }

      if (prev.end_sequence && !row.end_sequence) {
        prev.end_sequence = false;
        out.push_back(0); // extended opcode
        encodeULEB128(1 + header.address_size, out);
        out.push_back(2); // DW_LNE_SET_ADDRESS
        for (int i = 0; i < header.address_size; ++i)
          out.push_back((row.address >> (i * 8)) & 0xff);
        prev.address = row.address;
      }
      int64_t tempSigned;
      uint64_t Temp, Opcode;
      bool needCopy = false;

      tempSigned = lineDelta - header.line_base;

      if (tempSigned >= header.line_range || tempSigned + header.opcode_base > 255 || tempSigned < 0) {
        out.push_back(3);
        encodeSLEB128(lineDelta, out);
        lineDelta = 0;
        prev.line = row.line;
        tempSigned = 0 - header.line_base;
        needCopy = true;
      }

      if (lineDelta == 0 && addrDelta == 0) {
        out.push_back(1);
        continue;
      }

      Temp = static_cast<uint64_t>(tempSigned) + header.opcode_base;

      if (addrDelta >= 0 && static_cast<uint64_t>(addrDelta) < 256 + maxSpecialAddrDelta) {
        Opcode = Temp + addrDelta * header.line_range;
        if (Opcode <= 255) {
          out.push_back(Opcode);
          prev.discriminator = 0;
          prev.address = row.address;
          prev.line = row.line;
          prev.basic_block = false;
          prev.prologue_end = false;
          prev.epilogue_begin = false;
          continue;
        }

        // Try using DW_LNS_const_add_pc followed by special op.
        Opcode = Temp + (addrDelta - maxSpecialAddrDelta) * header.line_range;
        if (Opcode <= 255) {
          out.push_back(8);
          out.push_back(Opcode);
          prev.discriminator = 0;
          prev.address = row.address;
          prev.line = row.line;
          prev.basic_block = false;
          prev.prologue_end = false;
          prev.epilogue_begin = false;
          continue;
        }
      }

      out.push_back(2); // DW_LNS_advance_pc
      encodeULEB128(addrDelta / header.min_inst_length, out);

      if (needCopy)
        out.push_back(1);
      else {
        assert(Temp <= 255 && "Buggy special opcode encoding.");
        out.push_back(Temp);
      }
      prev.discriminator = 0;
      prev.address = row.address;
      prev.line = row.line;
    }

    uint32_t unitLength = static_cast<uint32_t>(out.size() - 4);
    for (int i = 0; i < 4; ++i)
      out[totalLengthOffset + i] = (unitLength >> (i * 8)) & 0xff;

    assert(out.size() <= sh_size &&
           "Rewritten .debug_line larger than original");
    memcpy(buffer, out.data(), out.size());
  }

  FormValueRaw parseFormValue(
      uint64_t form, const uint8_t *&p, const uint8_t *end,
      //                           const DebugInfoSection::CompileUnitHeader &cu,
      const DebugStrOffsetsSection *strOffsets = nullptr,
      const DebugStrSection *strSection = nullptr,
      const DebugAddrSection *addrSection = nullptr, uint64_t dieOffset = 0,
      int strOffsetsTableIndex = -1, uint64_t addrBaseOffset = 0,
      std::optional<int64_t> implicitConst = std::nullopt) {
    FormValueRaw result;
    result.form = form;
    const uint8_t *start = p;

    switch (form) {
    case 0x01: { // DW_FORM_addr
      if (end - p < 8) {
        llvm::errs() << "Error: DW_FORM_addr: not enough bytes left in buffer\n";
        p = end;
        break;
      }
      result.value = *reinterpret_cast<const uint64_t *>(p);
      p += 8;
      break;
    }
    case 0x03: { // DW_FORM_block2
      uint16_t len = *reinterpret_cast<const uint16_t *>(p);
      p += 2;
      result.blockData.insert(result.blockData.end(), p, p + len);
      p += len;
      break;
    }
    case 0x04: { // DW_FORM_block4
      uint32_t len = *reinterpret_cast<const uint32_t *>(p);
      p += 4;
      result.blockData.insert(result.blockData.end(), p, p + len);
      p += len;
      break;
    }
    case 0x05: { // DW_FORM_data2
      result.value = *reinterpret_cast<const uint16_t *>(p);
      p += 2;
      break;
    }
    case 0x06: { // DW_FORM_data4
      result.value = *reinterpret_cast<const uint32_t *>(p);
      p += 4;
      break;
    }
    case 0x07: { // DW_FORM_data8
      result.value = *reinterpret_cast<const uint64_t *>(p);
      p += 8;
      break;
    }
    case 0x08: { // DW_FORM_string
      result.str = std::string(reinterpret_cast<const char *>(p));
      p += result.str.size() + 1;
      break;
    }
    case 0x09: { // DW_FORM_block
      unsigned size = 0;
      uint64_t len = decodeULEB128(p, &size, end);
      p += size;
      std::ostringstream oss;
      result.blockData.insert(result.blockData.end(), p, p + len);
      p += len;
      break;
    }
    case 0x0a: { // DW_FORM_block1
      uint8_t len = *p++;
      result.blockData.insert(result.blockData.end(), p, p + len);
      p += len;
      break;
    }
    case 0x0b: { // DW_FORM_data1
      result.value = *p++;
      break;
    }
    case 0x0c: { // DW_FORM_flag
      result.flag = (*p++) != 0;
      break;
    }
    case 0x0d: { // DW_FORM_sdata
      unsigned size = 0;
      result.value = decodeSLEB128(p, &size, end);
      p += size;
      break;
    }
    case 0x0e: { // DW_FORM_strp
      result.value = *reinterpret_cast<const uint32_t *>(p);
      p += 4;
      if (strSection) {
        result.str = strSection->getString(result.value);
      } else {
        result.str = "";
      }
      break;
    }
    case 0x0f: { // DW_FORM_udata
      unsigned len = 0;
      result.value = decodeULEB128(p, &len, end);
      p += len;
      break;
    }
    case 0x10: // DW_FORM_ref_addr
    case 0x1c: // DW_FORM_ref_sup4
    case 0x24: // DW_FORM_ref_sup8
    case 0x20:
    case 0x14: { // DW_FORM_ref_sig8
      result.value = *reinterpret_cast<const uint64_t *>(p);
      p += 8;
      break;
    }
    case 0x11: {
      result.value = *p++;
      break;
    }
    case 0x12: {
      result.value = *reinterpret_cast<const uint16_t *>(p);
      p += 2;
      break;
    }
    case 0x13: {
      result.value = *reinterpret_cast<const uint32_t *>(p);
      p += 4;
      break;
    }
    case 0x15: { // DW_FORM_ref_udata
      unsigned len = 0;
      result.value = decodeULEB128(p, &len, end);
      p += len;
      break;
    }
    case 0x16: { // DW_FORM_indirect
      unsigned len = 0;
      uint64_t actualForm = decodeULEB128(p, &len, end);
      p += len;
      return parseFormValue(actualForm, p, end, strOffsets, strSection,
                            addrSection, dieOffset);
    }
    case 0x17: { // DW_FORM_sec_offset
      result.value = *reinterpret_cast<const uint32_t *>(p);
      p += 4;
      break;
    }
    case 0x18: { // DW_FORM_exprloc
      unsigned len = 0;
      uint64_t size = decodeULEB128(p, &len, end);
      p += len;
      result.blockData.insert(result.blockData.end(), p, p + size);
      p += size;
      break;
    }
    case 0x19: {
      result.flag = true;
      break;
    }
    case 0x1b: {
      unsigned len = 0;
      result.value = decodeULEB128(p, &len, end);
      p += len;
      break;
    }
    case 0x1f: {
      result.value = *reinterpret_cast<const uint32_t *>(p);
      p += 4;
      break;
    }
    case 0x1d: { // DW_FORM_strp_sup
      result.value = *reinterpret_cast<const uint32_t *>(p);
      result.str = std::to_string(result.value);
      p += 4;
      break;
    }
    case 0x1e: { // DW_FORM_data16
      result.blockData.insert(result.blockData.end(), p, p + 16);
      p += 16;
      break;
    }
    case 0x21: { // DW_FORM_implicit_const
      if (implicitConst.has_value())
        result.value = implicitConst.value();
      else
        llvm::errs() << "DW_FORM_implicit_const missing value at DIE offset 0x"
                     << intToHex(dieOffset, 8) << "\n";
      break;
    }
    case 0x22:
    case 0x23: { // DW_FORM_rnglistx
      unsigned len = 0;
      result.value = decodeULEB128(p, &len, end);
      p += len;
      break;
    }
    case 0x25: { // DW_FORM_strx1
      result.value = *p++;
      if (strOffsets && strSection && strOffsetsTableIndex >= 0) {
        result.str =
            strOffsets->getStringFromStrx(strOffsetsTableIndex, result.value);
      }
      break;
    }

    case 0x1a:   // DW_FORM_strx
    case 0x26:   // DW_FORM_strx2
    case 0x27:   // DW_FORM_strx3
    case 0x28: { // DW_FORM_strx4
      unsigned len = 0;
      result.value = decodeULEB128(p, &len, end);
      p += len;
      if (strOffsets && strSection && strOffsetsTableIndex >= 0) {
        result.str =
            strOffsets->getStringFromStrx(strOffsetsTableIndex, result.value);
      }
      break;
    }
    case 0x29:
    case 0x2a:
    case 0x2b:
    case 0x2c: { // DW_FORM_addrx[1-4]
      unsigned len = 0;
      result.value = decodeULEB128(p, &len, end);
      p += len;
      break;
    }
    default:
      llvm::errs() << "Unsupported form 0x" + intToHex(form, 2);
    }
    result.rawBytes.assign(start, p);
    return result;
  }
};

} // namespace elf
} // namespace funcv
} // namespace iclang
#endif

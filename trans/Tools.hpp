#ifndef ICLANG_TOOLS_HPP
#define ICLANG_TOOLS_HPP

#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>

inline  std::string  getUnitType(uint8_t unitType){
  switch (unitType) {
  case 0x01: return "DW_UT_compile";
  case 0x02: return "DW_UT_type";
  case 0x03: return "DW_UT_partial";
  case 0x04: return "DW_UT_skeleton";
  case 0x05: return "DW_UT_split_compile";
  case 0x06: return "DW_UT_split_type";
  case 0x80: return "DW_UT_lo_user";
  case 0xff: return "DW_UT_hi_user";
  default: return "DW_UT_<unknown>";
  }
}
// Reference: "DWARF5", page 204.
inline std::string getTagName(uint64_t tag) {
  switch (tag) {
  case 0x01: return "DW_TAG_array_type";
  case 0x02: return "DW_TAG_class_type";
  case 0x03: return "DW_TAG_entry_point";
  case 0x04: return "DW_TAG_enumeration_type";
  case 0x05: return "DW_TAG_formal_parameter";
  case 0x08: return "DW_TAG_imported_declaration";
  case 0x0a: return "DW_TAG_label";
  case 0x0b: return "DW_TAG_lexical_block";
  case 0x0d: return "DW_TAG_member";
  case 0x0f: return "DW_TAG_pointer_type";
  case 0x10: return "DW_TAG_reference_type";
  case 0x11: return "DW_TAG_compile_unit";
  case 0x12: return "DW_TAG_string_type";
  case 0x13: return "DW_TAG_structure_type";
  case 0x15: return "DW_TAG_subroutine_type";
  case 0x16: return "DW_TAG_typedef";
  case 0x17: return "DW_TAG_union_type";
  case 0x18: return "DW_TAG_unspecified_parameters";
  case 0x19: return "DW_TAG_variant";
  case 0x1a: return "DW_TAG_common_block";
  case 0x1b: return "DW_TAG_common_inclusion";
  case 0x1c: return "DW_TAG_inheritance";
  case 0x1d: return "DW_TAG_inlined_subroutine";
  case 0x1e: return "DW_TAG_module";
  case 0x1f: return "DW_TAG_ptr_to_member_type";
  case 0x20: return "DW_TAG_set_type";
  case 0x21: return "DW_TAG_subrange_type";
  case 0x22: return "DW_TAG_with_stmt";
  case 0x23: return "DW_TAG_access_declaration";
  case 0x24: return "DW_TAG_base_type";
  case 0x25: return "DW_TAG_catch_block";
  case 0x26: return "DW_TAG_const_type";
  case 0x27: return "DW_TAG_constant";
  case 0x28: return "DW_TAG_enumerator";
  case 0x29: return "DW_TAG_file_type";
  case 0x2a: return "DW_TAG_friend";
  case 0x2b: return "DW_TAG_namelist";
  case 0x2c: return "DW_TAG_namelist_item";
  case 0x2d: return "DW_TAG_packed_type";
  case 0x2e: return "DW_TAG_subprogram";
  case 0x2f: return "DW_TAG_template_type_parameter";
  case 0x30: return "DW_TAG_template_value_parameter";
  case 0x31: return "DW_TAG_thrown_type";
  case 0x32: return "DW_TAG_try_block";
  case 0x33: return "DW_TAG_variant_part";
  case 0x34: return "DW_TAG_variable";
  case 0x35: return "DW_TAG_volatile_type";
  case 0x36: return "DW_TAG_dwarf_procedure";
  case 0x37: return "DW_TAG_restrict_type";
  case 0x38: return "DW_TAG_interface_type";
  case 0x39: return "DW_TAG_namespace";
  case 0x3a: return "DW_TAG_imported_module";
  case 0x3b: return "DW_TAG_unspecified_type";
  case 0x3c: return "DW_TAG_partial_unit";
  case 0x3d: return "DW_TAG_imported_unit";
  case 0x3f: return "DW_TAG_condition";
  case 0x40: return "DW_TAG_shared_type";
  case 0x41: return "DW_TAG_type_unit";
  case 0x42: return "DW_TAG_rvalue_reference_type";
  case 0x43: return "DW_TAG_template_alias";
  case 0x44: return "DW_TAG_coarray_type";
  case 0x45: return "DW_TAG_generic_subrange";
  case 0x46: return "DW_TAG_dynamic_type";
  case 0x47: return "DW_TAG_atomic_type";
  case 0x48: return "DW_TAG_call_site";
  case 0x49: return "DW_TAG_call_site_parameter";
  case 0x4a: return "DW_TAG_skeleton_unit";
  case 0x4b: return "DW_TAG_immutable_type";
  case 0x4080: return "DW_TAG_lo_user";
  case 0xffff: return "DW_TAG_hi_user";
  default: return "DW_TAG_<unknown>";
  }
}

// Reference: "DWARF5", page 207.
inline std::string getAttrName(uint64_t attr) {
  switch (attr) {
  case 0x01: return "DW_AT_sibling";
  case 0x02: return "DW_AT_location";
  case 0x03: return "DW_AT_name";
  case 0x09: return "DW_AT_ordering";
  case 0x0b: return "DW_AT_byte_size";
  case 0x0d: return "DW_AT_bit_size";
  case 0x10: return "DW_AT_stmt_list";
  case 0x11: return "DW_AT_low_pc";
  case 0x12: return "DW_AT_high_pc";
  case 0x13: return "DW_AT_language";
  case 0x15: return "DW_AT_discr";
  case 0x16: return "DW_AT_discr_value";
  case 0x17: return "DW_AT_visibility";
  case 0x18: return "DW_AT_import";
  case 0x19: return "DW_AT_string_length";
  case 0x1a: return "DW_AT_common_reference";
  case 0x1b: return "DW_AT_comp_dir";
  case 0x1c: return "DW_AT_const_value";
  case 0x1d: return "DW_AT_containing_type";
  case 0x1e: return "DW_AT_default_value";
  case 0x20: return "DW_AT_inline";
  case 0x21: return "DW_AT_is_optional";
  case 0x22: return "DW_AT_lower_bound";
  case 0x25: return "DW_AT_producer";
  case 0x27: return "DW_AT_prototyped";
  case 0x2a: return "DW_AT_return_addr";
  case 0x2c: return "DW_AT_start_scope";
  case 0x2e: return "DW_AT_bit_stride";
  case 0x2f: return "DW_AT_upper_bound";
  case 0x31: return "DW_AT_abstract_origin";
  case 0x32: return "DW_AT_accessibility";
  case 0x33: return "DW_AT_address_class";
  case 0x34: return "DW_AT_artificial";
  case 0x35: return "DW_AT_base_types";
  case 0x36: return "DW_AT_calling_convention";
  case 0x37: return "DW_AT_count";
  case 0x38: return "DW_AT_data_member_location";
  case 0x39: return "DW_AT_decl_column";
  case 0x3a: return "DW_AT_decl_file";
  case 0x3b: return "DW_AT_decl_line";
  case 0x3c: return "DW_AT_declaration";
  case 0x3d: return "DW_AT_discr_list";
  case 0x3e: return "DW_AT_encoding";
  case 0x3f: return "DW_AT_external";
  case 0x40: return "DW_AT_frame_base";
  case 0x41: return "DW_AT_friend";
  case 0x42: return "DW_AT_identifier_case";
  case 0x44: return "DW_AT_namelist_item";
  case 0x45: return "DW_AT_priority";
  case 0x46: return "DW_AT_segment";
  case 0x47: return "DW_AT_specification";
  case 0x48: return "DW_AT_static_link";
  case 0x49: return "DW_AT_type";
  case 0x4a: return "DW_AT_use_location";
  case 0x4b: return "DW_AT_variable_parameter";
  case 0x4c: return "DW_AT_virtuality";
  case 0x4d: return "DW_AT_vtable_elem_location";
  case 0x4e: return "DW_AT_allocated";
  case 0x4f: return "DW_AT_associated";
  case 0x50: return "DW_AT_data_location";
  case 0x51: return "DW_AT_byte_stride";
  case 0x52: return "DW_AT_entry_pc";
  case 0x53: return "DW_AT_use_UTF8";
  case 0x54: return "DW_AT_extension";
  case 0x55: return "DW_AT_ranges";
  case 0x56: return "DW_AT_trampoline";
  case 0x57: return "DW_AT_call_column";
  case 0x58: return "DW_AT_call_file";
  case 0x59: return "DW_AT_call_line";
  case 0x5a: return "DW_AT_description";
  case 0x5b: return "DW_AT_binary_scale";
  case 0x5c: return "DW_AT_decimal_scale";
  case 0x5d: return "DW_AT_small";
  case 0x5e: return "DW_AT_decimal_sign";
  case 0x5f: return "DW_AT_digit_count";
  case 0x60: return "DW_AT_picture_string";
  case 0x61: return "DW_AT_mutable";
  case 0x62: return "DW_AT_threads_scaled";
  case 0x63: return "DW_AT_explicit";
  case 0x64: return "DW_AT_object_pointer";
  case 0x65: return "DW_AT_endianity";
  case 0x66: return "DW_AT_elemental";
  case 0x67: return "DW_AT_pure";
  case 0x68: return "DW_AT_recursive";
  case 0x69: return "DW_AT_signature";
  case 0x6a: return "DW_AT_main_subprogram";
  case 0x6b: return "DW_AT_data_bit_offset";
  case 0x6c: return "DW_AT_const_expr";
  case 0x6d: return "DW_AT_enum_class";
  case 0x6e: return "DW_AT_linkage_name";
  case 0x6f: return "DW_AT_string_length_bit_size";
  case 0x70: return "DW_AT_string_length_byte_size";
  case 0x71: return "DW_AT_rank";
  case 0x72: return "DW_AT_str_offsets_base";
  case 0x73: return "DW_AT_addr_base";
  case 0x74: return "DW_AT_rnglists_base";
  case 0x76: return "DW_AT_dwo_name";
  case 0x77: return "DW_AT_reference";
  case 0x78: return "DW_AT_rvalue_reference";
  case 0x79: return "DW_AT_macros";
  case 0x7a: return "DW_AT_call_all_calls";
  case 0x7b: return "DW_AT_call_all_source_calls";
  case 0x7c: return "DW_AT_call_all_tail_calls";
  case 0x7d: return "DW_AT_call_return_pc";
  case 0x7e: return "DW_AT_call_value";
  case 0x7f: return "DW_AT_call_origin";
  case 0x80: return "DW_AT_call_parameter";
  case 0x81: return "DW_AT_call_pc";
  case 0x82: return "DW_AT_call_tail_call";
  case 0x83: return "DW_AT_call_target";
  case 0x84: return "DW_AT_call_target_clobbered";
  case 0x85: return "DW_AT_call_data_location";
  case 0x86: return "DW_AT_call_data_value";
  case 0x87: return "DW_AT_noreturn";
  case 0x88: return "DW_AT_alignment";
  case 0x89: return "DW_AT_export_symbols";
  case 0x8a: return "DW_AT_deleted";
  case 0x8b: return "DW_AT_defaulted";
  case 0x8c: return "DW_AT_loclists_base";
  case 0x2000: return "DW_AT_lo_user";
  case 0x3fff: return "DW_AT_hi_user";
  default:   return "DW_AT_<unknown>";
  }
}

// Reference: "DWARF5", page 220.
inline std::string getFormName(uint64_t form) {
  switch (form) {
  case 0x01: return "DW_FORM_addr";
  case 0x03: return "DW_FORM_block2";
  case 0x04: return "DW_FORM_block4";
  case 0x05: return "DW_FORM_data2";
  case 0x06: return "DW_FORM_data4";
  case 0x07: return "DW_FORM_data8";
  case 0x08: return "DW_FORM_string";
  case 0x09: return "DW_FORM_block";
  case 0x0a: return "DW_FORM_block1";
  case 0x0b: return "DW_FORM_data1";
  case 0x0c: return "DW_FORM_flag";
  case 0x0d: return "DW_FORM_sdata";
  case 0x0e: return "DW_FORM_strp";
  case 0x0f: return "DW_FORM_udata";
  case 0x10: return "DW_FORM_ref_addr";
  case 0x11: return "DW_FORM_ref1";
  case 0x12: return "DW_FORM_ref2";
  case 0x13: return "DW_FORM_ref4";
  case 0x14: return "DW_FORM_ref8";
  case 0x15: return "DW_FORM_ref_udata";
  case 0x16: return "DW_FORM_indirect";
  case 0x17: return "DW_FORM_sec_offset";
  case 0x18: return "DW_FORM_exprloc";
  case 0x19: return "DW_FORM_flag_present";
  case 0x1a: return "DW_FORM_strx";
  case 0x1b: return "DW_FORM_addrx";
  case 0x1c: return "DW_FORM_ref_sup4";
  case 0x1d: return "DW_FORM_strp_sup";
  case 0x1e: return "DW_FORM_data16";
  case 0x1f: return "DW_FORM_line_strp";
  case 0x20: return "DW_FORM_ref_sig8";
  case 0x21: return "DW_FORM_implicit_const";
  case 0x22: return "DW_FORM_loclistx";
  case 0x23: return "DW_FORM_rnglistx";
  case 0x24: return "DW_FORM_ref_sup8";
  case 0x25: return "DW_FORM_strx1";
  case 0x26: return "DW_FORM_strx2";
  case 0x27: return "DW_FORM_strx3";
  case 0x28: return "DW_FORM_strx4";
  case 0x29: return "DW_FORM_addrx1";
  case 0x2a: return "DW_FORM_addrx2";
  case 0x2b: return "DW_FORM_addrx3";
  case 0x2c: return "DW_FORM_addrx4";
  default:   return "DW_FORM_<unknown>";
  }
}

inline  std::string  getLLEName(uint8_t kind){
  switch (kind) {
  case 0x00: return "DW_LLE_end_of_list";
  case 0x01: return "DW_LLE_base_addressx";
  case 0x02: return "DW_LLE_startx_endx";
  case 0x03: return "DW_LLE_startx_length";
  case 0x04: return "DW_LLE_offset_pair";
  case 0x05: return "DW_LLE_default_location";
  case 0x06: return "DW_LLE_base_address";
  case 0x07: return "DW_LLE_start_end";
  case 0x08: return "DW_LLE_start_length";
  default: return "DW_UT_<unknown>";
  }
}

static std::string opcodeName(uint8_t opcode) {
  switch (opcode) {
  case 1:
    return "copy";
  case 2:
    return "advance_pc";
  case 3:
    return "advance_line";
  case 4:
    return "set_file";
  case 5:
    return "set_column";
  case 6:
    return "negate_stmt";
  case 7:
    return "set_basic_block";
  case 8:
    return "const_add_pc";
  case 9:
    return "fixed_advance_pc";
  case 10:
    return "set_prologue_end";
  case 11:
    return "set_epilogue_begin";
  case 12:
    return "set_isa";
  default:
    return std::to_string(opcode);
  }
}

std::string intToHex(uint64_t val, int width) {
  std::ostringstream oss;
  oss << std::hex << std::setw(width) << std::setfill('0') << val;
  return oss.str();
}

// Ref: llvm/include/llvm/Support/LEB128.h
uint64_t decodeULEB128(const uint8_t *p, unsigned *n = nullptr,
                       const uint8_t *end = nullptr,
                       const char **error = nullptr) {
  const uint8_t *orig_p = p;
  uint64_t Value = 0;
  unsigned Shift = 0;
  do {
    if (p == end) {
      if (error)
        *error = "malformed uleb128, extends past end";
      Value = 0;
      break;
    }
    uint64_t Slice = *p & 0x7f;

    if (Shift >= 63 &&
        ((Shift == 63 && (Slice << Shift >> Shift) != Slice) ||
         (Shift > 63 && Slice != 0))) {
      if (error)
        *error = "uleb128 too big for uint64";
      Value = 0;
      break;
    }
    Value += Slice << Shift;
    Shift += 7;
  } while (*p++ >= 128);
  if (n)
    *n = (unsigned)(p - orig_p);
  return Value;
}

int64_t decodeSLEB128(const uint8_t *p, unsigned *n = nullptr,
                      const uint8_t *end = nullptr,
                      const char **error = nullptr) {
  const uint8_t *orig_p = p;
  int64_t Value = 0;
  unsigned Shift = 0;
  uint8_t Byte;
  do {
    if (p == end) {
      if (error)
        *error = "malformed sleb128, extends past end";
      if (n)
        *n = (unsigned)(p - orig_p);
      return 0;
    }
    Byte = *p;
    uint64_t Slice = Byte & 0x7f;
    if (Shift >= 63 &&
        ((Shift == 63 && Slice != 0 && Slice != 0x7f) ||
         (Shift > 63 && Slice != (Value < 0 ? 0x7f : 0x00)))) {
      if (error)
        *error = "sleb128 too big for int64";
      if (n)
        *n = (unsigned)(p - orig_p);
      return 0;
    }
    Value |= Slice << Shift;
    Shift += 7;
    ++p;
  } while (Byte >= 128);
  // Sign extend negative numbers if needed.
  if (Shift < 64 && (Byte & 0x40))
    Value |= UINT64_MAX << Shift;
  if (n)
    *n = (unsigned)(p - orig_p);
  return Value;
}

inline unsigned encodeULEB128(uint64_t Value, uint8_t *p, unsigned PadTo = 0) {
  uint8_t *orig_p = p;
  unsigned Count = 0;
  do {
    uint8_t Byte = Value & 0x7f;
    Value >>= 7;
    Count++;
    if (Value != 0 || Count < PadTo)
      Byte |= 0x80;
    *p++ = Byte;
  } while (Value != 0);
  if (Count < PadTo) {
    for (; Count < PadTo - 1; ++Count)
      *p++ = '\x80';
    *p++ = '\x00';
  }
  return static_cast<unsigned>(p - orig_p);
}

inline unsigned encodeSLEB128(int64_t Value, uint8_t *p, unsigned PadTo = 0) {
  uint8_t *orig_p = p;
  unsigned Count = 0;
  bool More;
  do {
    uint8_t Byte = Value & 0x7f;
    Value >>= 7;
    More = !((((Value == 0 ) && ((Byte & 0x40) == 0)) ||
              ((Value == -1) && ((Byte & 0x40) != 0))));
    Count++;
    if (More || Count < PadTo)
      Byte |= 0x80;
    *p++ = Byte;
  } while (More);
  if (Count < PadTo) {
    uint8_t PadValue = Value < 0 ? 0x7f : 0x00;
    for (; Count < PadTo - 1; ++Count)
      *p++ = (PadValue | 0x80);
    *p++ = PadValue;
  }
  return static_cast<unsigned>(p - orig_p);
}

inline void encodeULEB128(uint64_t Value, std::vector<uint8_t> &out, unsigned PadTo = 0) {
  uint8_t buf[16];
  unsigned len = encodeULEB128(Value, buf, PadTo);
  out.insert(out.end(), buf, buf + len);
}

inline void encodeSLEB128(int64_t Value, std::vector<uint8_t> &out, unsigned PadTo = 0) {
  uint8_t buf[16];
  unsigned len = encodeSLEB128(Value, buf, PadTo);
  out.insert(out.end(), buf, buf + len);
}


std::string skipFormValue(uint64_t form, const uint8_t *&p, const uint8_t *end,
                           uint64_t dieOffset = 0) {
  switch (form) {
  case 0x01: { // DW_FORM_addr
    uint64_t val = *reinterpret_cast<const uint64_t *>(p);
    p += 8;
    return "addr: 0x" + intToHex(val, 16);
  }
  case 0x03: { // DW_FORM_block2
    uint16_t len = *reinterpret_cast<const uint16_t *>(p);
    p += 2;
    std::ostringstream oss;
    oss << "block2[";
    for (uint16_t i = 0; i < len && p < end; ++i)
      oss << intToHex(*p++, 2) << (i + 1 < len ? " " : "");
    oss << "]";
    return oss.str();
  }
  case 0x04: { // DW_FORM_block4
    uint32_t len = *reinterpret_cast<const uint32_t *>(p);
    p += 4;
    std::ostringstream oss;
    oss << "block4[";
    for (uint32_t i = 0; i < len && p < end; ++i)
      oss << intToHex(*p++, 2) << (i + 1 < len ? " " : "");
    oss << "]";
    return oss.str();
  }
  case 0x05: {
    uint16_t val = *reinterpret_cast<const uint16_t *>(p);
    p += 2;
    return "0x" + intToHex(val, 4);
  }
  case 0x06: {
    uint32_t val = *reinterpret_cast<const uint32_t *>(p);
    p += 4;
    return "0x" + intToHex(val, 8);
  }
  case 0x07: {
    uint64_t val = *reinterpret_cast<const uint64_t *>(p);
    p += 8;
    return "0x" + intToHex(val, 16);
  }
  case 0x08: { // DW_FORM_string
    std::string s(reinterpret_cast<const char *>(p));
    p += s.size() + 1;
    return "\"" + s + "\"";
  }
  case 0x09: { // DW_FORM_block
    unsigned size = 0;
    uint64_t len = decodeULEB128(p, &size, end);
    p += size;
    std::ostringstream oss;
    oss << "block[";
    for (uint64_t i = 0; i < len && p < end; ++i)
      oss << intToHex(*p++, 2) << (i + 1 < len ? " " : "");
    oss << "]";
    return oss.str();
  }
  case 0x0a: { // DW_FORM_block1
    uint8_t len = *p++;
    std::ostringstream oss;
    oss << "block1[";
    for (uint8_t i = 0; i < len && p < end; ++i)
      oss << intToHex(*p++, 2) << (i + 1 < len ? " " : "");
    oss << "]";
    return oss.str();
  }
  case 0x0b: {
    uint8_t val = *p++;
    return "0x" + intToHex(val, 2);
  }
  case 0x0c: { // DW_FORM_flag
    return *p++ ? "true" : "false";
  }
  case 0x0d: { // DW_FORM_sdata
    unsigned size = 0;
    int64_t val = decodeSLEB128(p, &size, end);
    p += size;
    return std::to_string(val);
  }
  case 0x0e: {
    p += 4;
    return "strp";
  }
  case 0x0f: { // DW_FORM_udata
    unsigned size = 0;
    uint64_t val = decodeULEB128(p, &size, end);
    p += size;
    return std::to_string(val);
  }
  case 0x10: // DW_FORM_ref_addr
  case 0x1c: // DW_FORM_ref_sup4
  case 0x24: // DW_FORM_ref_sup8
  case 0x20: { // DW_FORM_ref_sig8
    uint64_t ref = *reinterpret_cast<const uint64_t *>(p);
    p += 8;
    return "ref: 0x" + intToHex(ref, 16);
  }
  case 0x11: {
    uint8_t ref = *p++;
    return "ref: 0x" + intToHex(ref, 2);
  }
  case 0x12: {
    uint16_t ref = *reinterpret_cast<const uint16_t *>(p);
    p += 2;
    return "ref: 0x" + intToHex(ref, 4);
  }
  case 0x13: {
    uint32_t ref = *reinterpret_cast<const uint32_t *>(p);
    p += 4;
    return "ref: 0x" + intToHex(ref, 8);
  }
  case 0x14: {
    uint64_t ref = *reinterpret_cast<const uint64_t *>(p);
    p += 8;
    return "ref: 0x" + intToHex(ref, 16);
  }
  case 0x15: {
    unsigned len = 0;
    uint64_t ref = decodeULEB128(p, &len, end);
    p += len;
    return "ref: 0x" + intToHex(ref, 8);
  }
  case 0x16: { // DW_FORM_indirect
    unsigned len = 0;
    uint64_t actualForm = decodeULEB128(p, &len, end);
    p += len;
    return skipFormValue(actualForm, p, end, dieOffset);
  }
  case 0x17: {
    uint32_t val = *reinterpret_cast<const uint32_t *>(p);
    p += 4;
    return "sec_offset: 0x" + intToHex(val, 8);
  }
  case 0x18: {
    unsigned size = 0;
    uint64_t exprLen = decodeULEB128(p, &size, end);
    p += size;
    std::ostringstream expr;
    expr << "exprloc[";
    for (uint64_t i = 0; i < exprLen && p < end; ++i)
      expr << intToHex(*p++, 2) << (i + 1 < exprLen ? " " : "");
    expr << "]";
    return expr.str();
  }
  case 0x19: {
    return "true"; // 隐含存在
  }
  case 0x1b: {
    unsigned len = 0;
    uint64_t index = decodeULEB128(p, &len, end);
    p += len;
    return "addrx[]" + intToHex(index, 2);
  }
  case 0x1d: { // DW_FORM_strp_sup
    uint32_t offset = *reinterpret_cast<const uint32_t *>(p);
    p += 4;
    // return "\"" + debugStrSup.getString(offset) + "\"";  // 若有 .debug_str_sup 数据
    return "strp_sup[0x" + intToHex(offset, 8) + "]";
  }
  case 0x1e: { // DW_FORM_data16
    std::ostringstream out;
    out << "data16[";
    for (int i = 0; i < 16; ++i)
      out << intToHex(p[i], 2) << (i < 15 ? " " : "");
    out << "]";
    p += 16;
    return out.str();
  }
  case 0x1f: { // DW_FORM_line_strp
    uint32_t offset = *reinterpret_cast<const uint32_t *>(p);
    p += 4;
    // return "\"" + lineStrSection.getString(offset) + "\"";  // 若有 .debug_line_str 支持
    return "line_strp[0x" + intToHex(offset, 8) + "]";
  }
  case 0x21: { // DW_FORM_implicit_const
    return "(implicit_const from abbrev)";
  }
  case 0x22: { // DW_FORM_loclistx
    unsigned len = 0;
    uint64_t index = decodeULEB128(p, &len, end);
    p += len;
    return "loclistx[" + std::to_string(index) + "]";
  }
  case 0x23: { // DW_FORM_rnglistx
    unsigned len = 0;
    uint64_t index = decodeULEB128(p, &len, end);
    p += len;
    return "rnglistx[" + std::to_string(index) + "]";
  }
  case 0x25: { // DW_FORM_strx1
    uint8_t index = *p++;
    //    if (strOffsets && strSection) {
    //      uint32_t strOffset = strOffsets->getOffset(index);
    //      return "\"" + strSection->getString(strOffset) + "\"";
    //    }
    return intToHex(index, 2);
  }

  case 0x1a: // DW_FORM_strx
  case 0x26: // DW_FORM_strx2
  case 0x27: // DW_FORM_strx3
  case 0x28: { // DW_FORM_strx4
    unsigned len = 0;
    uint64_t index = decodeULEB128(p, &len, end);
    p += len;
    return "strx[" + std::to_string(index) + "]";
  }
  case 0x29: case 0x2a: case 0x2b: case 0x2c: { // DW_FORM_addrx[1-4]
    unsigned len = 0;
    uint64_t index = decodeULEB128(p, &len, end);
    p += len;
    return "addrx[" + std::to_string(index) + "]";
  }
  default:
    return "(unhandled form: 0x" + intToHex(form, 2) + ")";
  }
}

struct FormValueRaw {
  uint64_t form;
  std::vector<uint8_t> rawBytes;  // 原始字节
  uint64_t value = 0;             // 用于常数、引用、偏移
  std::string str;                // 用于 string、strp 解码后的值
  std::vector<uint8_t> blockData;// 用于 block、exprloc
  bool flag = false;             // 用于 DW_FORM_flag
};

void writeFormValue(const FormValueRaw &val, std::vector<uint8_t> &out)  {
  switch (val.form) {
  case 0x01: case 0x07: case 0x14: case 0x20: case 0x24: { // addr, data8, ref8
    for (int i = 0; i < 8; ++i)
      out.push_back((val.value >> (i * 8)) & 0xFF);
    break;
  }
  case 0x05: case 0x12: { // data2, ref2
    out.push_back(val.value & 0xFF);
    out.push_back((val.value >> 8) & 0xFF);
    break;
  }
  case 0x06: case 0x13: case 0x17: case 0x1d: case 0x1f: { // data4, ref4
    for (int i = 0; i < 4; ++i)
      out.push_back((val.value >> (i * 8)) & 0xFF);
    break;
  }
  case 0x0b: case 0x11: case 0x25: { // data1, ref1, strx1
    out.push_back(val.value & 0xFF);
    break;
  }
  case 0x0c: { // flag
    out.push_back(val.flag ? 1 : 0);
    break;
  }
  case 0x19: {
    break;
  }
  case 0x0d: { // sdata
    encodeSLEB128(val.value, out);
    break;
  }
  case 0x0f: case 0x15: case 0x1a: case 0x22: case 0x23:
  case 0x1b: { // udata
    encodeULEB128(val.value, out);
    break;
  }
  case 0x08: { // string
    out.insert(out.end(), val.str.begin(), val.str.end());
    out.push_back('\0');
    break;
  }
  case 0x03: { // block2
    out.push_back(val.blockData.size() & 0xFF);
    out.push_back((val.blockData.size() >> 8) & 0xFF);
    out.insert(out.end(), val.blockData.begin(), val.blockData.end());
    break;
  }
  case 0x04: { // block4
    for (int i = 0; i < 4; ++i)
      out.push_back((val.blockData.size() >> (i * 8)) & 0xFF);
    out.insert(out.end(), val.blockData.begin(), val.blockData.end());
    break;
  }
  case 0x09: case 0x18: { // block / exprloc
    encodeULEB128(val.blockData.size(), out);
    out.insert(out.end(), val.blockData.begin(), val.blockData.end());
    break;
  }
  case 0x0a: { // block1
    out.push_back(val.blockData.size() & 0xFF);
    out.insert(out.end(), val.blockData.begin(), val.blockData.end());
    break;
  }
  case 0x1e: { // data16
    out.insert(out.end(), val.blockData.begin(), val.blockData.begin() + 16);
    break;
  }
  case 0x21: {
    // 由 abbrev 编码，无需写入
    break;
  }
  case 0x26: { // strx2
    out.push_back(val.value & 0xFF);
    out.push_back((val.value >> 8) & 0xFF);
    break;
  }
  case 0x27: { // strx3
    out.push_back(val.value & 0xFF);
    out.push_back((val.value >> 8) & 0xFF);
    out.push_back((val.value >> 16) & 0xFF);
    break;
  }
  case 0x28: { // strx4
    for (int i = 0; i < 4; ++i)
      out.push_back((val.value >> (i * 8)) & 0xFF);
    break;
  }
  case 0x16: {
    // indirect: 不应直接写出，应写入嵌套 form 编码
    llvm::errs() << "DW_FORM_indirect should not reach writeFormValue\n";
    break;
  }
  case 0x0e: {
    for (int i = 0; i < 4; ++i)
      out.push_back((val.value >> (i * 8)) & 0xFF);
    break;
  }
  default:
    llvm::errs() << "Unsupported form in writeFormValue: 0x" << intToHex(val.form, 2) << "\n";
  }
}



#endif // ICLANG_TOOLS_HPP

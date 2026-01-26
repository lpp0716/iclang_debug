#ifndef ICLANG_DEBUG_INFO_SECTION_HPP
#define ICLANG_DEBUG_INFO_SECTION_HPP

#include "iclang/FuncV/ELF/Section.hpp"
#include "iclang/FuncV/ELF/Debug/DebugStrOffsets.hpp"
#include "iclang/FuncV/ELF/Debug/DebugAbbrev.hpp"
#include "iclang/FuncV/ELF/Debug/DebugAddr.hpp"
#include "iclang/FuncV/ELF/Debug/DebugRng.hpp"

namespace iclang {
namespace funcv {
namespace elf {

// Ref 7.5
class DebugInfoSection final : public Section {
private:
  // Structure representing a compile unit header
  struct CompileUnitHeader {
    uint64_t oldOffset;
    uint64_t offset;               // Offset in section
    uint32_t unitLength;           // Unit length
    uint16_t version;              // DWARF version
    uint8_t unitType;              // Unit type
    uint8_t addrSize;              // Address size
    uint32_t abbrevOffset;         // Abbreviation offset
    std::optional<uint64_t> dwoId; // Optional DWO ID
  };

  // Structure representing a DIE (Debugging Information Entry)
  struct DIE {
    uint64_t oldOffset;
    uint64_t offset;     // Offset in section
    uint64_t abbrevCode; // Abbreviation code
    uint64_t tag;
    const DebugAbbrevSection::AbbreviationDecl
        *abbrevDecl; // Abbreviation declaration
    std::vector<std::pair<uint64_t, FormValueRaw>> attributes; // Attributes
    std::vector<DIE> children;                                 // Child DIEs

    bool isRemoved = false;
    DIE* linkToMaster = nullptr;
  };

  struct DIEInstance {
    size_t cuIndex;
    uint64_t offset;
    const DIE* ptr;
  };

  std::vector<CompileUnitHeader> cuHeaders; // Compile unit headers
  std::vector<std::vector<DIE>> cuDIEs; // Array of top-level DIEs for each CU
  DebugAbbrevSection *abbrev;
  const DebugStrSection *debugStr;
  const DebugStrOffsetsSection *debugStrOffset;
  const DebugAddrSection *debugAddr;
  const DebugRnglistSection *debugRnglist;

  // 在 DebugInfoSection 私有成员中添加
  std::unordered_map<uint64_t, DIE*> oldOffsetToDieMap;

  // 辅助函数：解析完成后立即建立映射
  void buildOldOffsetMap() {
    oldOffsetToDieMap.clear();
    auto traverse = [&](auto& self, DIE& die) -> void {
      oldOffsetToDieMap[die.offset] = &die;
      for (auto& child : die.children) self(self, child);
    };
    for (auto& unit : cuDIEs) {
      for (auto& root : unit) traverse(traverse, root);
    }
  }

  struct DIEStructuralHash {
    size_t operator()(const DIE* die) const {
      if (!die || !die->abbrevDecl) return 0;
      size_t h = std::hash<uint64_t>{}(die->abbrevDecl->tag);
      for (const auto& [attr, val] : die->attributes) {
        h ^= std::hash<uint64_t>{}(attr) + 0x9e3779b9 + (h << 6) + (h >> 2);
        if (!val.str.empty()) {
          h ^= std::hash<std::string>{}(val.str) + 0x9e3779b9 + (h << 6) + (h >> 2);
        } else if (!val.blockData.empty()) {
          for (auto b : val.blockData)
            h ^= std::hash<uint8_t>{}(b) + 0x9e3779b9 + (h << 6) + (h >> 2);
        } else {
          h ^= std::hash<uint64_t>{}(val.value) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
      }
      return h;
    }
  };

  struct DIEStructuralEquality {
    bool operator()(const DIE* lhs, const DIE* rhs) const {
      if (lhs->abbrevDecl->tag != rhs->abbrevDecl->tag) return false;
      if (lhs->attributes.size() != rhs->attributes.size()) return false;
      for (size_t i = 0; i < lhs->attributes.size(); ++i) {
        if (lhs->attributes[i].first != rhs->attributes[i].first) return false;
        const auto& v1 = lhs->attributes[i].second;
        const auto& v2 = rhs->attributes[i].second;
        if (v1.form != v2.form || v1.value != v2.value || v1.str != v2.str || v1.blockData != v2.blockData)
          return false;
      }
      return true;
    }
  };

  using DuplicateGroupsMap = std::unordered_map<const DIE*, std::vector<DIEInstance>, DIEStructuralHash, DIEStructuralEquality>;

  DuplicateGroupsMap findBaseTypeDuplicates() {
    DuplicateGroupsMap groups;

    for (size_t i = 0; i < cuDIEs.size(); ++i) {
      for (const auto& die : cuDIEs[i]) {
        collectDuplicatesRecursive(i, die, groups);
      }
    }

    auto it = groups.begin();
    while (it != groups.end()) {
      if (it->second.size() < 2) {
        it = groups.erase(it);
      } else {
        ++it;
      }
    }
    return groups;
  }

  void collectDuplicatesRecursive(size_t cuIdx, const DIE& die, DuplicateGroupsMap& groups) {

    if (die.abbrevDecl && die.abbrevDecl->tag == 0x24) {

      bool hasAddressInfo = false;
      for (const auto& [attr, val] : die.attributes) {
        if (attr == 0x11 /* DW_AT_low_pc */ ||
            attr == 0x12 /* DW_AT_high_pc */ ||
            attr == 0x40 /* DW_AT_data_member_location */ ||
            attr == 0x02 /* DW_AT_location */ ||
            attr == 0x55 /* DW_AT_ranges */) {
          hasAddressInfo = true;
          break;
        }
      }

      if (!hasAddressInfo) {
        groups[&die].push_back({cuIdx, die.offset, &die});
      }
    }

    for (const auto& child : die.children) {
      collectDuplicatesRecursive(cuIdx, child, groups);
    }
  }


public:
  DebugInfoSection(const llvm::object::ELF64LE::Shdr *shdr, const char *_data,
                    DebugAbbrevSection *abbrevRef,
                   const DebugStrSection *strRef,
                   const DebugStrOffsetsSection *strOffsetRef,
                   const DebugAddrSection *addrRef,
                   const DebugRnglistSection *rnglistRef)
      : Section(SectionType::DebugInfo, shdr, _data), abbrev(abbrevRef),
        debugStr(strRef), debugStrOffset(strOffsetRef), debugAddr(addrRef), debugRnglist(rnglistRef) {
    const uint8_t *start = reinterpret_cast<const uint8_t *>(data);
    const uint8_t *end = start + sh_size;
    const uint8_t *p = start;

    while (p + 12 <= end) {
      uint64_t offset = p - start;

      // Reference: "DWARF5", page 200.
      //      uint32_t unitLength = *reinterpret_cast<const uint32_t *>(p);
      uint32_t unitLength = llvm::support::endian::read32le(p);
      p += 4;
      uint16_t version = llvm::support::endian::read16le(p);
      p += 2;
      uint8_t unitType = *p++;
      uint8_t addrSize = *p++;
      uint32_t abbrevOffset = llvm::support::endian::read32le(p);
      p += 4;

      std::optional<uint64_t> dwoId;
      if (unitType == 0x04 || unitType == 0x05) {
        if (p + 8 <= end) {
          dwoId = llvm::support::endian::read64le(p);
          p += 8;
        }
      }

      cuHeaders.push_back({offset, offset, unitLength, version, unitType, addrSize,
                           abbrevOffset, dwoId});

      const uint8_t *cuEnd = start + offset + 4 + unitLength;

      // parse root DIE early to extract str_offsets_base
      int strOffsetsTableIndex = -1;
      uint64_t addrBaseOffset = 0;
      const uint8_t *tmp = p;

      uint64_t dieOffset = tmp - start;
      unsigned len = 0;
      uint64_t abbrevCode = decodeULEB128(tmp, &len, cuEnd);
      tmp += len;


      const auto *decl = abbrev->getAbbreviationDecl(abbrevOffset, abbrevCode);
      if (decl) {
        for (const auto &af : decl->attrForms) {
          if (af.attr == 0x72) {
            uint64_t val =
                parseFormValue(af.form, tmp, cuEnd, nullptr, nullptr, nullptr,
                               dieOffset, strOffsetsTableIndex, addrBaseOffset)
                    .value;
            assert(debugStrOffset != nullptr);
            strOffsetsTableIndex = debugStrOffset->getTableIndex(val);
          } else if (af.attr == 0x73) {
            addrBaseOffset =
                parseFormValue(af.form, tmp, cuEnd, nullptr, nullptr, nullptr,
                               dieOffset, strOffsetsTableIndex, addrBaseOffset)
                    .value;

          } else {
            skipFormValue(af.form, tmp, cuEnd); // Skip uninteresting attributes
          }
        }
      }

      std::vector<DIE> topLevelDIEs;
      while (p < cuEnd) {
        DIE die = parseDIE(p, start, cuEnd, abbrevOffset, strOffsetsTableIndex,
                           addrBaseOffset);
        if (die.abbrevDecl == nullptr)
          break;
        topLevelDIEs.push_back(std::move(die));
      }
      cuDIEs.push_back(std::move(topLevelDIEs));
      p = start + offset + 4 + unitLength;
    }
    checkSamDie();
    createPartialUnitAndMoveDuplicates();
    finalizeMetadata(*abbrev);
  }

  DIE parseDIE(const uint8_t *&p, const uint8_t *start, const uint8_t *end,
               uint32_t abbrevOffset, int strOffsetsTableIndex = -1,
               uint64_t addrBaseOffset = 0) {
    uint64_t offset = p - start;
    unsigned len = 0;
    uint64_t abbrevCode = decodeULEB128(p, &len, end);
    p += len;

    if (abbrevCode == 0)
      return {};

    const auto *decl = abbrev->getAbbreviationDecl(abbrevOffset, abbrevCode);
    if (!decl)
      return {};

    DIE die;
    die.oldOffset = offset;
    die.offset = offset;
    die.abbrevCode = abbrevCode;
    die.abbrevDecl = decl;
    die.tag = decl->tag;

    for (const auto &af : decl->attrForms) {
      FormValueRaw valueRaw;
      if (af.form == 0x21)
        valueRaw = parseFormValue(af.form, p, end, debugStrOffset, debugStr,
                                  debugAddr, die.offset, strOffsetsTableIndex,addrBaseOffset,
                                  af.implicitConst.value());
      else
        valueRaw = parseFormValue(af.form, p, end, debugStrOffset, debugStr,
                                  debugAddr, die.offset, strOffsetsTableIndex,
                                  addrBaseOffset);
      die.attributes.emplace_back(af.attr, valueRaw);

      if (af.attr == 0x55 /* DW_AT_ranges */) {
        uint32_t rnglistIndex = static_cast<uint32_t>(valueRaw.value);
        assert(debugRnglist != nullptr);
        debugRnglist->registerAddrBase(rnglistIndex, addrBaseOffset);
      }
    }

    if (decl->hasChildren) {
      while (true) {
        auto child =
            parseDIE(p, start, end, abbrevOffset, strOffsetsTableIndex);
        if (child.abbrevDecl == nullptr) // Empty DIE indicates end of children
          break;
        die.children.push_back(std::move(child));
      }
    }

    return die;
  }

  void checkSamDie() {
    auto dups = findBaseTypeDuplicates();
    std::cout << "Found " << dups.size() << " unique base_types that are duplicated." << std::endl;

    for (const auto& [tpl, instances] : dups) {
      std::string typeName = "unknown";
      for(auto& attr : tpl->attributes) {
        if(attr.first == 0x03 /* DW_AT_name */) typeName = attr.second.str;
      }

      std::cout << "Type [" << typeName << "] is repeated " << instances.size() << " times:" << std::endl;
      for (const auto& inst : instances) {
        std::cout << "  - CU index: " << inst.cuIndex << " at offset: 0x" << std::hex << inst.offset << std::dec << std::endl;
      }
    }
  }

  void createPartialUnitAndMoveDuplicates() {
    auto dups = findBaseTypeDuplicates();
    if (dups.empty()) return;

    // --- 第一步：改变结构 (Structural Changes) ---
    // 1. 插入 PU Header 和空的 DIE 容器
    CompileUnitHeader puHeader;
    puHeader.version = 5;
    puHeader.unitType = 0x03; // DW_UT_partial
    puHeader.addrSize = cuHeaders[0].addrSize;
    puHeader.abbrevOffset = 0;
    puHeader.oldOffset = 0; // PU 是新造的

    cuHeaders.insert(cuHeaders.begin(), puHeader);
    cuDIEs.insert(cuDIEs.begin(), std::vector<DIE>());

    // 2. 初始化 PU 根节点
    DIE puRoot;
    puRoot.tag = 0x3c; // DW_TAG_partial_unit
    // 拷贝属性...
    if (cuDIEs.size() > 1 && !cuDIEs[1].empty()) {
      for (const auto& attr : cuDIEs[1][0].attributes) {
        if (attr.first == 0x10 || attr.first == 0x1b)
          puRoot.attributes.push_back(attr);
      }
    }

    // 3. 预留空间 (非常重要！防止后面 push_back 导致 master 搬家)
    puRoot.children.reserve(dups.size());
    for (auto& [tpl, instances] : dups) {
      DIE masterCopy = *tpl;
      masterCopy.children.clear();
      masterCopy.oldOffset = 0; // 母版是新造的
      puRoot.children.push_back(std::move(masterCopy));
    }
    cuDIEs[0].push_back(std::move(puRoot));

    // 4. 插入导入标记 (注意：此时只插结构，不填指针)
    // 记录哪些 CU 需要插入
    std::set<size_t> affectedCUs;
    for (auto& [tpl, instances] : dups) {
      for (auto& inst : instances) affectedCUs.insert(inst.cuIndex + 1);
    }

    for (size_t cuIdx : affectedCUs) {
      DIE importTag;
      importTag.tag = 0x3d;
      FormValueRaw importVal;
      importVal.form = 0x10; // ref_addr
      importVal.value = 0;   // 占位
      importTag.attributes.emplace_back(0x18 /* DW_AT_import */, importVal);

      if (!cuDIEs[cuIdx].empty()) {
        cuDIEs[cuIdx][0].children.insert(cuDIEs[cuIdx][0].children.begin(), std::move(importTag));
      }
    }

    // --- 第二步：地址定格 (Fixation) ---
    // 现在所有的 insert/push_back 都做完了，DIE 在内存中的位置固定了
    buildOldOffsetMap();

    // --- 第三步：逻辑关联 (Logical Linkage) ---
    DIE* puRootPtr = &cuDIEs[0][0];

    // 再次遍历重复组，设置 linkToMaster 和 isRemoved
    size_t masterIdx = 0;
    for (auto& [tpl, instances] : dups) {
      DIE* masterInPU = &puRootPtr->children[masterIdx++];
      for (auto& inst : instances) {
        // 通过刚才建立的地图找到已经在堆内存中固定位置的 DIE
        if (oldOffsetToDieMap.count(inst.offset)) {
          DIE* originalDie = oldOffsetToDieMap[inst.offset];
          originalDie->isRemoved = true;
          originalDie->linkToMaster = masterInPU;
        }
      }
    }

    // 修正所有 CU 里的导入标记，让它指向 PU 根节点
    for (size_t cuIdx : affectedCUs) {
      auto& cuRoot = cuDIEs[cuIdx][0];
      for (auto& child : cuRoot.children) {
        if (child.tag == 0x3d) { // imported_unit
          for (auto& attr : child.attributes) {
            if (attr.first == 0x18) {
              attr.second.value = reinterpret_cast<uint64_t>(puRootPtr);
            }
          }
          break; // 每个 CU 只有一个
        }
      }
    }
  }

  void dumpData(std::ostream &oss) const override {
    oss << ".debug_info contents:\n";
    // dump Compile Unit
    for (size_t i = 0; i < cuHeaders.size(); ++i) {
      const auto &cu = cuHeaders[i];
      const std::string unitTypeStr = getUnitType(cu.unitType);

      oss << std::hex << std::setfill('0');
      oss << "0x" << std::setw(8) << cu.offset << ": Compile Unit: ";
      oss << "length = 0x" << std::setw(8) << cu.unitLength << ", ";
      oss << "format = DWARF32, ";
      oss << "version = 0x" << std::setw(4) << cu.version << ", ";
      oss << "unit_type = " << unitTypeStr << ", ";
      oss << "abbr_offset = 0x" << std::setw(4) << cu.abbrevOffset << ", ";
      oss << "addr_size = 0x" << std::setw(2) << static_cast<int>(cu.addrSize)
          << " ";
      if (cu.dwoId.has_value())
        oss << ", DWO_id = " << std::setw(12) << cu.dwoId.value() << " ";
      // NextUnitOffset = Offset + Length + LengthFieldByteSize
      oss << "(next unit at 0x" << std::setw(8)
          << (cu.offset + 4 + cu.unitLength) << ")\n";
      oss << "\n";
      for (const auto &die : cuDIEs[i]) {
        dumpDIE(oss, die);
      }
    }
  }

  void dumpDIE(std::ostream &oss, const DIE &die) const {
    if (die.isRemoved)
      return;
    oss << "0x" << std::setw(8) << std::setfill('0') << std::hex << die.offset
        << ": " << getTagName(die.abbrevDecl->tag) << "\n";
    if (die.isRemoved) oss << "remove!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
    for (const auto &[attr, val] : die.attributes) {
      if (val.form == 0x08 || val.form == 0x25) {
        oss << std::string(14, ' ') << getAttrName(attr) << "\t(\"" << val.str
            << "\")\n";
      } else if (val.form == 0x03 || val.form == 0x04 || val.form == 0x09 ||
                 val.form == 0x0a || val.form == 0x18 || val.form == 0x1e) {
        oss << std::string(14, ' ') << getAttrName(attr)
            << "\t[BLOCK DATA, size=" << val.blockData.size() << "]: ";

        for (uint8_t byte : val.blockData) {
          oss << "0x" << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<int>(byte) << " ";
        }
        oss << std::dec << "\n";
      } else {
        oss << std::string(14, ' ') << getAttrName(attr) << "\t(" << val.value
            << ")\n";
      }
    }

    for (const auto &child : die.children)
      dumpDIE(oss, child);
  }

  void writeDataTo(char *buffer) override {
    std::vector<uint8_t> out;

    for (size_t i = 0; i < cuHeaders.size(); ++i) {
      const auto &cu = cuHeaders[i];
      size_t headerStart = out.size();

      // 1. Reserve space for unit_length (to be filled later)
      out.resize(out.size() + 4); // DWARF32 format

      // 2. Write header
      out.push_back(cu.version & 0xff);
      out.push_back((cu.version >> 8) & 0xff);
      out.push_back(cu.unitType);
      out.push_back(cu.addrSize);

      out.push_back(cu.abbrevOffset & 0xff);
      out.push_back((cu.abbrevOffset >> 8) & 0xff);
      out.push_back((cu.abbrevOffset >> 16) & 0xff);
      out.push_back((cu.abbrevOffset >> 24) & 0xff);
//      out.push_back(0);
//      out.push_back(0);
//      out.push_back(0);
//      out.push_back(0);

      if (cu.dwoId.has_value()) {
        uint64_t id = cu.dwoId.value();
        for (int j = 0; j < 8; ++j)
          out.push_back((id >> (j * 8)) & 0xff);
      }

      // 3. Write all top-level DIEs
      for (const auto &die : cuDIEs[i])
        writeDIE(die, out);

      // 4. Fill in unit_length
      uint32_t length = static_cast<uint32_t>(out.size() - headerStart - 4);
      llvm::support::endian::write32le(&out[headerStart], length);
//      out[headerStart + 0] = (length & 0xff);
//      out[headerStart + 1] = (length >> 8) & 0xff;
//      out[headerStart + 2] = (length >> 16) & 0xff;
//      out[headerStart + 3] = (length >> 24) & 0xff;
    }

    // 5. Copy to target buffer
//    assert(out.size() <= sh_size &&
//           "Rewritten .debug_info larger than original");
    memcpy(buffer, out.data(), out.size());
  }

  void writeDIE(const DIE &die, std::vector<uint8_t> &out) const {
    if (die.isRemoved)
      return;

    encodeULEB128(die.abbrevCode, out);

    for (const auto &attr : die.attributes) {
      writeFormValue(attr.second, out);
    }

    if (die.abbrevDecl->hasChildren) {
      for (const auto &child : die.children) {
        if (!child.isRemoved){
          writeDIE(child, out);
        }
      }
      out.push_back(0x00); // Null abbrev code for end of children
    }
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
    case 0x1d:
    case 0x1f: { // DW_FORM_strp_sup
      result.value = *reinterpret_cast<const uint32_t *>(p);
      p += 4;
      break;
    }
    case 0x1e: { // DW_FORM_data16
      result.blockData.insert(result.blockData.end(), p, p + 16);
      p += 16;
      break;
    }
    case 0x21: { // DW_FORM_implicit_const
      assert(implicitConst.has_value());
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

    case 0x1a: { // DW_FORM_strx
      unsigned len = 0;
      result.value = decodeULEB128(p, &len, end);
      p += len;
      if (strOffsets && strSection && strOffsetsTableIndex >= 0) {
        result.str =
            strOffsets->getStringFromStrx(strOffsetsTableIndex, result.value);
      }
      break;
    }
    case 0x26: // strx2
      result.value = *reinterpret_cast<const uint16_t *>(p);
      p += 2;
      if (strOffsets && strSection && strOffsetsTableIndex >= 0) {
        result.str =
            strOffsets->getStringFromStrx(strOffsetsTableIndex, result.value);
      }
      break;
    case 0x27: // strx3
      result.value = (*reinterpret_cast<const uint32_t *>(p)) & 0xFFFFFFu;
      p += 3;
      if (strOffsets && strSection && strOffsetsTableIndex >= 0) {
        result.str =
            strOffsets->getStringFromStrx(strOffsetsTableIndex, result.value);
      }
      break;
    case 0x28: // strx4
      result.value = *reinterpret_cast<const uint32_t *>(p);
      p += 4;
      if (strOffsets && strSection && strOffsetsTableIndex >= 0) {
        result.str =
            strOffsets->getStringFromStrx(strOffsetsTableIndex, result.value);
      }
      break;
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

  void finalizeMetadata(DebugAbbrevSection& newAbbrev) {
    // 阶段 A: 全局重建 Abbrev 表 (去重)
    newAbbrev.abbrevTables.clear();
    struct AbbrevKey {
      uint64_t tag; bool hasChildren;
      std::vector<std::pair<uint64_t, uint64_t>> layout;
      bool operator<(const AbbrevKey& o) const {
        if (tag != o.tag) return tag < o.tag;
        if (hasChildren != o.hasChildren) return hasChildren < o.hasChildren;
        return layout < o.layout;
      }
    };
    std::map<AbbrevKey, uint64_t> sigToCode;
    uint64_t nextCode = 1;

    auto getAbbrevCode = [&](DIE& die) -> uint64_t {
      AbbrevKey key;
      key.tag = die.tag;
      key.hasChildren = !die.children.empty();
      for (auto& attr : die.attributes) key.layout.push_back({attr.first, attr.second.form});
      if (sigToCode.find(key) == sigToCode.end()) {
        uint64_t code = nextCode++;
        sigToCode[key] = code;
        DebugAbbrevSection::AbbreviationDecl decl;
        decl.code = code; decl.tag = key.tag; decl.hasChildren = key.hasChildren;
        for (auto& l : key.layout) decl.attrForms.push_back({l.first, l.second, std::nullopt});
        newAbbrev.abbrevTables[0][code] = std::move(decl);
      }
      return sigToCode[key];
    };

    // 阶段 B: 分配新偏移量 (Layout)
    uint64_t totalSectionSize = 0;
    for (size_t i = 0; i < cuHeaders.size(); ++i) {
      auto& h = cuHeaders[i];
      h.offset = totalSectionSize;
      h.abbrevOffset = 0;
      uint64_t unitOffset = 12;
      if (h.dwoId.has_value()) unitOffset += 8;

      std::function<void(DIE&)> doLayout = [&](DIE& die) {
        if (die.isRemoved) return;
        die.offset = h.offset + unitOffset;
        die.abbrevCode = getAbbrevCode(die);
        die.abbrevDecl = &newAbbrev.abbrevTables[0][die.abbrevCode];

        unitOffset += llvm::getULEB128Size(die.abbrevCode);
        for (auto& attr : die.attributes) {
          unitOffset += attr.second.rawBytes.size();
        }
        for (auto& child : die.children) doLayout(child);
        if (die.abbrevDecl->hasChildren) unitOffset += 1; // 0x00 terminator
      };

      for (auto& root : cuDIEs[i]) doLayout(root);
      h.unitLength = unitOffset - 4;
      totalSectionSize += unitOffset;
    }

    // 阶段 C: 核心引用修正与 Form 升级
    for (size_t i = 0; i < cuDIEs.size(); ++i) {
      uint64_t currentUnitNewBase = cuHeaders[i].offset;
      uint64_t currentUintOldBase = cuHeaders[i].oldOffset;

      std::function<void(DIE&)> fixRefs = [&](DIE& die) {
        if (die.isRemoved) return;

        for (auto& [attrId, attrVal] : die.attributes) {
          // 1. 修正 DW_AT_import
          if (attrId == 0x18 && attrVal.form == 0x10 && attrVal.value > 0xFFFFFF) {
            DIE* target = reinterpret_cast<DIE*>(attrVal.value);
            attrVal.value = target->offset;
            continue;
          }

          // 2. 修正常规引用 (DW_AT_type等)
          if (isReferenceForm(attrVal.form)) {
            uint64_t targetOldOffset;
            // 注意：如果解析时 ref4 存的是相对偏移，需先加上所在原单元的起始地址
            // 这里假设 attrVal.value 存的是绝对偏移
            if (attrVal.form == 0x10) {
              targetOldOffset = attrVal.value;
            }
            else {
              targetOldOffset = currentUintOldBase + attrVal.value;
            }

            if (oldOffsetToDieMap.count(targetOldOffset)) {
              DIE* targetDie = oldOffsetToDieMap[targetOldOffset];

              if (targetDie->isRemoved && targetDie->linkToMaster) {
                // --- 升级点：引用目标被移到了 PU ---
                attrVal.value = targetDie->linkToMaster->offset;
                attrVal.form = 0x10; // 必须升级为 ref_addr，因为 Master 在 Unit 0
              } else {
                // --- 目标依然在原处 ---
                if (attrVal.form == 0x10) {
                  attrVal.value = targetDie->offset;
                } else {
                  attrVal.value = targetDie->offset - currentUnitNewBase;
                }
              }
            }
          }
        }
        for (auto& child : die.children) fixRefs(child);
      };
      for (auto& root : cuDIEs[i]) fixRefs(root);
    }
    setShSize(totalSectionSize);
  }

};

} // namespace elf
} // namespace funcv
} // namespace iclang
#endif

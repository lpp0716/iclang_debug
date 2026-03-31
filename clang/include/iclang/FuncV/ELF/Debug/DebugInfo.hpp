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
    for (size_t i = 0; i < cuDIEs.size(); ++i) {
      for (auto& root : cuDIEs[i]) traverse(traverse, root);
    }
  }

//  struct DIEStructuralHash {
//    size_t operator()(const DIE* die) const {
//      if (!die || !die->abbrevDecl) return 0;
//      size_t h = std::hash<uint64_t>{}(die->tag);
//      for (const auto& [attr, val] : die->attributes) {
//        h ^= std::hash<uint64_t>{}(attr) + 0x9e3779b9 + (h << 6) + (h >> 2);
//        if (!val.str.empty()) {
//          h ^= std::hash<std::string>{}(val.str) + 0x9e3779b9 + (h << 6) + (h >> 2);
//        } else if (!val.blockData.empty()) {
//          for (auto b : val.blockData)
//            h ^= std::hash<uint8_t>{}(b) + 0x9e3779b9 + (h << 6) + (h >> 2);
//        } else {
//          h ^= std::hash<uint64_t>{}(val.value) + 0x9e3779b9 + (h << 6) + (h >> 2);
//        }
//      }
//
//      for (const auto& child : die->children) {
//        h ^= (*this)(&child) + 0x9e3779b9 + (h << 6) + (h >> 2);
//      }
//
//      return h;
//    }
//  };
  struct DIEStructuralHash {
    size_t operator()(const DIE* die) const {
      if (!die) return 0;

      // 1. 初始化哈希值为 Tag
      size_t h = std::hash<uint64_t>{}(die->tag);

      // 2. 累加属性哈希
      for (const auto& [attr, val] : die->attributes) {
        // 特殊处理：忽略 DW_AT_type (0x49)，因为不同 CU 里的偏移值 95 和 62 会导致哈希不同
        if (attr == 0x49) continue;

        h ^= std::hash<uint64_t>{}(attr) + 0x9e3779b9 + (h << 6) + (h >> 2);
        if (!val.str.empty()) {
          h ^= std::hash<std::string>{}(val.str) + 0x9e3779b9 + (h << 6) + (h >> 2);
        } else {
          h ^= std::hash<uint64_t>{}(val.value) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
      }

      // 3. 【新增】递归累加子项哈希
      for (const auto& child : die->children) {
        // 递归调用自身
        size_t childHash = (*this)(&child);
        h ^= childHash + 0x9e3779b9 + (h << 6) + (h >> 2);
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
        if (lhs->attributes[i].first == 0x49)
          continue;
        const auto& v1 = lhs->attributes[i].second;
        const auto& v2 = rhs->attributes[i].second;
        if (v1.form != v2.form || v1.str != v2.str || v1.blockData != v2.blockData)
          return false;
//        if (v1.form != 0x13 && v1.form != 0x10 && v1.form != 0x11) {
//          if (v1.value != v2.value) return false;
//        }
      }

      // 2. 递归比较子项
      if (lhs->children.size() != rhs->children.size()) return false;
      for (size_t i = 0; i < lhs->children.size(); ++i) {
        if (!(*this)(&lhs->children[i], &rhs->children[i])) return false;
      }

      return true;
    }
  };
//  struct DIEStructuralEquality {
//    // 辅助函数：增加 verbose 参数用于深度追踪
//    bool compare(const DIE* lhs, const DIE* rhs, bool verbose) const {
//      if (!lhs || !rhs) return lhs == rhs;
//
//      // 1. Tag 校验
//      if (lhs->tag != rhs->tag) {
//        if (verbose) std::cout << "      [FAIL] Tag mismatch: 0x" << std::hex << lhs->tag << " vs 0x" << rhs->tag << std::dec << std::endl;
//        return false;
//      }
//
//      // 2. 属性数量校验
//      if (lhs->attributes.size() != rhs->attributes.size()) {
//        if (verbose) std::cout << "      [FAIL] Attr count mismatch: " << lhs->attributes.size() << " vs " << rhs->attributes.size() << std::endl;
//        return false;
//      }
//
//      // 3. 属性内容校验
//      for (size_t i = 0; i < lhs->attributes.size(); ++i) {
//        const auto& a1 = lhs->attributes[i];
//        const auto& a2 = rhs->attributes[i];
//
//        if (a1.first != a2.first) {
//          if (verbose) std::cout << "      [FAIL] AttrID mismatch at index " << i << ": 0x" << std::hex << a1.first << std::dec << std::endl;
//          return false;
//        }
//
//        const auto& v1 = a1.second;
//        const auto& v2 = a2.second;
//
//        bool match = true;
//        std::string reason = "";
//
//        // 检查 Form 是否一致 (非常重要！有的编译器用 data1, 有的用 data4)
//        if (v1.form != v2.form) {
//          // 虽然 Form 不同，但如果值一样，在某些逻辑下可以算相等。
//          // 但为了严谨，我们先标记出来。
//          // match = false;
//          // reason = "Form mismatch";
//        }
//
//        if (!v1.str.empty() || !v2.str.empty()) {
//          if (v1.str != v2.str) { match = false; reason = "String mismatch: " + v1.str + " vs " + v2.str; }
//        } else if (!v1.blockData.empty() || !v2.blockData.empty()) {
//          if (v1.blockData != v2.blockData) { match = false; reason = "BlockData mismatch"; }
//        } else {
//          if (v1.value != v2.value) { match = false; reason = "Value mismatch: " + std::to_string(v1.value) + " vs " + std::to_string(v2.value); }
//        }
//
//        if (!match) {
//          if (verbose) std::cout << "      [FAIL] Attr 0x" << std::hex << a1.first << std::dec << " " << reason << std::endl;
//          return false;
//        }
//      }
//
//      // 4. 递归校验子项
//      if (lhs->children.size() != rhs->children.size()) {
//        if (verbose) std::cout << "      [FAIL] Children count mismatch: " << lhs->children.size() << " vs " << rhs->children.size() << std::endl;
//        return false;
//      }
//
//      for (size_t i = 0; i < lhs->children.size(); ++i) {
//        if (!compare(&lhs->children[i], &rhs->children[i], verbose)) {
//          if (verbose) std::cout << "    [FAIL] Child at index " << i << " (Tag: 0x" << std::hex << lhs->children[i].tag << std::dec << ") is different." << std::endl;
//          return false;
//        }
//      }
//
//      return true;
//    }
//
//    // unordered_map 调用的入口
//    bool operator()(const DIE* lhs, const DIE* rhs) const {
//      std::string name = "";
//      for (auto& attr : lhs->attributes) if (attr.first == 0x03) name = attr.second.str;
//
//      // 如果是 CommonPoint，开启 verbose 模式
//      bool verbose = (name == "CommonPoint");
//
//      if (verbose) {
//        std::cout << "\n[Step 3] Deep Comparing Structure: " << name << " (0x" << std::hex << lhs->oldOffset << " vs 0x" << rhs->oldOffset << ")" << std::dec << std::endl;
//      }
//
//      bool result = compare(lhs, rhs, verbose);
//
//      if (verbose && result) std::cout << "  [SUCCESS] Identity confirmed!" << std::endl;
//      return result;
//    }
//  };

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

    if (die.abbrevDecl) {
      // 增加识别范围：BaseType, Struct, Class, Union, Enum
      bool isTypeTag = (die.tag == 0x24 || die.tag == 0x13 ||
                        die.tag == 0x02 || die.tag == 0x17 ||
                        die.tag == 0x04);

      if (isTypeTag) {
        bool hasAddressInfo = false;
        bool isDeclaration = false;

        for (const auto& [attr, val] : die.attributes) {
          // 如果含有位置信息，不进行去重（dwz 策略）
          if (attr == 0x11 || attr == 0x12 || attr == 0x40 ||
              attr == 0x02 || attr == 0x55) {
            hasAddressInfo = true;
            break;
          }
          // 仅去重完整的定义，跳过声明
          if (attr == 0x3c /* DW_AT_declaration */ && val.value != 0) {
            isDeclaration = true;
            break;
          }
        }

        if (!hasAddressInfo && !isDeclaration) {
          groups[&die].push_back({cuIdx, die.offset, &die});
          // 一旦标记该节点为去重候选，不再递归子项（例如 member），
          // 因为 master 整体移动会带动 member。
          return;
        }
      }
    }

    for (const auto& child : die.children) {
      collectDuplicatesRecursive(cuIdx, child, groups);
    }
  }
//  void collectDuplicatesRecursive(size_t cuIdx, const DIE& die, DuplicateGroupsMap& groups) {
//    if (die.abbrevDecl) {
//      // 打印所有结构体，看看它们有没有通过过滤条件
//      std::string currentName = "";
//      for(auto& attr : die.attributes) if(attr.first == 0x03) currentName = attr.second.str;
//
//      if (die.tag == 0x13 && currentName == "CommonPoint") {
//        std::cout << "[Step 1] Found CommonPoint at 0x" << std::hex << die.offset << std::dec << std::endl;
//
//        bool hasAddressInfo = false;
//        for (const auto& [attr, val] : die.attributes) {
//          if (attr == 0x11 || attr == 0x12 || attr == 0x40 || attr == 0x02 || attr == 0x55) {
//            std::cout << "  [Filter] Has Address Info: 0x" << std::hex << attr << std::dec << std::endl;
//            hasAddressInfo = true; break;
//          }
//        }
//        if (!hasAddressInfo) {
//          std::cout << "  [Success] Passed filter, adding to map..." << std::endl;
//          groups[&die].push_back({cuIdx, die.offset, &die});
//        }
//      }
//    }
//    for (const auto& child : die.children) collectDuplicatesRecursive(cuIdx, child, groups);
//  }

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
        if (attr.first == 0x10 || attr.first == 0x1b || attr.first == 0x72)
          puRoot.attributes.push_back(attr);
      }
    }

    // 3. 预留空间 (非常重要！防止后面 push_back 导致 master 搬家)
    puRoot.children.reserve(dups.size());
    for (auto& [tpl, instances] : dups) {
      DIE masterCopy = *tpl;
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
    // =========================================================
    // 阶段 0: 预处理 Forms (Pre-calculation)
    // 必须在生成 Abbrev Key 之前完成 Form 的升级，否则 Abbrev 表会记录错误的 ref4
    // =========================================================
    for (size_t i = 0; i < cuDIEs.size(); ++i) {
      uint64_t currentUintOldBase = cuHeaders[i].oldOffset;

      std::function<void(DIE&)> upgradeForms = [&](DIE& die) {
        if (die.isRemoved) return;

        for (auto& [attrId, attrVal] : die.attributes) {
          // 检查是否是引用类型的 Form
          if (isReferenceForm(attrVal.form)) {
            // 跳过我们要手动处理的 import (它已经是 ref_addr 了)
            if (attrId == 0x18 /* DW_AT_import */) continue;

            uint64_t targetOldOffset;
            // 计算目标的原始偏移量
            if (attrVal.form == 0x10) { // ref_addr
              targetOldOffset = attrVal.value;
            } else { // ref4, ref_udata 等相对偏移
              targetOldOffset = currentUintOldBase + attrVal.value;
            }

            // 查表看目标去哪了
            if (oldOffsetToDieMap.count(targetOldOffset)) {
              DIE* targetDie = oldOffsetToDieMap[targetOldOffset];
              // 如果目标 DIE 被移除且指向了 Master (说明在 PU 中)
              if (targetDie->isRemoved && targetDie->linkToMaster) {
                // 【关键修改】立即升级 Form 为 ref_addr (0x10)
                attrVal.form = 0x10;
                // 【关键修改】清空 rawBytes，强迫 Writer 重新编码，
                // 否则 writeDataTo 可能会直接写出旧的 rawBytes
                attrVal.rawBytes.clear();
              }
            }
          }
        }
        for (auto& child : die.children) upgradeForms(child);
      };

      for (auto& root : cuDIEs[i]) upgradeForms(root);
    }

    // =========================================================
    // 阶段 A: 全局重建 Abbrev 表 (去重)
    // =========================================================
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

    // =========================================================
    // 阶段 B: 分配新偏移量 (Layout)
    // =========================================================
    uint64_t totalSectionSize = 0;
    for (size_t i = 0; i < cuHeaders.size(); ++i) {
      auto& h = cuHeaders[i];
      h.offset = totalSectionSize;
      h.abbrevOffset = 0;
      uint64_t unitOffset = 12; // Header size (DWARF32)
      if (h.dwoId.has_value()) unitOffset += 8;

      std::function<void(DIE&)> doLayout = [&](DIE& die) {
        if (die.isRemoved) return;
        die.offset = h.offset + unitOffset;
        die.abbrevCode = getAbbrevCode(die);
        die.abbrevDecl = &newAbbrev.abbrevTables[0][die.abbrevCode];

        unitOffset += llvm::getULEB128Size(die.abbrevCode);

        for (auto& attr : die.attributes) {
          // 如果 rawBytes 为空（新创建的 import 或 刚才被我们清空的 ref_addr），手动计算大小
          if (attr.second.rawBytes.empty()) {
            if (attr.second.form == 0x10) { // ref_addr (DWARF32)
              unitOffset += 4;
            } else {
              // 如果有其他新创建的 Form，这里需要补充 case
              unitOffset += 0;
            }
          } else {
            // 没动过的属性，直接用原来的大小
            unitOffset += attr.second.rawBytes.size();
          }
        }

        for (auto& child : die.children) doLayout(child);
        if (die.abbrevDecl->hasChildren) unitOffset += 1; // 0x00 terminator
      };

      for (auto& root : cuDIEs[i]) doLayout(root);
      h.unitLength = unitOffset - 4;
      totalSectionSize += unitOffset;
    }

    // =========================================================
    // 阶段 C: 核心引用修正 (Fix Values)
    // 注意：不再修改 Form，只修改 Value
    // =========================================================
    DIE* puRootPtr = nullptr;
    if (!cuDIEs.empty() && !cuDIEs[0].empty()) {
      puRootPtr = &cuDIEs[0][0]; // PU 总是位于 Unit 0 的第 0 个 DIE
    }
    uint64_t puRootAddr = reinterpret_cast<uint64_t>(puRootPtr);

    for (size_t i = 0; i < cuDIEs.size(); ++i) {
      uint64_t currentUnitNewBase = cuHeaders[i].offset;
      uint64_t currentUintOldBase = cuHeaders[i].oldOffset;

      std::function<void(DIE&)> fixRefs = [&](DIE& die) {
        if (die.isRemoved) return;

        for (auto& [attrId, attrVal] : die.attributes) {
          // 1. 修正 DW_AT_import
          if (attrId == 0x18 && attrVal.form == 0x10 && puRootPtr != nullptr && attrVal.value == puRootAddr) {
            attrVal.value = puRootPtr->offset;
            continue;
          }

          // 2. 修正常规引用
          if (isReferenceForm(attrVal.form)) {
            uint64_t targetOldOffset;

            // 简便判断：如果 rawBytes 被清空了，说明是我们刚才手动升级的，Value 还是旧的相对值
            bool wasUpgraded = attrVal.rawBytes.empty() && attrVal.form == 0x10;

            if (attrVal.form == 0x10 && !wasUpgraded) {
              targetOldOffset = attrVal.value; // 原生 ref_addr
            } else {
              targetOldOffset = currentUintOldBase + attrVal.value; // 相对值
            }

            if (oldOffsetToDieMap.count(targetOldOffset)) {
              DIE* targetDie = oldOffsetToDieMap[targetOldOffset];

              if (targetDie->isRemoved && targetDie->linkToMaster) {
                // 目标在 PU 中。Form 已经在 Phase 0 变成了 0x10。
                // 直接设置绝对偏移。
                attrVal.value = targetDie->linkToMaster->offset;
              } else {
                // 目标在原处 (或者同 Unit 内移动)。
                if (attrVal.form == 0x10) {
                  attrVal.value = targetDie->offset;
                } else {
                  // 保持 ref4，计算新的相对偏移
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

    // 使用正确计算的大小，不再硬编码 0xa5
    setShSize(totalSectionSize);
  }

//  void finalizeMetadata(DebugAbbrevSection& newAbbrev) {
//    // 阶段 A: 全局重建 Abbrev 表 (去重)
//    newAbbrev.abbrevTables.clear();
//    struct AbbrevKey {
//      uint64_t tag; bool hasChildren;
//      std::vector<std::pair<uint64_t, uint64_t>> layout;
//      bool operator<(const AbbrevKey& o) const {
//        if (tag != o.tag) return tag < o.tag;
//        if (hasChildren != o.hasChildren) return hasChildren < o.hasChildren;
//        return layout < o.layout;
//      }
//    };
//    std::map<AbbrevKey, uint64_t> sigToCode;
//    uint64_t nextCode = 1;
//
//    auto getAbbrevCode = [&](DIE& die) -> uint64_t {
//      AbbrevKey key;
//      key.tag = die.tag;
//      key.hasChildren = !die.children.empty();
//      for (auto& attr : die.attributes) key.layout.push_back({attr.first, attr.second.form});
//      if (sigToCode.find(key) == sigToCode.end()) {
//        uint64_t code = nextCode++;
//        sigToCode[key] = code;
//        DebugAbbrevSection::AbbreviationDecl decl;
//        decl.code = code; decl.tag = key.tag; decl.hasChildren = key.hasChildren;
//        for (auto& l : key.layout) decl.attrForms.push_back({l.first, l.second, std::nullopt});
//        newAbbrev.abbrevTables[0][code] = std::move(decl);
//      }
//      return sigToCode[key];
//    };
//
//    // 阶段 B: 分配新偏移量 (Layout)
//    uint64_t totalSectionSize = 0;
//    for (size_t i = 0; i < cuHeaders.size(); ++i) {
//      auto& h = cuHeaders[i];
//      h.offset = totalSectionSize;
//      h.abbrevOffset = 0;
//      uint64_t unitOffset = 12;
//      if (h.dwoId.has_value()) unitOffset += 8;
//
//      std::function<void(DIE&)> doLayout = [&](DIE& die) {
//        if (die.isRemoved) return;
//        die.offset = h.offset + unitOffset;
//        die.abbrevCode = getAbbrevCode(die);
//        die.abbrevDecl = &newAbbrev.abbrevTables[0][die.abbrevCode];
//
//        unitOffset += llvm::getULEB128Size(die.abbrevCode);
//        for (auto& attr : die.attributes) {
//          if (attr.second.rawBytes.empty() && attr.second.form == 0x10) {
//            unitOffset += 4;
//          }
//          unitOffset += attr.second.rawBytes.size();
//        }
//        for (auto& child : die.children) doLayout(child);
//        if (die.abbrevDecl->hasChildren) unitOffset += 1; // 0x00 terminator
//      };
//
//      for (auto& root : cuDIEs[i]) doLayout(root);
//      h.unitLength = unitOffset - 4;
//      totalSectionSize += unitOffset;
//    }
//
//    // 阶段 C: 核心引用修正与 Form 升级
//    for (size_t i = 0; i < cuDIEs.size(); ++i) {
//      uint64_t currentUnitNewBase = cuHeaders[i].offset;
//      uint64_t currentUintOldBase = cuHeaders[i].oldOffset;
//
//      std::function<void(DIE&)> fixRefs = [&](DIE& die) {
//        if (die.isRemoved) return;
//
//        for (auto& [attrId, attrVal] : die.attributes) {
//          // 1. 修正 DW_AT_import
//          if (attrId == 0x18 && attrVal.form == 0x10 && attrVal.value > 0xFFFFFF) {
//            DIE* target = reinterpret_cast<DIE*>(attrVal.value);
//            attrVal.value = target->offset;
//            continue;
//          }
//
//          // 2. 修正常规引用 (DW_AT_type等)
//          if (isReferenceForm(attrVal.form)) {
//            uint64_t targetOldOffset;
//            // 注意：如果解析时 ref4 存的是相对偏移，需先加上所在原单元的起始地址
//            // 这里假设 attrVal.value 存的是绝对偏移
//            if (attrVal.form == 0x10) {
//              targetOldOffset = attrVal.value;
//            }
//            else {
//              targetOldOffset = currentUintOldBase + attrVal.value;
//            }
//
//            if (oldOffsetToDieMap.count(targetOldOffset)) {
//              DIE* targetDie = oldOffsetToDieMap[targetOldOffset];
//
//              if (targetDie->isRemoved && targetDie->linkToMaster) {
//                // --- 升级点：引用目标被移到了 PU ---
//                attrVal.value = targetDie->linkToMaster->offset;
//                attrVal.form = 0x10; // 必须升级为 ref_addr，因为 Master 在 Unit 0
//              } else {
//                // --- 目标依然在原处 ---
//                if (attrVal.form == 0x10) {
//                  attrVal.value = targetDie->offset;
//                } else {
//                  attrVal.value = targetDie->offset - currentUnitNewBase;
//                }
//              }
//            }
//          }
//        }
//        for (auto& child : die.children) fixRefs(child);
//      };
//      for (auto& root : cuDIEs[i]) fixRefs(root);
//    }
//    setShSize(0xa5);
//  }

};

} // namespace elf
} // namespace funcv
} // namespace iclang
#endif

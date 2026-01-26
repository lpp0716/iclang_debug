//===--- ReuseSection.hpp - Reuse section ---------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// Reuse text/data section.
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_REUSESECTION_HPP
#define ICLANG_REUSESECTION_HPP

#include "iclang/FuncV/ELF/Reuse/BDG.hpp"

namespace iclang {
namespace funcv {
namespace elf {
namespace reuse {

class ReuseSection {
  static std::shared_ptr<Section>
  createNewSection(ObjFile &newObjFile,
                   const std::shared_ptr<Section> &oldSection) {
    using Elf_Shdr = llvm::object::ELF64LE::Shdr;

    Elf_Shdr shdr;
    shdr.sh_name = 0;
    shdr.sh_type = oldSection->getShType();
    shdr.sh_flags = oldSection->getShFlags();
    shdr.sh_addr = 0;
    shdr.sh_offset = 0;
    shdr.sh_size = oldSection->getShSize();
    // This function is only for text/data sections, just set link/info to 0.
    shdr.sh_link = 0;
    shdr.sh_info = 0;
    shdr.sh_addralign = oldSection->getShAddralign();
    shdr.sh_entsize = oldSection->getEntSize();

    // Create new section.
    // Set data. (shadow copy)
    const auto newSection =
        std::make_shared<OrdinarySection>(&shdr, oldSection->getData());

    // Create new idx ref.
    newSection->setIdx(
        std::make_shared<IdxRef>(newObjFile.getSections().size()));

    // Create new str ref.
    const auto name = oldSection->getNameValue();
    newSection->setName(newObjFile.getShstrTab()->push_back(name));

    newObjFile.getSections().push_back(newSection);

    return newSection;
  }

public:
  static void run(ObjFile &newObjFile, const BDG &bdg) {
    const auto &funcVReuseNodes = bdg.getFuncVReuseNodes();

    // Old section -> new section.
    std::unordered_map<std::shared_ptr<Section>, std::shared_ptr<Section>>
        visited;

    for (const auto &p : funcVReuseNodes) {
      const auto reuseNode = p.second;

      const auto oldSection = reuseNode->getOldSection();
      if (oldSection == nullptr || reuseNode->getNewSection() != nullptr) {
        continue;
      }

      const auto it = visited.find(oldSection);
      if (it != visited.end()) {
        reuseNode->setNewSection(it->second);
        continue;
      }

      const auto newSection = createNewSection(newObjFile, oldSection);
      reuseNode->setNewSection(newSection);
      visited.emplace(oldSection, newSection);
    }
  }
};

} // namespace reuse
} // namespace elf
} // namespace funcv
} // namespace iclang

#endif // ICLANG_REUSESECTION_HPP

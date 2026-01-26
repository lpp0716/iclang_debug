//===--- Reuse.hpp - FuncV reuse ------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// Workflow:
// * Build BDG.
// * Reuse sections.
// * Reuse symbols.
// * Reuse eh_frame.
// * Reuse relocations.
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_REUSE_HPP
#define ICLANG_REUSE_HPP

#include "iclang/FuncV/ELF/Reuse/BDG.hpp"
#include "iclang/FuncV/ELF/Reuse/ReuseEhFrame.hpp"
#include "iclang/FuncV/ELF/Reuse/ReuseRelocation.hpp"
#include "iclang/FuncV/ELF/Reuse/ReuseSection.hpp"
#include "iclang/FuncV/ELF/Reuse/ReuseSymbol.hpp"

namespace iclang {
namespace funcv {
namespace elf {
namespace reuse {

class Reuse {
  ObjFile &oldObjFile;
  ObjFile &newObjFile;
  const std::unordered_set<std::string> &funcXSet;

public:
  Reuse(ObjFile &_oldObjFile, ObjFile &_newObjFile,
        const std::unordered_set<std::string> &_funcXSet)
      : oldObjFile(_oldObjFile), newObjFile(_newObjFile), funcXSet(_funcXSet) {}

  void run() {
    // (1) Build BDG.
    BDG bdg;
    bdg.build(oldObjFile, newObjFile);
    bdg.propagation(funcXSet);

    // llvm::errs() << "funcv reuse nodes:\n";
    // for (auto p: bdg.getFuncVReuseNodes()) {
    //   llvm::errs() << p.first << "\n";
    // }

    // (2) Reuse sections.
    ReuseSection::run(newObjFile, bdg);

    // llvm::errs() << "New obj file:\n" << newObjFile.toString() << "\n";

    // (3) Reuse symbols.
    ReuseSymbol::run(newObjFile, bdg);

    // llvm::errs() << "New obj file:\n" << newObjFile.toString() << "\n";

    // (4) Reuse eh_frame.
    ReuseEhFrame::run(newObjFile, bdg);

    // llvm::errs() << "New obj file:\n" << newObjFile.toString() << "\n";
    // llvm::errs() << "BDG:\n";
    // llvm::errs() << bdg.toString() << "\n";

    // (5) Reuse relocations.
    ReuseRelocation::run(newObjFile, bdg);

    // llvm::errs() << "New obj file:\n" << newObjFile.toString() << "\n";
  }
};

} // namespace reuse
} // namespace elf
} // namespace funcv
} // namespace iclang

#endif // ICLANG_REUSE_HPP

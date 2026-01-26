//===--- FuncV.hpp - FuncV -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// Workflow:
// * Init old/new obj file.
// * Reuse binary code form old obj file to new obj file according to funcXSet.
// * Fini new obj file.
// * Write new obj file.
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_FUNCV_HPP
#define ICLANG_FUNCV_HPP

#include "iclang/FuncV/ELF/ObjFile.hpp"
#include "iclang/FuncV/ELF/Reuse/Reuse.hpp"

namespace iclang {
namespace funcv {
namespace elf {

class FuncV {
private:
  static bool startsWith(const char *str, const char *prefix) {
    const size_t prefixLen = std::strlen(prefix);
    return std::strncmp(str, prefix, prefixLen) == 0;
  }

  static std::pair<unsigned char, unsigned char>
  getElfArchType(const BinFile &binFile) {
    if (binFile.getFileSize() < llvm::ELF::EI_NIDENT) {
      return std::make_pair(static_cast<uint8_t>(llvm::ELF::ELFCLASSNONE),
                            static_cast<uint8_t>(llvm::ELF::ELFDATANONE));
    }
    const char *object = binFile.readBytes(0);
    return std::make_pair(static_cast<uint8_t>(object[llvm::ELF::EI_CLASS]),
                          static_cast<uint8_t>(object[llvm::ELF::EI_DATA]));
  }

  static ELFKind getELFKind(const BinFile &binFile) {
    const auto &logger = Logger::getInstance();

    const auto pr = getElfArchType(binFile);
    const auto size = pr.first;
    const auto endian = pr.second;

    const char *object = binFile.readBytes(0);
    if (!startsWith(object, llvm::ELF::ElfMagic)) {
      logger.fatal(binFile.getFilePath() + " is not an ELF file");
    }
    if (endian != llvm::ELF::ELFDATA2LSB && endian != llvm::ELF::ELFDATA2MSB) {
      logger.fatal(binFile.getFilePath() + " is a corrupted ELF file: "
                                           "invalid data encoding");
    }
    if (size != llvm::ELF::ELFCLASS32 && size != llvm::ELF::ELFCLASS64) {
      logger.fatal(binFile.getFilePath() + " is a corrupted ELF file: "
                                           "invalid file class");
    }

    const size_t fileSize = binFile.getFileSize();
    if ((size == llvm::ELF::ELFCLASS32 &&
         fileSize < sizeof(llvm::ELF::Elf32_Ehdr)) ||
        (size == llvm::ELF::ELFCLASS64 &&
         fileSize < sizeof(llvm::ELF::Elf64_Ehdr))) {
      logger.fatal(binFile.getFilePath() + " is a corrupted ELF file: "
                                           "file is too short");
    }

    if (size == llvm::ELF::ELFCLASS32) {
      return (endian == llvm::ELF::ELFDATA2LSB) ? ELFKind::ELF32LEKind
                                                : ELFKind::ELF32BEKind;
    }
    return (endian == llvm::ELF::ELFDATA2LSB) ? ELFKind::ELF64LEKind
                                              : ELFKind::ELF64BEKind;
  }

public:
  // Load symbol table for FuncX.
  static std::unordered_set<std::string>
  onlyLoadSymbolTable(const std::string &objPath) {
    const BinFile binFile(objPath);

    const ELFKind kind = getELFKind(binFile);

    std::unordered_set<std::string> res;

    if (kind != ELFKind::ELF64LEKind) {
      return res;
    }

    ObjFile objFile(binFile);

    // Parse ELF header and sections.
    objFile.init();

    // Load symbol table.
    const auto &symbols = objFile.getSymTab()->getSymbols();
    for (const auto &symbol : symbols) {
      const auto type = symbol->getStType();
      const auto bind = symbol->getStBind();
      const auto name = symbol->getNameValue();
      if (type == llvm::ELF::STT_FUNC &&
          (bind == llvm::ELF::STB_GLOBAL || bind == llvm::ELF::STB_WEAK) &&
          !name.empty()) {
        res.insert(name);
      }
    }

    return res;
  }

  static void run(const std::string &oldObjPath, const std::string &newObjPath,
                  const std::string &outputPath,
                  const std::unordered_set<std::string> &funcXSet,
                  bool dumpOutput = false) {
    auto &logger = Logger::getInstance();

    if (oldObjPath == newObjPath || oldObjPath == outputPath ||
        newObjPath == outputPath) {
      logger.fatal("old/new/merge overlap");
    }

    const BinFile oldBinFile(oldObjPath);
    const BinFile newBinFile(newObjPath);

    const ELFKind oldELFKind = getELFKind(oldBinFile);
    const ELFKind newELFKind = getELFKind(newBinFile);
    if (oldELFKind != newELFKind) {
      logger.fatal("old ELF kind != new ELF kind");
    }

    if (newELFKind != ELFKind::ELF64LEKind) {
      logger.fatal("We only support ELF64LEKind");
    }

    ObjFile oldObjFile(oldBinFile);
    ObjFile newObjFile(newBinFile);

    // Parse ELF header and sections.
    oldObjFile.init();
    newObjFile.init();

    auto reuseDriver = reuse::Reuse(oldObjFile, newObjFile, funcXSet);
    reuseDriver.run();

    // Reset layout.
    newObjFile.fini();

    // Dump
    // llvm::errs() << "Old obj file:\n" << oldObjFile.toString() << "\n";
    // llvm::errs() << "New obj file:\n" << newObjFile.toString() << "\n";

    if (dumpOutput) {
      llvm::errs() << "New obj file:\n" << newObjFile.toString() << "\n";
    }

    newObjFile.save(outputPath);
  }
};

} // namespace elf
} // namespace funcv
} // namespace iclang

#endif // ICLANG_FUNCV_HPP

#include "illvm/FuncV/ELF/FuncV.h"

#include "illvm/Support/Diagnostics.h"

namespace illvm {
namespace funcv {
namespace elf {

bool FuncV::startsWith(const char *str, const char *prefix) {
  const size_t prefixLen = std::strlen(prefix);
  return std::strncmp(str, prefix, prefixLen) == 0;
}

std::pair<unsigned char, unsigned char>
FuncV::getElfArchType(const BinFile &binFile) {
  if (binFile.getFileSize() < llvm::ELF::EI_NIDENT) {
    return std::make_pair(static_cast<uint8_t>(llvm::ELF::ELFCLASSNONE),
                          static_cast<uint8_t>(llvm::ELF::ELFDATANONE));
  }
  const auto *object = binFile.readBytes(0);
  return std::make_pair(static_cast<uint8_t>(object[llvm::ELF::EI_CLASS]),
                        static_cast<uint8_t>(object[llvm::ELF::EI_DATA]));
}

void FuncV::check(const BinFile &binFile) {
  const auto pr = getElfArchType(binFile);
  const auto size = pr.first;
  const auto endian = pr.second;

  const auto *object = binFile.readBytes(0);
  ILLVM_FCHECK(
      startsWith(reinterpret_cast<const char *>(object), llvm::ELF::ElfMagic),
      binFile.getFilePath() + " is not an ELF file");
  ILLVM_FCHECK(endian == llvm::ELF::ELFDATA2LSB,
               binFile.getFilePath() + " is not LE");
  ILLVM_FCHECK(size == llvm::ELF::ELFCLASS64,
               binFile.getFilePath() + " is not 64");
  const size_t fileSize = binFile.getFileSize();
  ILLVM_FCHECK(fileSize >= sizeof(llvm::ELF::Elf64_Ehdr),
               binFile.getFilePath() + " is a corrupted ELF file: "
                                       "file is too short");
}

llvm::Expected<ObjFile> FuncV::loadObjFile(const std::string &objPath) {
  const BinFile binFile(objPath);

  check(binFile);

  return ObjFile::create(binFile);
}

llvm::Expected<std::unordered_set<std::string>>
FuncV::onlyLoadSymbolTable(const std::string &objPath) {
  auto objFileOrErr = loadObjFile(objPath);
  ILLVM_ETRANS(objFileOrErr.takeError());
  const auto &objFile = *objFileOrErr;

  std::unordered_set<std::string> res;

  // Load symbol table.
  const auto &symbols = objFile.getSymTabSec()->getSymbols();
  for (const auto &symbol : symbols) {
    if (symbol->isFunc() && (symbol->isGlobal() || symbol->isWeak())) {
      res.insert(symbol->getName());
    }
  }

  return res;
}

llvm::Error FuncV::run(const std::string &oldObjPath, const std::string &newObjPath,
                const std::string &outputPath,
                const std::unordered_set<std::string> &funcXSet) {
  ILLVM_FCHECK(oldObjPath != newObjPath, "");

  // auto oldObjFileOrErr = loadObjFile(oldObjPath);
  // ILLVM_ETRANS(oldObjFileOrErr.takeError());
  // auto &oldObjFile = *oldObjFileOrErr;
  auto newObjFileOrErr = loadObjFile(newObjPath);
  ILLVM_ETRANS(newObjFileOrErr.takeError());
  auto &newObjFile = *newObjFileOrErr;

  // auto reuseDriver = Reuse(oldObjFile, newObjFile, funcXSet);
  // if (auto err = reuseDriver.run()) {
  //   return err;
  // }

  newObjFile.layout();

  newObjFile.write(outputPath);

  return llvm::Error::success();
}

} // namespace elf
} // namespace funcv
} // namespace illvm

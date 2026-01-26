#include "iclang/FuncV/ELF/ObjFile.hpp"
#include "iclang/Global.hpp"

using namespace iclang;
using namespace iclang::funcv::elf;

static bool startsWith(const char* str, const char* prefix) {
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
  auto &logger = Logger::getInstance();

  auto pr = getElfArchType(binFile);
  auto size = pr.first;
  auto endian = pr.second;

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
    return (endian == llvm::ELF::ELFDATA2LSB) ? ELFKind::ELF32LEKind : ELFKind::ELF32BEKind;
  }
  return (endian == llvm::ELF::ELFDATA2LSB) ? ELFKind::ELF64LEKind : ELFKind::ELF64BEKind;
}

// iclang-dwarf <input> <output>
int main(int argc, char **argv) {
  if (argc != 3) {
    llvm::errs() << "Usage: iclang-dwarf <input> <output>\n";
    exit(1);
  }
  std::string inputPath = argv[1];
  std::string outputPath = argv[2];

  BinFile binFile(inputPath);

  const ELFKind elfKind = getELFKind(binFile);
  if (elfKind != ELFKind::ELF64LEKind) {
    llvm::errs() << "We only support ELF64LEKind\n";
    exit(1);
  }

  ObjFile objFile(binFile);
  objFile.init();

  std::ostringstream oss;
  objFile.dump(oss);
  llvm::errs() << oss.str() << "\n";

  objFile.fini();

  objFile.save(outputPath);

  return 0;
}
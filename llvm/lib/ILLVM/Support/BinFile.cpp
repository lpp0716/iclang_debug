#include "illvm/Support/BinFile.h"

#include "illvm/Support/Diagnostics.h"

namespace illvm {

BinFile::BinFile(const std::string &path)
    : filePath(path), fileData(nullptr), fileSize(0) {
  std::ifstream file(filePath, std::ios::binary | std::ios::ate);
  ILLVM_FCHECK(file.is_open(), "Failed to open binary file: " + filePath);
  fileSize = file.tellg();
  file.seekg(0, std::ios::beg);

  fileData = malloc(fileSize);
  ILLVM_FCHECK(fileData != nullptr,
               "Malloc failed for binary file: " + filePath);

  ILLVM_FCHECK(file.read(static_cast<char *>(fileData), fileSize),
               "Failed to read binary file: " + filePath);

  file.close();
}

const uint8_t *BinFile::readBytes(const size_t offset) const {
  assert(offset < static_cast<size_t>(fileSize));
  return static_cast<uint8_t *>(fileData) + offset;
}

} // namespace illvm

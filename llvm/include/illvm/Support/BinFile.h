//===--- BinFile.h - Binary file -----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// Binary file.
//
//===----------------------------------------------------------------------===/

#ifndef ILLVM_BINFILE_HPP
#define ILLVM_BINFILE_HPP

#include <cstdint>
#include <fstream>
#include <string>

#include "llvm/Support/Error.h"

namespace illvm {

class BinFile {
private:
  std::string filePath;

  void *fileData;

  std::streamsize fileSize;

public:
  explicit BinFile(const std::string &path);

  BinFile(const BinFile &) = delete;
  BinFile& operator=(const BinFile &) = delete;
  BinFile(BinFile &&) = default;

  ~BinFile() {
    if (fileData) {
      free(fileData);
    }
  }

  std::streamsize getFileSize() const { return fileSize; }

  const std::string &getFilePath() const { return filePath; }

  const uint8_t *readBytes(const size_t offset) const;
};

} // namespace illvm

#endif //ILLVM_BINFILE_HPP

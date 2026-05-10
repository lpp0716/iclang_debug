//===--- FileSystem.h - ILLVM file system utils ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// ILLVM file system utils.
//
//===----------------------------------------------------------------------===/

#ifndef ILLVM_FILESYSTEM_H
#define ILLVM_FILESYSTEM_H

#include <string>
#include <unordered_set>
#include <vector>

#include "llvm/Support/Error.h"

namespace illvm {

class FileSystem {
public:
  static std::string getCurrentPath();

  // Auto remove dot.
  static std::string toAbsPath(const std::string &filepath);

  static std::string linkPath(const std::string &path1,
                              const std::string &path2);

  static std::string parentPath(const std::string &path);

  static bool checkFileExists(const std::string &filepath);

  static long long
  getLastModificationTime(const std::string &filepath);

  static std::string readAll(const std::string &filepath);

  static std::vector<std::string>
  readLines(const std::string &filepath);

  // Note: check res.size() == n yourself.
  static std::vector<std::string>
  readFirstNLines(const std::string &filepath, const size_t n);

  // Return error when the dirpath exists.
  static llvm::Error mkdir(const std::string &dirpath);

  // rm -rf.
  static void rmFile(const std::string &filepath);

  // Auto cover.
  // Support mv directory.
  static void mvFile(const std::string &from, const std::string &to);

  // Auto cover.
  // Do not support cp -r yet.
  static void cpFile(const std::string &from, const std::string &to);

  static void saveStr(const std::string &filepath,
                             const std::string &str);

  static void saveVector(const std::string &filepath,
                                const std::vector<std::string> &vec);

  static void saveSet(const std::string &filepath,
                             const std::unordered_set<std::string> &st,
                             const bool ordered = true);
};

} // namespace illvm

#endif // ILLVM_FILESYSTEM_H

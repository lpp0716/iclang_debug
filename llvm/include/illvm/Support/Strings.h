//===--- Strings.h - ILLVM string utils ----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// ILLVM string utils.
//
//===----------------------------------------------------------------------===/

#ifndef ILLVM_STRINGS_H
#define ILLVM_STRINGS_H

#include <string>

#include "llvm/ADT/SmallVector.h"

namespace illvm {

class Strings {
public:
  static std::string trimWhitespace(const std::string &str);

  static bool hasPrefix(const std::string &str, const std::string &prefix);

  static std::string
  argVToArgs(const llvm::SmallVector<const char *, 128> &argv);
};

} // namespace illvm

#endif // ILLVM_STRINGS_H

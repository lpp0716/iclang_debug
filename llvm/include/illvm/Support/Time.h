//===--- Time.h - ILLVM time utils ---------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// ILLVM time utils.
//
//===----------------------------------------------------------------------===/

#ifndef ILLVM_TIME_H
#define ILLVM_TIME_H

#include <string>

namespace illvm {

class Time {
public:
  static long long currentTsMs();

  static std::string currentDateTime();
};

} // namespace illvm

#endif // ILLVM_TIME_H

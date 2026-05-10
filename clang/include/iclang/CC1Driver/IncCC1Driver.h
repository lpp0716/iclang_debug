//===--- IncCC1Driver.h - IClang cc1 driver for inc compilation ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// IClang cc1 driver for inc compilation.
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_INCCC1DRIVER_H
#define ICLANG_INCCC1DRIVER_H

#include <string>
#include <vector>

namespace iclang {

class IncCC1Driver {
public:
  static void run();
};

class IncCheckCC1Driver {
public:
  static void run();
};

} // namespace iclang

#endif //ICLANG_INCCC1DRIVER_H

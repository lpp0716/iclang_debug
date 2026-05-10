//===--- Driver.h - IClang driver -----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// IClang driver.
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_DRIVER_H
#define ICLANG_DRIVER_H

#include "clang/Driver/Action.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/InputInfo.h"

namespace iclang {

class Driver {
public:
  static int run(const clang::driver::Action::ActionClass &kind,
                 const std::vector<clang::driver::InputInfo> &inputInfos,
                 const std::vector<std::string> &outputFilenames,
                 const llvm::SmallVector<const char *, 128> &originalArgv,
                 const clang::driver::Driver &clangDriver);
};

} // namespace iclang

#endif //ICLANG_DRIVER_H

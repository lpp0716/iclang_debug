//===--- ShareDriver.h - IClang shared compilation driver -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// * ShareMasterMode
// * ShareClientMode
// * ShareTestMode
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_SHAREDRIVER_H
#define ICLANG_SHAREDRIVER_H

#include "iclang/Driver/DriverBase.h"

namespace iclang {

class ShareMasterDriver {
public:
  static int run(Global &global,
                 const llvm::SmallVector<const char *, 128> &originalArgv,
                 const clang::driver::Driver &clangDriver);
};

class ShareClientDriver {
public:
  static int run(Global &global,
                 const llvm::SmallVector<const char *, 128> &originalArgv,
                 const clang::driver::Driver &clangDriver);
};

class ShareCheckDriver {
public:
  static int run(Global &global,
                 const llvm::SmallVector<const char *, 128> &originalArgv,
                 const clang::driver::Driver &clangDriver);
};

} // namespace iclang

#endif //ICLANG_SHAREDRIVER_H

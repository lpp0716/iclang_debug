//===--- DriverBase.h - IClang basic driver utils ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// IClang basic driver utils.
// Init IClang:
// * Parse IClang args.
// * Record start time stamp.
// * Basic check:
//   * AssembleJob, 1 input, 1 output, 1 emit-obj.
//   * Important: concurrent compilation, only one can work
//     (mkdir .iclangtmp).
// * Config IClang basic path.
// * return false: Back to Clang.
//
//===----------------------------------------------------------------------===/
#ifndef ICLANG_DRIVERBASE_H
#define ICLANG_DRIVERBASE_H

#include <string>
#include <vector>
#include <unordered_map>

#include "iclang/Support/Global.h"

#include "clang/Driver/Action.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/InputInfo.h"

namespace iclang {

class DriverBase {
public:
  // Return false: back to Clang.
  static bool init(Global &global,
                   const clang::driver::Action::ActionClass &kind,
                   const std::vector<clang::driver::InputInfo> &inputInfos,
                   const std::vector<std::string> &outputFilenames,
                   const llvm::SmallVector<const char *, 128> &originalArgv);

  // If you do not want to change input/output/emit,
  // please set inputIdx, outputIdx, emitIdx to -1.
  // skipArgs: <argName, arg num following argName>.
  // For example:
  // * -x c : <-x, 1>.
  // * -O3 : <-O3, 0>.
  static int compile(const clang::driver::Driver &clangDriver,
                     const llvm::SmallVector<const char *, 128> &originalArgv,
                     const int inputIdx, const char *newInputPath,
                     const int outputIdx, const char *newOutputPath,
                     const int emitIdx, const char *newEmit,
                     const std::unordered_map<std::string, int> &skipArgs,
                     const std::vector<const char *> &newArgs);

  static int
  clangCompile(const clang::driver::Driver &clangDriver,
               const llvm::SmallVector<const char *, 128> &originalArgv);

  static void fini(Global &global);

  static int recover(Global &global, const clang::driver::Driver &clangDriver,
                     const llvm::SmallVector<const char *, 128> &originalArgv);

  static int runBase(Global &global,
                     const llvm::SmallVector<const char *, 128> &originalArgv,
                     const clang::driver::Driver &clangDriver);
};

} // namespace iclang

#endif //ICLANG_DRIVERBASE_H

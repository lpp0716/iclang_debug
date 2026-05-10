//===--- IncDriver.h - IClang incremental compilation driver -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// * IncMode
// * IncTestMode
//
// Workflow:
// ---------- Driver ----------
// Step1. Init:
// * DriverBase init.
// * Ext check, config paths.
//
// Step2. Determine inc mode:
//   * .iclang/compile.json exists.
//   * Previous output exists.
//   * The last modified timestamp of previous output <= that of
//     .iclang/compile.json.
//   * Previous compilation command == current compilation command.
//   * The top include region remains unchanged.
//   * The timestamp of all header files remains unchanged.
//   * recoverFlag disable (If the recoverFlag is enabled, continue it).
//
// Step3. Build cache (incFlag):
// * If .iclang/iclang.cache or .iclang/iclang.h does not exist, build them:
//   * Save top include region in .iclang/iclang.h.
//   * -x c++-header -o .iclang/iclang.cache ./iclang/iclang.h.
//   (remove build dependency options)
// * Set skipTopIncludeRegion true.
// * If IncTestMode: Save hackedMainBuffer to .iclangtmp/iclang.cpp. (cc1)
//
// Step4. Load prev binary symbol table (incFlag):
// * mv previous output to iclangtmp/prev.o.
// * Load symbol table from iclangtmp/prev.o to prevAPIs.
//
// Step5. Compilation:
// * If incFlag:
//   * inputFile + .iclang/iclang.cache -> outputFile.
//   Failed:
//   * if !IncTestMode: rm .iclangtmp/prev.o.
//   * iClangFlag = false.
//   * res = Clang compilation: inputFile -> outputFile.
//   * if res == 0: recoverFlag = true.
//   * goto Step10. Fini.
// * Else: Clang compilation: inputFile -> outputFile.
// Note:
// * Multiple calls to RunSafely can result in assertion errors.
//   ```
//   CrashRecoveryContext::RunSafely
//   assert(!Impl && "Crash recovery context already initialized!");
//   ```
// * When compile error, Clang will rm the output file.
//   Therefore, backing up cache is meaningless.
//
// ---------- CC1 ----------
//
// Step6. Record top include region and header timestamp (!incFlag).
//
// Step7. FuncX (incFlag):
// * Record funcXTime.
// * Perform funcx analysis.
// * Remove reusable functions from pendingInstQueue, add them to funcXSet.
// * If IncTestMode: save funcXSet to .iclangtmp/funcx.txt.
//
// ---------- Driver ----------
//
// Step8. Post processing cache (incFlag):
// * mv .iclang/iclang.h .iclangtmp/iclang.h.
// * mv .iclang/iclang.cache .iclangtmp/iclang.cache.
//
// Step9. FuncV (incFlag):
// * Record funcVTime.
// * If IncTestMode: cp outputFile .iclangtmp/partial.o.
// * .iclangtmp/prev.o + outputFile = outputFile.
// * If IncTestMode: cp outputFile .iclangtmp/output.o
// * If !IncTestMode: rm .iclangtmp/prev.o.
//
// Step10. Fini:
// * DriverBase fini.
//===----------------------------------------------------------------------===/

#ifndef ICLANG_INCDRIVER_H
#define ICLANG_INCDRIVER_H

#include "iclang/Driver/DriverBase.h"

namespace iclang {

class IncDriver {
public:
  static int run(Global &global,
                 const llvm::SmallVector<const char *, 128> &originalArgv,
                 const clang::driver::Driver &clangDriver);
};

class IncCheckDriver {
public:
  static int run(Global &global,
                 const llvm::SmallVector<const char *, 128> &originalArgv,
                 const clang::driver::Driver &clangDriver);
};

} // namespace iclang

#endif // ICLANG_INCDRIVER_H

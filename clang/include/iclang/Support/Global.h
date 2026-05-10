//===--- Global.h - IClang global data ------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// Sharing data between driver and cc1.
//
// iClangMode:
// * "Inc": function-level incremental compilation.
// * "IncCheck": inc check mode for IClang developers.
// * "ShareMaster": master mode of shared compilation optimization.
// * "ShareClient": client mode of shared compilation optimization.
// * "ShareCheck": share check mode for IClang developers.
// * "LineMacroCheck": Check line macro.
// * "Dump": AST dump mode.
// * "Profile": profile Clang.
// * "Clang": default, equivalent to Clang.
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_GLOBAL_H
#define ICLANG_GLOBAL_H

#include <memory>
#include <string>

#include "iclang/Support/MetaData.h"

#include "illvm/Support/Diagnostics.h"
#include "illvm/Support/Memory.h"

namespace iclang {

enum class IClangMode {
  IncMode,
  IncCheckMode,
  IncLineCheckMode,
  ShareMasterMode,
  ShareClientMode,
  ShareCheckMode,
  DumpMode,
  LineMacroCheckMode,
  ProfileMode,
  RedundantSkipMode,
  ClangMode
};

class ClangModeScope;

class Global {
private:
  IClangMode iClangMode = IClangMode::ClangMode;
  illvm::OPtr<MetaData> metaData;

  Global() {}

public:
  std::string outputPath;
  friend ClangModeScope;

  Global(const Global &) = delete;
  Global &operator=(const Global &) = delete;

  static Global &getInstance() {
    static Global instance;
    return instance;
  }

  IClangMode getIClangMode() const { return iClangMode; }

  bool isIClangMode(const IClangMode _iClangMode) const {
    if (iClangMode == IClangMode::IncCheckMode &&
        _iClangMode == IClangMode::IncMode) {
      return true;
    }
    return iClangMode == _iClangMode;
  }

  // Back to Clang.
  void resetIClangMode() { iClangMode = IClangMode::ClangMode; }

  static illvm::OPtr<MetaData>
  createMetaData(const IClangMode iClangMode) {
    illvm::OPtr<MetaData> metaData;
    if (iClangMode == IClangMode::IncMode) {
      metaData = illvm::make_owner<IncMetaData>().moveTo<MetaData>();
    } else if (iClangMode == IClangMode::IncCheckMode) {
      metaData = illvm::make_owner<IncCheckMetaData>().moveTo<MetaData>();
    } else if (iClangMode == IClangMode::IncLineCheckMode) {
      metaData = illvm::make_owner<IncLineCheckMetaData>().moveTo<MetaData>();
    } else if (iClangMode == IClangMode::ShareMasterMode) {
      metaData = illvm::make_owner<ShareMasterMetaData>().moveTo<MetaData>();
    } else if (iClangMode == IClangMode::ShareClientMode) {
      metaData = illvm::make_owner<ShareClientMetaData>().moveTo<MetaData>();
    } else if (iClangMode == IClangMode::ShareCheckMode) {
      metaData = illvm::make_owner<ShareCheckMetaData>().moveTo<MetaData>();
    } else if (iClangMode == IClangMode::LineMacroCheckMode) {
      metaData = illvm::make_owner<LineMacroCheckMetaData>().moveTo<MetaData>();
    } else if (iClangMode == IClangMode::DumpMode) {
      metaData = illvm::make_owner<DumpMetaData>().moveTo<MetaData>();
    } else if (iClangMode == IClangMode::ProfileMode) {
      metaData = illvm::make_owner<ProfileMetaData>().moveTo<MetaData>();
    } else if (iClangMode == IClangMode::RedundantSkipMode) {
      metaData = illvm::make_owner<MetaData>().moveTo<MetaData>();
    } else {
      metaData = illvm::make_owner<MetaData>();
    }
    return metaData;
  }

  template<typename T>
  illvm::BPtr<T> getMetaData() {
    return metaData.borrow().copyTo<T>();
  }

  void init(const std::string &iClangModeStr) {
    if (iClangModeStr == "Inc") {
      iClangMode = IClangMode::IncMode;
    } else if (iClangModeStr == "IncCheck") {
      iClangMode = IClangMode::IncCheckMode;
    } else if (iClangModeStr == "IncLineCheck") {
      iClangMode = IClangMode::IncLineCheckMode;
    } else if (iClangModeStr == "ShareMaster") {
      iClangMode = IClangMode::ShareMasterMode;
    } else if (iClangModeStr == "ShareClient") {
      iClangMode = IClangMode::ShareClientMode;
    } else if (iClangModeStr == "ShareTest") {
      iClangMode = IClangMode::ShareCheckMode;
    } else if (iClangModeStr == "LineMacroCheck") {
      iClangMode = IClangMode::LineMacroCheckMode;
    } else if (iClangModeStr == "Dump") {
      iClangMode = IClangMode::DumpMode;
    } else if (iClangModeStr == "Profile") {
      iClangMode = IClangMode::ProfileMode;
    } else if (iClangModeStr == "Clang" || iClangModeStr.empty()) {
      iClangMode = IClangMode::ClangMode;
    } else if (iClangModeStr == "Skip") {
      iClangMode = IClangMode::RedundantSkipMode;
    } else {
      ILLVM_FCHECK(false, "Unknown iClangMode: " + iClangModeStr);
    }

    metaData = createMetaData(iClangMode);
  }

  static void saveMetaDataToFile(const std::string &filepath,
                                 const illvm::BPtr<MetaData> &metaData);

  static illvm::OPtr<MetaData>
  loadMetaDataFromFile(const std::string &filepath, const IClangMode iClangMode);
};

class ClangModeScope {
private:
  IClangMode prevIClangMode;
  Global &global;

public:
  ClangModeScope() = delete;
  explicit ClangModeScope(Global &_global) : global(_global) {
    prevIClangMode = global.getIClangMode();
    global.resetIClangMode();
  }
  ~ClangModeScope() {
    global.iClangMode = prevIClangMode;
  }
};

} // namespace iclang

#endif // ICLANG_GLOBAL_H

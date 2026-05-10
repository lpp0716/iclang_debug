//===--- Global.h - IClang meta compilation data --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// IClang meta compilation data.
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_METADATA_H
#define ICLANG_METADATA_H

#include <string>
#include <unordered_set>
#include <vector>

#include "illvm/Support/Interval.h"
#include "llvm/Support/JSON.h"

namespace iclang {

enum IClangDir {
  PrevDir = 0, // .iclang
  CurDir = 1,  // .iclangtmp
};

class MetaData {
public:
  bool recoverFlag = false;

  std::string recoverReason = "";

  // The current compilation path.
  std::string currentPath = "";

  // Compilation command.
  std::string originalCommand = "";

  // The index of input file in compilation command.
  int inputIdx = -1;

  // The index of output file in compilation command.
  int outputIdx = -1;

  // The index of '-emit-obj' file in compilation command.
  int emitObjIdx = -1;

  // Input file path.
  std::string inputPath = "";

  // Output file path.
  std::string outputPath = "";

  // 0: .iclang 1: .iclangtmp.
  std::string iClangDirPath[2];

  // compile.json.
  std::string compileJsonPath[2];

  // log.txt.
  std::string logPath = "";

  long long totalTimeMs = 0;

  long long frontTimeMs = 0;

  long long backTimeMs = 0;

  long long startTs = 0;

  long long midTs = 0;

  long long endTs = 0;

  // We will intercept Preprocessor::enterSourceFile() and
  // modify the source file buffer
  // without modifying the source file itself.
  std::string hackedMainBuffer = "";

  llvm::StringRef hackedMainBufferRef = "";

  static std::string hackMainBuffer(const std::string &originalBuffer,
                                  const std::vector<std::string> &tir);

  MetaData() = default;

  virtual ~MetaData() = default;

  // Format:
  // recoverFlag
  // recoverReason
  // currentPath
  // originalCommand
  // inputPath
  // outputPath
  // totalTimeMs
  // frontTimeMs
  // backTimeMs
  virtual llvm::json::Object serialize() const;

  virtual void deserialize(llvm::json::Object &root);
};

class IncMetaData : public MetaData {
public:
  // Can be incrementally compiled safely.
  bool incFlag = false;

  std::string cannotIncReason = "";

  std::string iInputDir = "";

  // IClang funcX time.
  long long funcXTime = 0;

  // IClang funcV time.
  long long funcVTime = 0;

  // iclang.cache.
  std::string cachePath[2];

  // iclang.h.
  std::string cacheHeaderPath[2];

  // prev.o.
  std::string prevOPath = "";

  // Code region above the first Decl in the source file.
  // For example:
  // #ifndef ICLANG_GLOBAL_HPP               *|
  // #define ICLANG_GLOBAL_HPP                |
  // #include <stirng>                        |
  // #include <vector>                        |-> top include region
  // #include <map>                           |
  // ...                                     *| // the first "#include" before
  // the first Decl. namespace iclang { // the first Decl
  // ...
  // }
  // #endif ICLANG_GLOBAL_HPP
  std::vector<std::string> topIncludeRegion = {};

  // Headers' timestamp, abs path -> timestamp.
  std::map<std::string, long long> headerTs = {};

  // Symbol api names in previous binary file.
  std::unordered_set<std::string> prevAPIs = {};

  // Mangled names of funcXed symbols.
  std::unordered_set<std::string> funcXSet = {};

  // Enable hackedMainBuffer.
  bool skipTopIncludeRegionFlag = false;

  // MetaData
  // incFlag
  // cannotIncReason
  // funcXTime
  // funcVTime
  // topIncludeRegion
  // headerTs
  llvm::json::Object serialize() const override;

  void deserialize(llvm::json::Object &root) override;
};

class IncCheckMetaData final : public IncMetaData {
public:
  // iclang.cpp.
  std::string cacheSrcPath = "";

  // partial.o.
  std::string partialOPath = "";

  // output.o.
  std::string outputOPath = "";

  // funcx.txt.
  std::string funcXTxtPath = "";
};

class IncLineCheckMetaData : public MetaData {
public:
  const char *lineMacro = "__LINE__";
  const char *iClangLineWrapper = "__ICLW__";
  bool hashHashFlag = false;

  int baseFuncDefNum = 0;

  std::unordered_set<std::string> inValidMacro;
  std::vector<bool> isValidFunctionStack;

  // MetaData
  // hashHashFlag
  // baseFuncDefNum
  llvm::json::Object serialize() const override;

  void deserialize(llvm::json::Object &root) override;
};

class ShareMasterMetaData final : public MetaData {
public:
};

class ShareClientMetaData final : public MetaData {
public:
};

class ShareCheckMetaData final : public MetaData {
public:
  // iclang.i.
  std::string ppPath = "";

  // iclangs.i.
  std::string ppSPath = "";

  long long originalTimeMs = 0;

  long long masterTimeMs = 0;

  long long clientTimeMs = 0;

  long long originalPPLoc = 0;

  long long funcXedPPLoc = 0;

  bool enableRefedSymbolAnalysisFlag = false;

  std::vector<illvm::Interval> unRefedDeclIntervals = {};

  ShareCheckMetaData() = default;

  ~ShareCheckMetaData() override = default;

  // Format:
  // MetaData
  // originalTimeMs
  // masterTimeMs
  // clientTimeMs
  // originalPPLoc
  // funcXedPPLoc
  llvm::json::Object serialize() const override;

  void deserialize(llvm::json::Object &root) override;
};

class LineMacroCheckMetaData final : public MetaData {
public:
  unsigned totalFuncNum = 0;
  unsigned funcWithLineMacroNum = 0;

  // Format:
  // MetaData
  // totalFuncNum
  // funcWithLineMacroNum
  llvm::json::Object serialize() const override;

  void deserialize(llvm::json::Object &root) override;
};

class DumpMetaData final : public MetaData {
public:
};

class ProfileMetaData final : public MetaData {
public:
};

} // namespace iclang

#endif //ICLANG_METADATA_H

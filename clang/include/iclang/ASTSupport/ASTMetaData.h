//===--- ASTMetaData.h - IClang AST meta data ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// IClang AST meta data.
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_ASTMETADATA_H
#define ICLANG_ASTMETADATA_H

#include <unordered_set>

#include "clang/AST/Decl.h"
#include "clang/Sema/Sema.h"

#include <chrono>
#include <system_error>
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Process.h"

namespace iclang {

class ASTMetaData {
public:
};

class IncASTMetaData final : public ASTMetaData {
public:
};

class IncCheckASTMetaData final : public ASTMetaData {
public:
};

class IncLineCheckASTMetaData final : public ASTMetaData {
public:
  static void injectIClangLineWMacro(clang::Sema &sema);

  static void injectIClangLineFunc(clang::Sema &sema);
};

class ShareMasterASTMetaData final : public ASTMetaData {
public:
};

class ShareClientASTMetaData final : public ASTMetaData {
public:
};

class ShareCheckASTMetaData final : public ASTMetaData {
public:
  std::unordered_set<const clang::FunctionDecl *> emitGlobalFuncDefs = {};

  void addEmitGlobalFuncDef(const clang::FunctionDecl *funcDecl);
};

class LineMacroCheckASTMetaData final : public ASTMetaData {
public:
};

class DumpASTMetaData final : public ASTMetaData {
public:
};

class ProfileASTMetaData final : public ASTMetaData {
public:
};

class RedundantSkipASTMetaData  : public ASTMetaData {
public:
  llvm::StringSet<> redundantSymbols;
  llvm::StringSet<> redundantBaseNames;

  std::string md5Name; // 用于生成唯一的报告文件名

  // --- 耗时统计 (毫秒) ---
  double initTimeMs = 0.0;
  double manglingTimeMs = 0.0;
  double lookupTimeMs = 0.0;
  double genTime = 0.0;

  // --- 数量统计 ---
  unsigned totalChecked = 0;       // 总共检查的符号数
  unsigned skippedNormalFuncs = 0; // 跳过的普通函数
  unsigned skippedInlineFuncs = 0; // 跳过的内联函数
  unsigned skippedTemplateInst = 0;// 跳过的模板实例化
  unsigned skippedInCodeGen = 0;
  unsigned skippedTemplateVars = 0;

  bool hasDumped = false; // 防止重复打印

  // 新增：显式调用以输出报告
  void dumpReport() {
    if (hasDumped) return;
    hasDumped = true;

    if (totalChecked > 0 || initTimeMs > 0) {
      // 改用 /tmp 目录，避免权限问题
      std::string reportDir = "/root/iclang_report";
      std::error_code EC;
      llvm::sys::fs::create_directories(reportDir);

//      llvm::errs() << "ICLANG temp num:" << skippedTemplateInst << '\n';
      std::string reportFile = reportDir + "/report_";
      if (!md5Name.empty()) {
        reportFile += md5Name + "_";
      }
      reportFile += std::to_string(llvm::sys::Process::getProcessId()) + ".txt";

      llvm::raw_fd_ostream OS(reportFile, EC, llvm::sys::fs::OF_Text);
      if (!EC) {
        OS << "InitTimeMs: " << initTimeMs << "\n";
        OS << "ManglingTimeMs: " << manglingTimeMs << "\n";
        OS << "LookupTimeMs: " << lookupTimeMs << "\n";
        OS << "GenTime: " << genTime << "\n";
        OS << "TotalChecked: " << totalChecked << "\n";
        OS << "SkippedNormal: " << skippedNormalFuncs << "\n";
        OS << "SkippedInline: " << skippedInlineFuncs << "\n";
        OS << "SkippedTemplate: " << skippedTemplateInst << "\n";
        OS << "SkippedInCodeGen:" << skippedInCodeGen << "\n";
        OS << "SkippedTemplateVars:" << skippedTemplateVars << '\n';
        OS.close();
      } else {
        llvm::errs() << "[IClang] Warning: Failed to write report to " << reportFile << " : " << EC.message() << "\n";
      }
    }
  }

};

} // namespace iclang

#endif // ICLANG_ASTMETADATA_H

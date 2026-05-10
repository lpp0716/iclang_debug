//===--- ReusableInstAnalysis.h - Reusable inst analysis ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// A Decl can be funcxed if it meets the following conditions:
// (1) Necessary checking:
//     * Global Linkage.
//     * Not constructor / destructor (conservative).
//     * Not operator (conservative).
//     * Not virtual (cleaning it may result in virtual table errors).
// (2) MangledName exists in the previous binary.
// (3) AST global function instantiation. (check itself and its context)
//     AST global means that there is no FunctionDecl in its context.
// (4) Self template safe.
//     If it is a function template instantiation,
//     the safeFlag of its template Decl should be true.
//     For member function instantiations, see (6).
// (5) Self instantiation type safe.
//     We will de-alias and recursively extract instantiated types,
//     (only support Type and Integer)
//     and for each type, we will recursively extract CXXRecordDecls within it.
//     (only support Builtin, Pointer, Reference, CXXRecordDecls)
//     Foreach extracted CXXRecordDecl, perform (7).
// (6) Context template, instantiation type safe.
//     For each CXXRecordDecl context, perform (7).
//     There is no need to consider inheritance,
//     as the subclass security means that the parent class must be secure.
// (7) CXXRecordDecl safe recursive checking
//     * AST global.
//     * Self template safe.
//     * Self instantiation type safe.
//     * Context template, instantiation type safe.
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_REUSABLEINSTANALYSIS_HPP
#define ICLANG_REUSABLEINSTANALYSIS_HPP

#include <deque>
#include <unordered_map>

#include "iclang/FuncX/BasicAnalysis.h"

#include "illvm/Support/Memory.h"

namespace iclang {
namespace funcx {

class BasicSafeChecker {
public:
  static bool isMainFileDecl(const clang::SourceManager &sourceManager,
                             const clang::Decl *decl);
};

class TopIncludeRegionVisitor
    : public clang::RecursiveASTVisitor<TopIncludeRegionVisitor> {
private:
  const clang::SourceManager &sourceManager;
  unsigned mainFirstDeclLine = 0;

public:
  explicit TopIncludeRegionVisitor(const clang::ASTContext &context)
      : sourceManager(context.getSourceManager()) {}

  bool TraverseDecl(clang::Decl *decl);

  unsigned getMainFirstDeclLine() const { return mainFirstDeclLine; }
};

class ReusableInstAnalysis {
private:
  illvm::BPtr<IncMetaData> incMetaData;
  ASTGlobal &astGlobal;

  const clang::SourceManager &sourceManager;
  std::unordered_map<const clang::Decl *, bool> checkRecord;

  // Traverse types and extract all included CXXRecordDecl.
  // This function won't parse class specs recursively, you need to parse them
  // yourself. This function only support combinations of Builtin, Pointer,
  // Reference, Decl, and it will return false when encountering any unsupported
  // type. Please ensure that type is de-alised.
  static bool
  classExtraction(const clang::QualType &type,
                  std::unordered_set<const clang::CXXRecordDecl *> &decls);

  bool cache(const clang::CXXRecordDecl *classDecl, const bool res) {
    checkRecord[classDecl] = res;
    return res;
  }

  static bool necessaryCheck(const clang::FunctionDecl *funcDecl);

  bool mangleNameExist(const clang::FunctionDecl *funcDecl) const;

  static bool isGlobalFuncInst(const clang::FunctionDecl *funcDecl);

  bool funcTemplateSafeCheck(const clang::FunctionDecl *funcDecl) const;

  bool classTemplateSafeCheck(const clang::CXXRecordDecl *classDecl) const;

  bool instTypeSafeCheck(const clang::TemplateArgumentList &tempArgList);

  bool funcContextSafeCheck(const clang::FunctionDecl *funcDecl);

  bool classContextSafeCheck(const clang::CXXRecordDecl *classDecl);

  bool checkClass(const clang::CXXRecordDecl *classDecl);

  bool canBeFuncXed(const clang::FunctionDecl *funcDecl);

public:
  using VSPair = std::pair<clang::ValueDecl *, clang::SourceLocation>;

  explicit ReusableInstAnalysis(
      illvm::BPtr<IncMetaData> &&_incMetaData, ASTGlobal &_astGlobal)
      : incMetaData(std::move(_incMetaData)), astGlobal(_astGlobal),
        sourceManager(astGlobal.getSourceManager()) {}

  void run(std::deque<VSPair> &pendingInstQue);
};

} // namespace funcx
} // namespace iclang

#endif // ICLANG_REUSABLEINSTANALYSIS_HPP

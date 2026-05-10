//===--- RefSymbolAnalysis.h - Referenced symbol analysis ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// This module should be executed after building the whole the AST
// (including instantiation).
// The analysis results will be used for funcx.
// We will clear the (global) unreferenced functions (all forward decls + def)
// according to their source ranges.
//
// used and refed:
// used -> refed, refed -x-> used.
// used -> codegen, refed -x-> codegen.
// We can determine which functions are used according to codegen,
// but we cannot determine which functions are referenced.
// Clearing a referenced function will cause compilation errors.
// Therefore, we need to analyze which functions are being referenced.
// Although clang::Decl provides `Used` and `Referenced` fields,
// these are not on-demand.
// Hence, we will start from the codegen function and perform on-demand
// function reference propagation analysis.
//
// used:
// * function call.
// * normal function pointer (*,&)
//   (including function pointer parameter, field).
//
// refed:
// * decltype.
// * using, e.g., using n::test;
// * function pointer refed by unused lambda,
//   e.g., auto lambda = [] { return foo; }
// * unused function pointer refed as template typename,
//   e.g., template<void (*foo)()> void call() {}
// * friend.
//
// * emit global definition: always refed.
//   Note: emit global definition can handle:
//   * normal global/member function definition.
//   * constructor, destructor, operator, virtual.
//   * explicit inst func and explicit inst class.
//     Note:
//     * UND explicit func: compile error.
//     * UND member func in explicit class: no codegen.
//   * special attrs:
//     * __attribute__((used)).
//     * __attribute__((constructor)).
//     * __attribute__((destructor)).
//   * var-init func, e.g., int x = test();
//
// * out-of-func refed: always refed.
//
// * in-func refed, bfs:
//     * func ref dependencies.
//     * implicit/explicit inst, explicit spec -> template.
//       Representing template dependencies with instantiation dependencies.
//
// Handle special source range:
//   * extern "C" -> always refed.
//   * macro -> always refed.
//
// When a function is refed, all its forward decls and definition are
// considered to be refed (getCanonicalDecl).
//
// function header and function body:
// For a refed function, we could have just cleared its function body,
// which can avoid dependency propagation within its function body.
// However, this can lead to the following issues:
// * clear the function bodies of inline, constexpr, auto,
//   external linkage functions can lead to compilation error.
// * clearing function bodies will generate numerous warnings,
//   such as undefined inline, undefined internal, unused parameters.
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_REFSYMBOLANALYSIS_H
#define ICLANG_REFSYMBOLANALYSIS_H

#include "iclang/FuncX/BasicAnalysis.h"

namespace iclang {
namespace funcx {

// Get all global func / func template (without inst) decls int the AST
// (without canonical!).
// Ignore impl!
class AllFuncDeclVisitor
    : public clang::RecursiveASTVisitor<AllFuncDeclVisitor> {
public:
  std::unordered_set<const clang::FunctionDecl *> allFuncDecls;
  std::unordered_set<const clang::FunctionTemplateDecl *> allFuncTempDecls;

  bool shouldVisitImplicitCode() const { return true; }

  bool TraverseDecl(clang::Decl *decl);
};

class AlwaysRefedAnalysis
    : public clang::RecursiveASTVisitor<AlwaysRefedAnalysis> {
public:
  const clang::SourceManager &sm;

  explicit AlwaysRefedAnalysis(const clang::SourceManager &_sm) : sm(_sm) {}

  std::unordered_set<const clang::FunctionDecl *> alwaysRefedFuncDecls;

  bool shouldVisitTemplateInstantiations() const { return true; }

  bool shouldVisitImplicitCode() const { return true; }

  bool isSpecialSourceRange(const clang::FunctionDecl *funcDecl) const;

  bool TraverseDecl(clang::Decl *decl);

  bool TraverseStmt(clang::Stmt *stmt, DataRecursionQueue *queue = nullptr);
};

class FuncRefedVisitor : public clang::RecursiveASTVisitor<FuncRefedVisitor> {
public:
  std::unordered_set<const clang::FunctionDecl *> refedFuncDecls;

  FuncRefedVisitor() {}

  bool shouldVisitTemplateInstantiations() const { return true; }

  bool shouldVisitImplicitCode() const { return true; }

  void init();

  bool TraverseDecl(clang::Decl *decl);

  bool TraverseStmt(clang::Stmt *stmt, DataRecursionQueue *queue = nullptr);
};

class RefSymbolAnalysis {
private:
  FuncRefedVisitor funcRefedVisitor;

public:
  std::unordered_set<const clang::FunctionDecl *>
  propagation(const std::unordered_set<const clang::FunctionDecl *>
                  &alwaysRefedFuncDecls);
};

} // namespace funcx
} // namespace iclang

#endif // ICLANG_REFSYMBOLANALYSIS_H

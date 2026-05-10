#include "iclang/FuncX/CheckAnalysis.h"

#include <iomanip>
#include <sstream>

#include "clang/AST/Expr.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/Preprocessor.h"

namespace iclang {
namespace funcx {

bool IncLineCheckAnalysis::TraverseDecl(clang::Decl *decl) {
  if (!decl) {
    return true;
  }
  auto *funcDecl = llvm::dyn_cast<clang::FunctionDecl>(decl);
  if (funcDecl == nullptr || !astGlobal.isValidFuncHeader(funcDecl) ||
      !astGlobal.isValidFuncBody(funcDecl)) {
    return RecursiveASTVisitor::TraverseDecl(decl);
  }
  funcDefNum += 1;
  // llvm::errs() << astGlobal.dumpDecl(funcDecl) << "\n";
  const int res = RecursiveASTVisitor::TraverseDecl(decl);
  return res;
}

bool LineMacroCheckAnalysis::TraverseDecl(clang::Decl *decl) {
  if (!decl) {
    return true;
  }
  auto *funcDecl = llvm::dyn_cast<clang::FunctionDecl>(decl);
  if (funcDecl == nullptr) {
    return RecursiveASTVisitor::TraverseDecl(decl);
  }
  if (!astGlobal.isMainFileDecl(funcDecl)) {
    return RecursiveASTVisitor::TraverseDecl(decl);
  }
  clang::FunctionDecl *oldFuncDecl = curFuncDecl;
  if (curFuncDecl == nullptr) {
    curFuncDecl = funcDecl;
    totalFuncNum += 1;
  }
  const int res = RecursiveASTVisitor::TraverseDecl(decl);
  curFuncDecl = oldFuncDecl;
  return res;
}

bool LineMacroCheckAnalysis::TraverseStmt(clang::Stmt *stmt,
                                         DataRecursionQueue *queue) {
  if (stmt == nullptr) {
    return true;
  }
  if (curFuncDecl == nullptr) {
    return RecursiveASTVisitor::TraverseStmt(stmt, queue);
  }
  if (const auto *callExpr = llvm::dyn_cast<clang::CallExpr>(stmt)) {
    const auto *funcDecl = callExpr->getDirectCallee();
    if (funcDecl != nullptr) {
      return true;
    }
  } else if (const auto *sourceLocExpr =
                 llvm::dyn_cast<clang::SourceLocExpr>(stmt)) {
    if (sourceLocExpr->getIdentKind() == clang::SourceLocIdentKind::Line) {
      funcsWithLineMacro.insert(curFuncDecl);
    }
  } else if (const auto *integerLiteralExpr = llvm::dyn_cast<clang::IntegerLiteral>(stmt)) {
    auto &sm = astGlobal.getSourceManager();
    auto &langOpts = astGlobal.getLangOpts();
    const clang::SourceLocation loc = integerLiteralExpr->getLocation();
    if (loc.isValid() && loc.isMacroID()) {
      const std::string macroName =
        clang::Lexer::getImmediateMacroName(loc, sm, langOpts).str();
      if (macroName == "__LINE__") {
        funcsWithLineMacro.insert(curFuncDecl);
        // llvm::errs() << curFuncDecl->getNameAsString() << "\n";
        // curFuncDecl->getSourceRange().dump(sm);
        // loc.dump(sm);
        // stmt->dump();
        // llvm::errs() << (void*)stmt << "\n";
      }
    }
  }
  return RecursiveASTVisitor::TraverseStmt(stmt, queue);
}

std::string DumpAnalysis::dumpPrefix(const int n) {
  std::ostringstream oss;
  for (int i = 0; i < n; i++) {
    oss << "|--";
  }
  return oss.str();
}

bool DumpAnalysis::TraverseDecl(clang::Decl *decl) {
  if (!decl) {
    return true;
  }

  llvm::errs() << dumpPrefix(depth) << astGlobal.dumpDecl(decl);

  if (const auto *funcTempDecl =
          llvm::dyn_cast<clang::FunctionTemplateDecl>(decl)) {
    llvm::errs() << "(templatedDecl: "
                 << astGlobal.dumpDecl(funcTempDecl->getTemplatedDecl()) << ")";
  }

  if (const auto *funcDecl = llvm::dyn_cast<clang::FunctionDecl>(decl)) {
    llvm::errs() << " (isExternC: " << funcDecl->isExternC() << ")";
    const auto *desTempDecl = funcDecl->getDescribedTemplate();
    llvm::errs() << " (desTempDecl: " << astGlobal.dumpDecl(desTempDecl) << ")";
    const auto *priTempDecl = funcDecl->getPrimaryTemplate();
    llvm::errs() << " (priTempDecl: " << astGlobal.dumpDecl(priTempDecl) << ")";
    llvm::errs() << " (isTemplated: " << funcDecl->isTemplated() << ")";
    llvm::errs() << " (isTemplateInstantiation: "
                 << funcDecl->isTemplateInstantiation() << ")";
    llvm::errs() << " (isFunctionTemplateSpecialization: "
                 << funcDecl->isFunctionTemplateSpecialization() << ")";
    llvm::errs() << " (Auto: " << ASTGlobal::hasAutoReturn(funcDecl) << ")";
  }

  llvm::errs() << "\n";

  if (const auto *usingShadowDecl =
          llvm::dyn_cast<clang::UsingShadowDecl>(decl)) {
    llvm::errs() << dumpPrefix(depth + 1) << "<UsingShadowDecl::getTargetDecl> "
                 << astGlobal.dumpDecl(usingShadowDecl->getTargetDecl())
                 << "\n";
    llvm::errs() << dumpPrefix(depth + 1)
                 << "<UsingShadowDecl::getUnderlyingDecl> "
                 << astGlobal.dumpDecl(usingShadowDecl->getUnderlyingDecl())
                 << "\n";
  }
  if (const auto *usingDecl = llvm::dyn_cast<clang::UsingDecl>(decl)) {
    llvm::errs() << dumpPrefix(depth + 1) << "<UsingDecl::getUnderlyingDecl> "
                 << astGlobal.dumpDecl(usingDecl->getUnderlyingDecl()) << "\n";
  }

  depth += 1;

  const bool res = RecursiveASTVisitor::TraverseDecl(decl);

  depth -= 1;

  return res;
}

bool DumpAnalysis::TraverseStmt(clang::Stmt *stmt, DataRecursionQueue *queue) {
  if (!stmt) {
    return true;
  }

  if (const auto *declRefExpr = llvm::dyn_cast<clang::DeclRefExpr>(stmt)) {
    llvm::errs() << dumpPrefix(depth) << "<DeclRefExpr> "
                 << astGlobal.dumpDecl(declRefExpr->getDecl()) << "\n";
  } else if (const auto *memberExpr = llvm::dyn_cast<clang::MemberExpr>(stmt)) {
    llvm::errs() << dumpPrefix(depth) << "<MemberExpr> "
                 << astGlobal.dumpDecl(memberExpr->getMemberDecl()) << "\n";
  } else if (const auto *ctor = llvm::dyn_cast<clang::CXXConstructExpr>(stmt)) {
    llvm::errs() << dumpPrefix(depth) << "<CXXConstructExpr> "
                 << astGlobal.dumpDecl(ctor->getConstructor()) << "\n";
  } else if (const auto *usLookupExpr =
                 llvm::dyn_cast<clang::UnresolvedLookupExpr>(stmt)) {
    for (const auto *decl : usLookupExpr->decls()) {
      llvm::errs() << dumpPrefix(depth) << "<UnresolvedLookupExpr> "
                   << astGlobal.dumpDecl(decl) << "\n";
    }
  } else if (const auto *usMemberExpr =
                 llvm::dyn_cast<clang::UnresolvedMemberExpr>(stmt)) {
    for (const auto *decl : usMemberExpr->decls()) {
      llvm::errs() << dumpPrefix(depth) << "<UnresolvedMemberExpr> "
                   << astGlobal.dumpDecl(decl) << "\n";
    }
  } else if (const auto *newExpr = llvm::dyn_cast<clang::CXXNewExpr>(stmt)) {
    llvm::errs() << dumpPrefix(depth) << "<CXXNewExpr::getOperatorNew> "
                 << astGlobal.dumpDecl(newExpr->getOperatorNew()) << "\n";
    llvm::errs() << dumpPrefix(depth) << "<CXXNewExpr::getOperatorDelete> "
                 << astGlobal.dumpDecl(newExpr->getOperatorDelete()) << "\n";
  } else if (const auto *deleteExpr =
                 llvm::dyn_cast<clang::CXXDeleteExpr>(stmt)) {
    llvm::errs() << dumpPrefix(depth) << "<CXXDeleteExpr::getOperatorDelete> "
                 << astGlobal.dumpDecl(deleteExpr->getOperatorDelete()) << "\n";
  } else if (const auto *icie =
                 llvm::dyn_cast<clang::CXXInheritedCtorInitExpr>(stmt)) {
    llvm::errs() << dumpPrefix(depth) << "<CXXInheritedCtorInitExpr> "
                 << astGlobal.dumpDecl(icie->getConstructor()) << "\n";
  }

  return RecursiveASTVisitor::TraverseStmt(stmt, queue);
}

} // namespace funcx
} // namespace iclang
#include "iclang/FuncX/RefSymbolAnalysis.h"

#include <queue>

namespace iclang {
namespace funcx {

bool AllFuncDeclVisitor::TraverseDecl(clang::Decl *decl) {
  if (!decl) {
    return true;
  }

  if (const auto *funcDecl = llvm::dyn_cast<clang::FunctionDecl>(decl)) {
    // Do not use getCanonicalDecl here.
    if (!funcDecl->isImplicit()) {
      allFuncDecls.insert(funcDecl);
    }
  }
  if (const auto *funcTempDecl =
          llvm::dyn_cast<clang::FunctionTemplateDecl>(decl)) {
    if (!funcTempDecl->isImplicit()) {
      allFuncTempDecls.insert(funcTempDecl);
    }
  }

  // Skip local functions.
  if (llvm::dyn_cast<clang::FunctionDecl>(decl) != nullptr ||
      llvm::dyn_cast<clang::VarDecl>(decl) != nullptr ||
      llvm::dyn_cast<clang::FieldDecl>(decl) != nullptr) {
    return true;
  }

  const bool res = RecursiveASTVisitor::TraverseDecl(decl);

  return res;
}

bool AlwaysRefedAnalysis::isSpecialSourceRange(
    const clang::FunctionDecl *funcDecl) const {
  // extern "C".
  if (funcDecl->isExternC()) {
    return true;
  }

  // macro.
  const auto loc = funcDecl->getLocation();
  if (sm.isMacroBodyExpansion(loc) || sm.isMacroArgExpansion(loc)) {
    return true;
  }

  return false;
}

bool AlwaysRefedAnalysis::TraverseDecl(clang::Decl *decl) {
  if (!decl) {
    return true;
  }

  if (const auto *usingShadowDecl =
          llvm::dyn_cast<clang::UsingShadowDecl>(decl)) {
    const auto *targetDecl = usingShadowDecl->getTargetDecl();
    if (const auto *funcDecl =
            llvm::dyn_cast<clang::FunctionDecl>(targetDecl)) {
      alwaysRefedFuncDecls.insert(funcDecl->getCanonicalDecl());
    }
  }

  if (const auto *funcDecl = llvm::dyn_cast<clang::FunctionDecl>(decl)) {
    if (isSpecialSourceRange(funcDecl)) {
      alwaysRefedFuncDecls.insert(funcDecl->getCanonicalDecl());
    }
    return true;
  }

  const bool res = RecursiveASTVisitor::TraverseDecl(decl);

  return res;
}

static std::unordered_set<const clang::FunctionDecl *>
extractFuncDependencies(clang::Stmt *stmt) {
  std::unordered_set<const clang::FunctionDecl *> res;

  std::vector<const clang::Decl *> targetDecls;
  if (const auto *declRefExpr = llvm::dyn_cast<clang::DeclRefExpr>(stmt)) {
    targetDecls.push_back(declRefExpr->getDecl());
  } else if (const auto *memberExpr = llvm::dyn_cast<clang::MemberExpr>(stmt)) {
    targetDecls.push_back(memberExpr->getMemberDecl());
  } else if (const auto *ctor = llvm::dyn_cast<clang::CXXConstructExpr>(stmt)) {
    targetDecls.push_back(ctor->getConstructor());
  } else if (const auto *usLookupExpr =
                 llvm::dyn_cast<clang::UnresolvedLookupExpr>(stmt)) {
    for (const auto *decl : usLookupExpr->decls()) {
      targetDecls.push_back(decl);
    }
  } else if (const auto *usMemberExpr =
                 llvm::dyn_cast<clang::UnresolvedMemberExpr>(stmt)) {
    for (const auto *decl : usMemberExpr->decls()) {
      targetDecls.push_back(decl);
    }
  } else if (const auto *newExpr = llvm::dyn_cast<clang::CXXNewExpr>(stmt)) {
    targetDecls.push_back(newExpr->getOperatorNew());
    targetDecls.push_back(newExpr->getOperatorDelete());
  } else if (const auto *deleteExpr =
                 llvm::dyn_cast<clang::CXXDeleteExpr>(stmt)) {
    targetDecls.push_back(deleteExpr->getOperatorDelete());
  } else if (const auto *icie =
                 llvm::dyn_cast<clang::CXXInheritedCtorInitExpr>(stmt)) {
    targetDecls.push_back(icie->getConstructor());
  }

  for (const clang::Decl *targetDecl : targetDecls) {
    if (targetDecl == nullptr) {
      continue;
    }
    const auto *targetFuncDecl =
        llvm::dyn_cast<clang::FunctionDecl>(targetDecl);
    if (targetFuncDecl == nullptr) {
      continue;
    }
    res.insert(targetFuncDecl->getCanonicalDecl());
  }

  return res;
}

bool AlwaysRefedAnalysis::TraverseStmt(clang::Stmt *stmt,
                                       DataRecursionQueue *queue) {
  if (!stmt) {
    return true;
  }

  auto dependencies = extractFuncDependencies(stmt);
  alwaysRefedFuncDecls.insert(dependencies.begin(), dependencies.end());

  return RecursiveASTVisitor::TraverseStmt(stmt, queue);
}

void FuncRefedVisitor::init() { refedFuncDecls.clear(); }

bool FuncRefedVisitor::TraverseDecl(clang::Decl *decl) {
  if (!decl) {
    return true;
  }
  if (const auto *usingShadowDecl =
          llvm::dyn_cast<clang::UsingShadowDecl>(decl)) {
    const auto *targetDecl = usingShadowDecl->getTargetDecl();
    if (const auto *funcDecl =
            llvm::dyn_cast<clang::FunctionDecl>(targetDecl)) {
      refedFuncDecls.insert(funcDecl->getCanonicalDecl());
    }
  }
  return RecursiveASTVisitor::TraverseDecl(decl);
}

bool FuncRefedVisitor::TraverseStmt(clang::Stmt *stmt, DataRecursionQueue *queue) {
  if (!stmt) {
    return true;
  }

  auto dependencies = extractFuncDependencies(stmt);
  refedFuncDecls.insert(dependencies.begin(), dependencies.end());

  return RecursiveASTVisitor::TraverseStmt(stmt, queue);
}

std::unordered_set<const clang::FunctionDecl *> RefSymbolAnalysis::propagation(
    const std::unordered_set<const clang::FunctionDecl *>
        &alwaysRefedFuncDecls) {
  std::unordered_set<const clang::FunctionDecl *> refedFuncDecls;

  std::queue<const clang::FunctionDecl *> que;
  for (const auto *decl : alwaysRefedFuncDecls) {
    que.push(decl);
  }

  while (!que.empty()) {
    auto *funcDecl = que.front();
    que.pop();

    if (!refedFuncDecls.emplace(funcDecl->getCanonicalDecl()).second) {
      continue;
    }

    if (funcDecl->isTemplated()) {
      continue;
    }

    if (funcDecl->getDefinition() != nullptr) {
      funcDecl = funcDecl->getDefinition();
    }
    funcRefedVisitor.init();
    funcRefedVisitor.TraverseDecl(const_cast<clang::FunctionDecl *>(funcDecl));
    for (const auto *refedFuncDecl : funcRefedVisitor.refedFuncDecls) {
      que.push(refedFuncDecl->getCanonicalDecl());
    }
  }

  return refedFuncDecls;
}

} // namespace funcx
} // namespace iclang
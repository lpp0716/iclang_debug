#include "iclang/FuncX/ReusableInstAnalysis.h"

namespace iclang {
namespace funcx {

bool BasicSafeChecker::isMainFileDecl(const clang::SourceManager &sourceManager,
                                      const clang::Decl *decl) {
  if (decl->isImplicit()) {
    return false;
  }
  const auto loc = decl->getLocation();
  return loc.isValid() && sourceManager.isInMainFile(loc);
}

bool TopIncludeRegionVisitor::TraverseDecl(clang::Decl *decl) {
  if (!decl) {
    return true;
  }
  if (auto *transDecl = llvm::dyn_cast<clang::TranslationUnitDecl>(decl)) {
    return RecursiveASTVisitor::TraverseDecl(transDecl);
  }

  if (mainFirstDeclLine == 0 &&
      BasicSafeChecker::isMainFileDecl(sourceManager, decl)) {
    const clang::FullSourceLoc fLoc(decl->getLocation(), sourceManager);
    if (fLoc.isValid() && fLoc.getExpansionLineNumber() > 0) {
      mainFirstDeclLine = fLoc.getExpansionLineNumber();
    }
  }

  if (mainFirstDeclLine != 0) {
    return false;
  }

  return true;
}

bool ReusableInstAnalysis::classExtraction(
    const clang::QualType &type,
    std::unordered_set<const clang::CXXRecordDecl *> &decls) {
  if (type->isBuiltinType()) {
    return true;
  }
  if (type->isPointerType() || type->isReferenceType()) {
    const auto pteType = type->getPointeeType();
    if (!classExtraction(pteType, decls)) {
      return false;
    }
    return true;
  }
  if (type->isRecordType()) {
    const clang::RecordType *recordType = type->getAs<clang::RecordType>();
    if (recordType == nullptr) {
      return false;
    }
    const clang::RecordDecl *recordDecl = recordType->getDecl();
    if (recordDecl == nullptr) {
      return false;
    }
    const auto *cxxRecordDecl =
        llvm::dyn_cast<clang::CXXRecordDecl>(recordDecl);
    if (cxxRecordDecl == nullptr) {
      return false;
    }
    decls.insert(cxxRecordDecl);
    return true;
  }
  return false;
}

bool ReusableInstAnalysis::necessaryCheck(const clang::FunctionDecl *funcDecl) {
  if (funcDecl->getLinkageAndVisibility().getLinkage() !=
          clang::Linkage::External ||
      funcDecl->getType()->getLinkage() != clang::Linkage::External) {
    return false;
  }
  if (llvm::dyn_cast<clang::CXXConstructorDecl>(funcDecl) != nullptr ||
      llvm::dyn_cast<clang::CXXDestructorDecl>(funcDecl) != nullptr) {
    return false;
  }
  if (funcDecl->getOverloadedOperator() !=
          clang::OverloadedOperatorKind::OO_None ||
      llvm::dyn_cast<clang::CXXConversionDecl>(funcDecl) != nullptr) {
    return false;
  }
  if (const auto *cxxMethodDecl =
          llvm::dyn_cast<clang::CXXMethodDecl>(funcDecl)) {
    if (cxxMethodDecl->isVirtual()) {
      return false;
    }
  }
  return true;
}

bool ReusableInstAnalysis::mangleNameExist(
    const clang::FunctionDecl *funcDecl) const {
  const auto &prevAPIs = incMetaData->prevAPIs;
  const std::string mangledName = astGlobal.getMangledName(funcDecl);
  if (mangledName.empty() || prevAPIs.find(mangledName) == prevAPIs.end()) {
    return false;
  }
  return true;
}

bool ReusableInstAnalysis::isGlobalFuncInst(
    const clang::FunctionDecl *funcDecl) {
  bool isInst = false;
  const clang::DeclContext *dc = funcDecl->getDeclContext();
  while (dc && !llvm::isa<clang::TranslationUnitDecl>(dc)) {
    if (dc->isFunctionOrMethod()) {
      return false;
    }
    if (llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(dc) != nullptr) {
      isInst = true;
      break;
    }
    dc = dc->getParent();
  }
  const auto *funcTemp = funcDecl->getPrimaryTemplate();
  if (funcTemp != nullptr) {
    isInst = true;
  }
  return isInst;
}

bool ReusableInstAnalysis::funcTemplateSafeCheck(
    const clang::FunctionDecl *funcDecl) const {
  const auto *funcTemp = funcDecl->getPrimaryTemplate();
  if (funcTemp != nullptr && BasicSafeChecker::isMainFileDecl(
                                 sourceManager, funcTemp->getCanonicalDecl())) {
    return false;
  }
  return true;
}

bool ReusableInstAnalysis::classTemplateSafeCheck(
    const clang::CXXRecordDecl *classDecl) const {
  const auto *classSpec =
      llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(classDecl);
  if (classSpec == nullptr) {
    if (BasicSafeChecker::isMainFileDecl(sourceManager, classDecl)) {
      return false;
    }
    // May access to class template from context.
    const auto *desTempDecl = classDecl->getDescribedClassTemplate();
    if (desTempDecl != nullptr &&
        BasicSafeChecker::isMainFileDecl(sourceManager,
                                         desTempDecl->getCanonicalDecl())) {
      return false;
    }
  } else {
    if (BasicSafeChecker::isMainFileDecl(
            sourceManager,
            classSpec->getSpecializedTemplate()->getCanonicalDecl())) {
      return false;
    }
    // Handle explicit specializations.
    if (BasicSafeChecker::isMainFileDecl(sourceManager, classDecl)) {
      return false;
    }
  }
  return true;
}

bool ReusableInstAnalysis::instTypeSafeCheck(
    const clang::TemplateArgumentList &tempArgList) {
  // Note that getTemplateSpecializationArgs() / getTemplateArgs()
  // can de-alise (recursively) automatically,
  // and it can parse nested instantiation types.
  for (size_t i = 0; i < tempArgList.size(); i++) {
    const auto &arg = tempArgList.get(i);
    if (arg.getKind() != clang::TemplateArgument::Type) {
      if (arg.getKind() != clang::TemplateArgument::Integral) {
        return false;
      }
      continue;
    }

    const auto argType = arg.getAsType();
    std::unordered_set<const clang::CXXRecordDecl *> extractedClasses;
    if (!classExtraction(argType, extractedClasses)) {
      return false;
    }

    for (const auto *extractedClass : extractedClasses) {
      if (!checkClass(extractedClass)) {
        return false;
      }
    }
  }
  return true;
}

bool ReusableInstAnalysis::funcContextSafeCheck(
    const clang::FunctionDecl *funcDecl) {
  const clang::DeclContext *dc = funcDecl->getDeclContext();
  while (dc && !llvm::isa<clang::TranslationUnitDecl>(dc)) {
    if (const auto *enclosingClass = llvm::dyn_cast<clang::CXXRecordDecl>(dc)) {
      if (!checkClass(enclosingClass)) {
        return false;
      }
      // Note: Only parse one layer at a time.
      break;
    }
    dc = dc->getParent();
  }
  return true;
}

bool ReusableInstAnalysis::classContextSafeCheck(
    const clang::CXXRecordDecl *classDecl) {
  const clang::DeclContext *dc = classDecl->getDeclContext();
  while (dc && !llvm::isa<clang::TranslationUnitDecl>(dc)) {
    // AST global.
    if (dc->isFunctionOrMethod()) {
      return false;
    }
    if (const auto *enclosingClass = llvm::dyn_cast<clang::CXXRecordDecl>(dc)) {
      if (!checkClass(enclosingClass)) {
        return false;
      }
      // Note: Only parse one layer at a time.
      break;
    }
    dc = dc->getParent();
  }
  return true;
}

bool ReusableInstAnalysis::checkClass(const clang::CXXRecordDecl *classDecl) {
  const auto *cClassDecl = classDecl->getCanonicalDecl();

  const auto checkIt = checkRecord.find(cClassDecl);
  if (checkIt != checkRecord.end()) {
    return checkIt->second;
  }

  // (1) Self safe check.
  if (!classTemplateSafeCheck(cClassDecl)) {
    return cache(cClassDecl, false);
  }

  // (2) Self inst type safe flag check.
  const auto *classSpec =
      llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(classDecl);
  if (classSpec != nullptr &&
      !instTypeSafeCheck(classSpec->getTemplateArgs())) {
    return cache(cClassDecl, false);
  }

  // (3) Enclosing classes recursive.
  if (!classContextSafeCheck(classDecl)) {
    return cache(cClassDecl, false);
  }

  return cache(cClassDecl, true);
}

bool ReusableInstAnalysis::canBeFuncXed(const clang::FunctionDecl *funcDecl) {
  // (1) Necessary checking.
  if (!necessaryCheck(funcDecl)) {
    return false;
  }
  // (2) MangledName existing check.
  if (!mangleNameExist(funcDecl)) {
    return false;
  }
  // (3) Is AST global function inst.
  if (!isGlobalFuncInst(funcDecl)) {
    return false;
  }
  // (4) Self safe check.
  if (!funcTemplateSafeCheck(funcDecl)) {
    return false;
  }
  // For member function instantiations, see (6).
  // (5) Self inst type safe check.
  const auto *tempArgList = funcDecl->getTemplateSpecializationArgs();
  if (tempArgList != nullptr && !instTypeSafeCheck(*tempArgList)) {
    return false;
  }
  // (6) Enclosing classes (and their inst types) safe check.
  if (!funcContextSafeCheck(funcDecl)) {
    return false;
  }

  return true;
}

void ReusableInstAnalysis::run(std::deque<VSPair> &pendingInstQue) {
  std::vector<VSPair> preservedInsts;
  while (!pendingInstQue.empty()) {
    const auto inst = pendingInstQue.front();
    pendingInstQue.pop_front();

    // Remove reusable functions from pendingInstQueue,
    // add them to funcXSet.
    if (const auto *funcDecl =
            llvm::dyn_cast<clang::FunctionDecl>(inst.first)) {
      if (canBeFuncXed(funcDecl)) {
        const std::string mangledName = astGlobal.getMangledName(funcDecl);
        incMetaData->funcXSet.insert(mangledName);
        astGlobal.addDisableWarningDecl(funcDecl);
        continue;
      }
    }

    preservedInsts.push_back(inst);
  }
  for (const auto &inst : preservedInsts) {
    pendingInstQue.push_back(inst);
  }
}

} // namespace funcx
} // namespace iclang
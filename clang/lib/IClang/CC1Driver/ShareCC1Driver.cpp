#include "iclang/CC1Driver/ShareCC1Driver.h"

#include "iclang/FuncX/RefSymbolAnalysis.h"
#include "illvm/Support/Diagnostics.h"
#include "illvm/Support/Interval.h"

namespace iclang {

void ShareMasterCC1Driver::run() {
  ILLVM_FCHECK(false, "We haven't implemented ShareMasterCC1Driver yet");
}

void ShareClientCC1Driver::run() {
  ILLVM_FCHECK(false, "We haven't implemented ShareClientCC1Driver yet");
}

void ShareCheckCC1Driver::run() {
  auto &global = Global::getInstance();

  assert(global.getIClangMode() == IClangMode::ShareCheckMode);

  auto &astGlobal = ASTGlobal::getInstance();
  auto &context = astGlobal.getContext();

  auto metaData = global.getMetaData<ShareCheckMetaData>();
  const auto astMetaData = astGlobal.getASTMetaData<ShareCheckASTMetaData>();

  if (!metaData->enableRefedSymbolAnalysisFlag) {
    return;
  }

  // Init
  funcx::AlwaysRefedAnalysis alwaysRefedAnalysis(context.getSourceManager());
  alwaysRefedAnalysis.TraverseDecl(context.getTranslationUnitDecl());

  auto alwaysRefedFuncDecls = alwaysRefedAnalysis.alwaysRefedFuncDecls;
  for (const auto &elem : astMetaData->emitGlobalFuncDefs) {
    alwaysRefedFuncDecls.insert(elem);
  }

  // Propagation.
  funcx::RefSymbolAnalysis refSymbolAnalysis;
  auto refedFuncDecls = refSymbolAnalysis.propagation(alwaysRefedFuncDecls);
  // Handle templates.
  std::unordered_set<const clang::FunctionTemplateDecl *> refedTempDecls;
  for (const clang::FunctionDecl *elem : refedFuncDecls) {
    const auto *priTempDecl = elem->getPrimaryTemplate();
    if (priTempDecl != nullptr) {
      refedTempDecls.insert(priTempDecl->getCanonicalDecl());
    }
    // Note: desTempDecl only work for templated function.
  }
  for (const auto *elem : refedTempDecls) {
    const auto *templatedFuncDecl = elem->getTemplatedDecl();
    refedFuncDecls.insert(templatedFuncDecl);
  }

  // All - refed = unRefed.
  // refedFuncDecls: canonical decls.
  // allFuncDecls: all decls.
  funcx::AllFuncDeclVisitor allFuncDeclVisitor;
  allFuncDeclVisitor.TraverseDecl(context.getTranslationUnitDecl());

  // unRefed source ranges.
  illvm::IntervalManager intervalManager;

  for (const auto *elem : allFuncDeclVisitor.allFuncDecls) {
    if (refedFuncDecls.find(elem->getCanonicalDecl()) == refedFuncDecls.end()) {
      intervalManager.addInterval(
          astGlobal.getDeclSourceInterval(elem).toInterval());
    }
  }
  for (const auto *elem : allFuncDeclVisitor.allFuncTempDecls) {
    if (refedTempDecls.find(elem->getCanonicalDecl()) == refedTempDecls.end()) {
      intervalManager.addInterval(
          astGlobal.getDeclSourceInterval(elem).toInterval());
    }
  }

  metaData->unRefedDeclIntervals =
      intervalManager.merge(intervalManager.getIntervals());
}

} // namespace iclang
#include "iclang/CC1Driver/CheckCC1Driver.h"

#include "iclang/CC1Driver/CC1DriverBase.h"
#include "iclang/FuncX/CheckAnalysis.h"

namespace iclang {

void IncLineCheckCC1Driver::run() {
  auto &global = Global::getInstance();

  assert(global.getIClangMode() == IClangMode::IncLineCheckMode);

  auto &astGlobal = ASTGlobal::getInstance();
  auto &context = astGlobal.getContext();

  auto metaData = global.getMetaData<IncLineCheckMetaData>();
  const auto astMetaData = astGlobal.getASTMetaData<IncLineCheckASTMetaData>();

  funcx::IncLineCheckAnalysis incLineCheckAnalysis(astGlobal);
  incLineCheckAnalysis.TraverseDecl(context.getTranslationUnitDecl());

  metaData->baseFuncDefNum = incLineCheckAnalysis.getFuncDefNum();
}

void LineMacroCheckCC1Driver::run() {
  auto &global = Global::getInstance();

  assert(global.getIClangMode() == IClangMode::LineMacroCheckMode);

  auto &astGlobal = ASTGlobal::getInstance();
  auto &context = astGlobal.getContext();

  auto metaData = global.getMetaData<LineMacroCheckMetaData>();
  const auto astMetaData = astGlobal.getASTMetaData<LineMacroCheckASTMetaData>();

  funcx::LineMacroCheckAnalysis lineMacroCheckAnalysis(astGlobal);
  lineMacroCheckAnalysis.TraverseDecl(context.getTranslationUnitDecl());

  metaData->totalFuncNum = lineMacroCheckAnalysis.getTotalFuncNum();
  metaData->funcWithLineMacroNum =
      lineMacroCheckAnalysis.getFuncWithLineMacroNum();
}

void DumpCC1Driver::run() {
  auto &global = Global::getInstance();

  assert(global.getIClangMode() == IClangMode::DumpMode);

  auto &astGlobal = ASTGlobal::getInstance();
  auto &context = astGlobal.getContext();

  // auto metaData = global.getMetaData<DumpMetaData>();
  // const auto astMetaData = astGlobal.getASTMetaData<DumpASTMetaData>();

  funcx::DumpAnalysis dumpAnalysis(astGlobal);
  dumpAnalysis.TraverseDecl(context.getTranslationUnitDecl());
}

void ProfileCC1Driver::run() {
  // Empty.
}

}
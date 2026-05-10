#include "iclang/ASTSupport/ASTMetaData.h"

#include <iomanip>
#include <sstream>

#include "clang/AST/ASTConsumer.h"
#include "clang/Lex/Preprocessor.h"

namespace iclang {

void IncLineCheckASTMetaData::injectIClangLineWMacro(clang::Sema &sema) {
  auto &pp = sema.getPreprocessor();
  auto &sm = pp.getSourceManager();

  // line 1, col 1.
  const clang::FileID mainFID = sm.getMainFileID();
  const clang::SourceLocation defLoc = sm.getLocForStartOfFile(mainFID);

  const std::string bodyText = "__iclang_line(__ICLANGLINE__)";
  auto buf =
      llvm::MemoryBuffer::getMemBufferCopy(bodyText, "<ICLANGLINEWMACRO>");
  const clang::FileID fId =
      sm.createFileID(std::move(buf), clang::SrcMgr::C_User);
  const clang::SourceLocation base = sm.getLocForStartOfFile(fId);

  // Create IClang macro.
  clang::IdentifierInfo &iiMacro =
      pp.getIdentifierTable().get("__ICLW__");

  clang::MacroInfo *mi = pp.AllocateMacroInfo(defLoc);

  auto body =
      mi->allocateTokens(/*NumTokens=*/4, pp.getPreprocessorAllocator());

  // __iclang_line
  {
    clang::Token t;
    t.startToken();
    t.setKind(clang::tok::identifier);
    t.setIdentifierInfo(&pp.getIdentifierTable().get("__iclang_line"));
    t.setLocation(base);
    t.setLength(13);
    body[0] = t;
  }

  // '('
  {
    clang::Token t;
    t.startToken();
    t.setKind(clang::tok::l_paren);
    t.setLocation(base.getLocWithOffset(13));
    t.setLength(1);
    body[1] = t;
  }

  // __ICLANGLINE__
  {
    clang::Token t;
    t.startToken();
    t.setKind(clang::tok::identifier);
    t.setIdentifierInfo(&pp.getIdentifierTable().get("__ICLANGLINE__"));
    t.setLocation(base.getLocWithOffset(14));
    t.setLength(14);
    body[2] = t;
  }

  // ')'
  {
    clang::Token t;
    t.startToken();
    t.setKind(clang::tok::r_paren);
    t.setLocation(base.getLocWithOffset(28));
    t.setLength(1);
    body[3] = t;
  }

  pp.appendDefMacroDirective(&iiMacro, mi);
}

void IncLineCheckASTMetaData::injectIClangLineFunc(clang::Sema &sema) {
  using namespace clang;

  ASTContext &ctx = sema.getASTContext();
  TranslationUnitDecl *tu = ctx.getTranslationUnitDecl();

  // Func name: _iclang_line_table_interceptor.
  const IdentifierInfo &ii = ctx.Idents.get("__iclang_line");
  const DeclarationName name(&ii);

  // Return type: unsigned int.
  const QualType retTy = ctx.UnsignedIntTy;
  // Param type: unsigned int.
  const QualType paramTy = ctx.UnsignedIntTy;

  // Function Type: unsigned int(unsigned int).
  const FunctionProtoType::ExtProtoInfo epi;
  const QualType fnTy = ctx.getFunctionType(retTy, {paramTy}, epi);

  // FunctionDecl: static int _iclang_line_table_interceptor(int);
  auto *fd = FunctionDecl::Create(ctx, tu, SourceLocation(), SourceLocation(),
                                  name, fnTy, nullptr, SC_Static);
  fd->setImplicit(true);

  // Parameter: (unsigned int line).
  const IdentifierInfo &pii = ctx.Idents.get("line");
  auto *param = ParmVarDecl::Create(ctx, fd, SourceLocation(), SourceLocation(),
                                    &pii, paramTy, nullptr, SC_None, nullptr);
  fd->setParams({param});

  // DeclRefExpr in ReturnStmt: line.
  auto *dre =
      DeclRefExpr::Create(ctx, NestedNameSpecifierLoc(), SourceLocation(),
                          param, false, SourceLocation(), paramTy, VK_LValue);

  // Function body: { return line; }.
  auto *ret = ReturnStmt::Create(ctx, SourceLocation(), dre, nullptr);
  auto *body =
      CompoundStmt::Create(ctx, {ret}, {}, SourceLocation(), SourceLocation());
  fd->setBody(body);

  // __attribute__((optnone, noinline, used)).
  fd->addAttr(NoInlineAttr::CreateImplicit(ctx));
  fd->addAttr(OptimizeNoneAttr::CreateImplicit(ctx));
  fd->addAttr(UsedAttr::CreateImplicit(ctx));

  // Add to AST.
  tu->addDecl(fd);

  // Codegen.
  const DeclGroupRef dgr(fd);
  sema.getASTConsumer().HandleTopLevelDecl(dgr);
}

void ShareCheckASTMetaData::addEmitGlobalFuncDef(
    const clang::FunctionDecl *funcDecl) {
  emitGlobalFuncDefs.insert(funcDecl->getCanonicalDecl());
}

} // namespace iclang

#include "iclang/ASTSupport/ASTGlobal.h"

#include <iomanip>
#include <sstream>

#include "clang/Lex/Lexer.h"
#include "llvm/Demangle/Demangle.h"

namespace iclang {

void ASTGlobal::init(const Global &global, clang::Sema *_sema) {
  iClangMode = global.getIClangMode();
//  llvm::errs() << "[IClang Debug] Current Mode: " << (int)iClangMode << "\n";
  std::unique_ptr<ASTMetaData> ptr;
  if (iClangMode == IClangMode::IncMode) {
    astMetaData = illvm::make_owner<IncASTMetaData>().moveTo<ASTMetaData>();
  } else if (iClangMode == IClangMode::IncCheckMode) {
    astMetaData = illvm::make_owner<IncCheckASTMetaData>().moveTo<ASTMetaData>();
  } else if (iClangMode == IClangMode::IncLineCheckMode) {
    astMetaData =
        illvm::make_owner<IncLineCheckASTMetaData>().moveTo<ASTMetaData>();
  } else if (iClangMode == IClangMode::ShareMasterMode) {
    astMetaData =
        illvm::make_owner<ShareMasterASTMetaData>().moveTo<ASTMetaData>();
  } else if (iClangMode == IClangMode::ShareClientMode) {
    astMetaData =
        illvm::make_owner<ShareClientASTMetaData>().moveTo<ASTMetaData>();
  } else if (iClangMode == IClangMode::ShareCheckMode) {
    astMetaData =
        illvm::make_owner<ShareCheckASTMetaData>().moveTo<ASTMetaData>();
  } else if (iClangMode == IClangMode::LineMacroCheckMode) {
    astMetaData =
        illvm::make_owner<LineMacroCheckASTMetaData>().moveTo<ASTMetaData>();
  } else if (iClangMode == IClangMode::DumpMode) {
    astMetaData = illvm::make_owner<DumpASTMetaData>().moveTo<ASTMetaData>();
  } else if (iClangMode == IClangMode::ProfileMode) {
    astMetaData = illvm::make_owner<ProfileASTMetaData>().moveTo<ASTMetaData>();
  } else if (iClangMode == IClangMode::RedundantSkipMode) {
    auto skipMeta = illvm::make_owner<RedundantSkipASTMetaData>();
    auto tInitStart = std::chrono::high_resolution_clock::now();

    std::string outPath = global.outputPath;
    if (!outPath.empty()) {
      // 1. 获取绝对路径确保 MD5 唯一
      llvm::SmallString<128> AbsPath(outPath);
      llvm::sys::fs::make_absolute(AbsPath);
      // 2. 计算 MD5
      llvm::MD5 hash;
      hash.update(AbsPath);
      llvm::MD5::MD5Result result;
      hash.final(result);
      std::string md5Name = result.digest().str().str();
//      llvm::errs() << "[IClang] OutputPath: " << AbsPath << '\n';
//      llvm::errs() << "[IClang] md5Name: " << md5Name << '\n';
      // 3. 从目录加载纯文本清单 (/tmp/iclang_redundant/[MD5].txt)
      std::string dirPath;
      if (const char *envDir = std::getenv("ICLANG_SHARD_DIR")) {
        dirPath = envDir;
        if (!dirPath.empty() && dirPath.back() != '/') {
          dirPath += '/'; // 确保路径以 '/' 结尾
        }
      } else {
        // 兜底路径 (万一没配环境变量也能跑)
        llvm::errs() << "ICLANG error: do not set ICLANG_SHARD_DIR \n";
        dirPath = "/root/llvm-project/iclang_shardssss/";
      }

      std::string shardPath = dirPath + md5Name + ".list";
      std::string basesPath = dirPath + md5Name + "_bases.list";

      auto loadListFile =[](const std::string &path, auto insertAction) {
        auto bufferOrErr = llvm::MemoryBuffer::getFile(path);
        // 如果文件不存在或无权限读取，静默返回，避免编译器崩溃
        if (!bufferOrErr) return;

        llvm::StringRef content = bufferOrErr.get()->getBuffer();

        // 放弃 SmallVector，采用极速的游标切割，全程 0 次堆内存分配！
        while (!content.empty()) {
          llvm::StringRef line;
          std::tie(line, content) = content.split('\n');
          line = line.trim();
          if (!line.empty()) {
            insertAction(line);
          }
        }
      };

      // 加载全量混淆符号表
      loadListFile(shardPath, [&](llvm::StringRef line) {
        skipMeta->redundantSymbols.insert(line.str());
      });

      // 加载短名字基名表
      loadListFile(basesPath, [&](llvm::StringRef line) {
        skipMeta->redundantBaseNames.insert(line);
      });
    }
    auto tInitEnd = std::chrono::high_resolution_clock::now();
    skipMeta->initTimeMs = std::chrono::duration<double, std::milli>(tInitEnd - tInitStart).count();
    astMetaData = skipMeta.moveTo<ASTMetaData>();
  } else {
    astMetaData = illvm::make_owner<ASTMetaData>();
  }
  sema = _sema;
  astNameGenerator =
      std::make_unique<clang::ASTNameGenerator>(sema->getASTContext());
}

std::string ASTGlobal::getMangledName(const clang::NamedDecl *decl) const {
  if (decl && decl->getDeclName()) {
    if (llvm::isa<clang::RequiresExprBodyDecl>(decl->getDeclContext())) {
      return "";
    }
    auto *varDecl = llvm::dyn_cast<clang::VarDecl>(decl);
    if (varDecl && varDecl->hasLocalStorage()) {
      return "";
    }
    return astNameGenerator->getName(decl);
  }
  return "";
}

illvm::SourceInterval
ASTGlobal::getDeclSourceInterval(const clang::Decl *decl) const {
  illvm::SourceInterval res{};

  res.isValid = false;

  const auto sr = decl->getSourceRange();
  const auto &sm =  getSourceManager();

  // start
  const clang::FullSourceLoc startFullSourceLoc(sr.getBegin(), sm);
  if (startFullSourceLoc.isInvalid()) {
    return res;
  }
  res.startLine = startFullSourceLoc.getExpansionLineNumber();
  res.startColumn = startFullSourceLoc.getExpansionColumnNumber();

  // end
  const clang::SourceLocation endSourceLoc = clang::Lexer::getLocForEndOfToken(
      sr.getEnd(), 0, sm, getLangOpts());
  const clang::FullSourceLoc endFullSourceLoc(endSourceLoc, sm);
  if (endFullSourceLoc.isInvalid()) {
    return res;
  }
  res.endLine = endFullSourceLoc.getExpansionLineNumber();
  res.endColumn = endFullSourceLoc.getExpansionColumnNumber();

  // [)
  res.startOffset = startFullSourceLoc.getFileOffset();
  res.endOffset = endFullSourceLoc.getFileOffset();

  res.filename = "";

  res.isValid = true;

  return res;
}

std::string ASTGlobal::dumpDecl(const clang::Decl *decl) const {
  if (decl == nullptr) {
    return "nullptr";
  }

  std::ostringstream oss;

  oss << "[" << decl->getDeclKindName() << "] " << decl << " ";

  if (auto *namedDecl = llvm::dyn_cast<clang::NamedDecl>(decl)) {
    oss << namedDecl->getNameAsString() + "(" + getMangledName(namedDecl) + ")";
  }

  oss << getDeclSourceInterval(decl).toString();

  return oss.str();
}

void ASTGlobal::addDisableWarningDecl(const clang::Decl *decl) {
  disableWarningDecls.insert(decl->getCanonicalDecl());
}

bool ASTGlobal::isDisableWarningDecl(const clang::Decl *decl) const {
  return disableWarningDecls.find(decl->getCanonicalDecl()) !=
         disableWarningDecls.end();
}

} // namespace iclang
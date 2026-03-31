#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Mangle.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/Path.h"

// count
//#include "llvm/Support/FileSystem.h"
//#include <unistd.h>
//#include <chrono> // 用于性能统计
// count

using namespace clang;

namespace {

//using Clock = std::chrono::high_resolution_clock;
//// 用于存储各阶段累计时间的结构
//struct ProfileData {
//  uint64_t ShardLoadNs = 0;
//  uint64_t TotalVisitorNs = 0;
//  uint64_t MangleNs = 0;
//  uint64_t LookupNs = 0;
//  size_t VisitCount = 0;
//  size_t MangleCount = 0;
//  size_t HitCount = 0;
//};

class IClangVisitor : public RecursiveASTVisitor<IClangVisitor> {
  ASTContext &Context;
  const llvm::StringSet<> &DiscardList;
  MangleContext *MC;

//  ProfileData &Prof;
  llvm::SmallString<256> MangleBuf;
//  size_t &HitCount;
//  std::set<std::string> &HitSymbols;

public:
  IClangVisitor(ASTContext &Context, const llvm::StringSet<> &List, MangleContext *Mangle)
//                size_t &Counter, std::set<std::string> &SymSet)
      : Context(Context), DiscardList(List), MC(Mangle) {}

  // option
//  bool shouldVisitImplicitCode() const { return true; }
//  bool shouldVisitTemplateInstantiations() const { return true; }

  // 1. 访问所有函数声明（包括命名空间内的、类成员、内联函数）
  bool VisitFunctionDecl(FunctionDecl *D) {
//    Prof.VisitCount++;
    if (!D) return true;

//    if (!D->hasExternalFormalLinkage()) return true;
    // 构造/析构函数多变体匹配
    if (auto *CD = dyn_cast<CXXConstructorDecl>(D)) {
      for (auto Type : {Ctor_Complete, Ctor_Base, Ctor_Comdat})
        checkAndApply(D, GlobalDecl(CD, Type));
    } else if (auto *DD = dyn_cast<CXXDestructorDecl>(D)) {
      for (auto Type : {Dtor_Deleting, Dtor_Complete, Dtor_Base})
        checkAndApply(D, GlobalDecl(DD, Type));
    } else {
      checkAndApply(D, GlobalDecl(D));
    }
    return true;
  }

  bool VisitVarDecl(VarDecl *D) {
    if (!D || D->isInvalidDecl() || !D->hasExternalFormalLinkage()) return true;
    checkAndApply(D, GlobalDecl(D));
    return true;
  }

  // 2. 访问类定义（针对 VTable/RTTI）
//  bool VisitCXXRecordDecl(CXXRecordDecl *RD) {
//    if (!RD || !RD->isCompleteDefinition() || RD->isDependentType()) return true;
//
//    // 虚表匹配
//    if (RD->isDynamicClass()) {
//      std::string VTableName;
//      llvm::raw_string_ostream VOS(VTableName);
//      MC->mangleCXXVTable(RD, VOS);
//      VOS.flush();
//      if (DiscardList.count(VTableName)) applyAttr(RD, VTableName);
//    }
//
//    // RTTI 匹配
//    if (Context.getLangOpts().RTTI) {
//      QualType Ty = Context.getRecordType(RD);
//      std::string TI, TS;
//      llvm::raw_string_ostream TIOS(TI), TSOS(TS);
//      MC->mangleCXXRTTI(Ty, TIOS);
//      MC->mangleCXXRTTIName(Ty, TSOS);
//      TIOS.flush(); TSOS.flush();
//      if (DiscardList.count(TI)) applyAttr(RD, TI);
//      if (DiscardList.count(TS)) applyAttr(RD, TS);
//    }
//    return true;
//  }
  bool VisitCXXRecordDecl(CXXRecordDecl *RD) {
    if (!RD || !RD->isCompleteDefinition() || RD->isDependentType())
      return true;

    if (RD->isDynamicClass()) {
      MangleBuf.clear();
      llvm::raw_svector_ostream OS(MangleBuf);

//      auto StartM = Clock::now();
      MC->mangleCXXVTable(RD, OS);
//      Prof.MangleNs += std::chrono::duration_cast<std::chrono::nanoseconds>(
//                           Clock::now() - StartM)
//                           .count();

      doApply(RD, MangleBuf);
    }
    return true;
  }

private:
//  void checkAndApply(NamedDecl  *D, GlobalDecl GD) {
//    std::string MName;
//    llvm::raw_string_ostream OS(MName);
//
//    auto StartM = Clock::now();
//    Prof.MangleCount++;
//    if (MC->shouldMangleDeclName(D)) {
//      MC->mangleName(GD, OS);
//    } else {
////      OS << D->getNameAsString();
//        if (const IdentifierInfo *II = D->getIdentifier())
//          MangleBuf = II->getName();
//        else
//          MangleBuf = D->getNameAsString();
//    }
//    Prof.MangleNs += std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - StartM).count();
//    OS.flush();
//
//    if (!MName.empty() && DiscardList.count(MName)) {
//      applyAttr(D, MName);
//    }
//  }
  void checkAndApply(NamedDecl *D, GlobalDecl GD) {
    MangleBuf.clear();
    llvm::raw_svector_ostream OS(MangleBuf);

//    auto StartM = Clock::now();
//    Prof.MangleCount++;
    if (MC->shouldMangleDeclName(D)) {
      MC->mangleName(GD, OS);
    } else {
      if (const IdentifierInfo *II = D->getIdentifier())
        MangleBuf = II->getName();
      else
        MangleBuf = D->getNameAsString();
    }
//    Prof.MangleNs += std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - StartM).count();

    if (!MangleBuf.empty()) {
      doApply(D, MangleBuf);
    }
  }

//  void applyAttr(NamedDecl *D, const std::string &MName) {
//    if (!D->hasAttr<NoDebugAttr>()) {
//      D->addAttr(NoDebugAttr::CreateImplicit(Context));
//    }
//  }
  void doApply(NamedDecl *D, StringRef Name) {
//    auto StartL = Clock::now();
    auto it = DiscardList.find(Name);
//    Prof.LookupNs += std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - StartL).count();

    if (it != DiscardList.end()) {
      if (!D->hasAttr<NoDebugAttr>()) {
        D->addAttr(NoDebugAttr::CreateImplicit(Context));
//        Prof.HitCount++;
      }
    }
  }
};

class IClangConsumer : public ASTConsumer {
  CompilerInstance &Instance;
  llvm::StringSet<> DiscardList;
  std::unique_ptr<MangleContext> MC;

  IClangVisitor Visitor;
//  ProfileData Prof;
//  std::string FileName;

  //count
//  size_t HitCount = 0;               // 1. 定义计数器
//  std::string CurrentInFile;          // 用于记录当前文件名
//  std::set<std::string> HitSymbols;
  //count

public:
  IClangConsumer(CompilerInstance &Instance, llvm::StringSet<> List)
      : Instance(Instance), DiscardList(std::move(List)),
        MC(Instance.getASTContext().createMangleContext()),
        Visitor(Instance.getASTContext(), DiscardList, MC.get()){
//    Prof.ShardLoadNs = LoadTime;
  }

  bool HandleTopLevelDecl(DeclGroupRef DG) override {
//    auto Start = Clock::now();
    for (Decl *D : DG) {
      Visitor.TraverseDecl(D);
    }
//    Prof.TotalVisitorNs += std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - Start).count();
    return true;
  }

  void HandleCXXImplicitFunctionInstantiation(FunctionDecl *D) override {
//    auto Start = Clock::now();
    Visitor.TraverseDecl(D);
//    Prof.TotalVisitorNs += std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - Start).count();
  }

  void HandleTagDeclDefinition(TagDecl *D) override {
//    auto Start = Clock::now();
    if (auto *RD = dyn_cast<CXXRecordDecl>(D)) {
      Visitor.VisitCXXRecordDecl(RD);
    }
//    Prof.TotalVisitorNs += std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - Start).count();
  }

  void HandleCXXStaticMemberVarInstantiation(VarDecl *D) override {
//    auto Start = Clock::now();
//    IClangVisitor Visitor(Instance.getASTContext(), DiscardList, MC.get(), HitCount, HitSymbols);
    Visitor.VisitVarDecl(D);
//    Prof.TotalVisitorNs += std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - Start).count();
  }

  // count
//  void HandleTranslationUnit(ASTContext &Context) override {
//
//    std::string StatsDir = "/root/llvm-project/iclang_prof";
//    llvm::sys::fs::create_directories(StatsDir);
//
//    // 2. 构建文件名: pid + 原文件名.prof
//    std::string BaseName = llvm::sys::path::filename(FileName).str();
////    std::string OutPath = StatsDir + "/" + std::to_string(getpid()) + "_" + BaseName + ".prof";
//
//    // 3. 写入 CSV 格式数据
//    std::error_code EC;
//    llvm::raw_fd_ostream OS(OutPath, EC);
//    if (!EC) {
//      // 头部说明: File, LoadMs, VisitorMs, MangleMs, MangleCalls, LookupMs, VisitCount, HitCount
//      OS << FileName << ","
//         << Prof.ShardLoadNs / 1000000.0 << ","
//         << Prof.TotalVisitorNs / 1000000.0 << ","
//         << Prof.MangleNs / 1000000.0 << ","
//         << Prof.MangleCount << ","
//         << Prof.LookupNs / 1000000.0 << ","
//         << Prof.VisitCount << ","
//         << Prof.HitCount << "\n";
//    }
//    // 1. 统计命中数量（原有逻辑）
////    saveHitCount();
//
//    // 2. 统计未命中的符号明细
////    saveMissedSymbols();
//
//  }
  // count

private:
//  void saveHitCount() {
//    std::string StatsDir = "/root/llvm-project/iclang_stats";
//    llvm::sys::fs::create_directories(StatsDir);
//    std::string FileName = llvm::sys::path::filename(CurrentInFile).str();
//    std::string StatsPath = StatsDir + "/" + std::to_string(getpid()) + "_" + FileName + ".cnt";
//    std::error_code EC;
//    llvm::raw_fd_ostream OS(StatsPath, EC);
//    if (!EC) OS << HitCount << "\n";
//  }

//  void saveMissedSymbols() {
//    std::string MissedDir = "/root/llvm-project/iclang_missed";
//    llvm::sys::fs::create_directories(MissedDir);
//
//    std::string FileName = llvm::sys::path::filename(CurrentInFile).str();
//    std::string MissedPath = MissedDir + "/" + std::to_string(getpid()) + "_" + FileName + ".txt";
//
//    std::error_code EC;
//    llvm::raw_fd_ostream OS(MissedPath, EC);
//    if (EC) return;
//
//    size_t miss_num = 0;
//    for (const auto &sym : DiscardList) {
//      // 如果 LLD 丢弃清单里的符号，在 HitSymbols 里找不到，说明没覆盖到
//      if (HitSymbols.find(sym) == HitSymbols.end()) {
//        OS << sym << "\n";
//        miss_num++;
//      }
//    }
//    // llvm::errs() << "[IClang] Missed symbols for " << FileName << ": " << miss_num << "\n";
//  }
};

class IClangAction : public PluginASTAction {
  std::string MetadataDir; // 现在这是一个文件夹路径
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef InFile) override {
//    auto StartLoad = Clock::now();
    std::string OutputStr = CI.getFrontendOpts().OutputFile;
    llvm::SmallString<256> ObjPath(OutputStr.empty() ? InFile : OutputStr);
    if (OutputStr.empty()) llvm::sys::path::replace_extension(ObjPath, ".o");

    // 2. 验证逻辑：统一转为物理真实路径 (解决软链接/符号链接问题)
//    llvm::SmallString<256> RealObjPath;
    llvm::sys::fs::make_absolute(ObjPath); // 先转绝对
//    if (llvm::sys::fs::real_path(ObjPath, RealObjPath)) {
//      // 如果获取真实路径失败（可能文件还没生成），就用绝对路径
//      RealObjPath = ObjPath;
//    }

    // --- 验证打印 A: 打印原始路径和处理后的路径 ---
//    llvm::errs() << "[IClang-Verify] Input File: " << InFile << "\n";
//    llvm::errs() << "[IClang-Verify] Final ObjPath used for Hash: " << RealObjPath << "\n";

    // 3. 计算分片哈希 (使用 RealObjPath)
    llvm::MD5 Hash;
    Hash.update(ObjPath);
    llvm::MD5::MD5Result Result;
    Hash.final(Result);
    SmallString<32> HashStr;
    llvm::MD5::stringifyResult(Result, HashStr);

    // --- 验证打印 B: 打印哈希值 ---
//    llvm::errs() << "[IClang-Verify] Calculated MD5: " << HashStr << "\n";

    // 4. 构建分片文件路径
    SmallString<256> ShardPath(MetadataDir);
//    if (!llvm::sys::fs::is_directory(MetadataDir)) {
//      llvm::errs() << "[IClang-Error] MetadataDir is NOT a directory: " << MetadataDir << "\n";
//    }
    llvm::sys::path::append(ShardPath, HashStr + ".list");

    // --- 验证打印 C: 检查文件是否存在 ---
//    if (!llvm::sys::fs::exists(ShardPath)) {
//      llvm::errs() << "[IClang-Verify] MISSED! Shard file not found at: " << ShardPath << "\n";
//    } else {
//      llvm::errs() << "[IClang-Verify] HIT! Loading shard: " << ShardPath << "\n";
//    }

    llvm::StringSet<> ShardList;
    if (auto BufferOrErr = llvm::MemoryBuffer::getFile(ShardPath)) {
      StringRef Content = BufferOrErr.get()->getBuffer();
      SmallVector<StringRef, 16> Lines;
      Content.split(Lines, '\n', -1, false);
      for (StringRef L : Lines) {
        if (!L.empty()) ShardList.insert(L.trim());
      }
    }
//    uint64_t LoadTime = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - StartLoad).count();
    if (ShardList.empty()) return std::make_unique<ASTConsumer>();
    return std::make_unique<IClangConsumer>(CI, std::move(ShardList));
  }

  bool ParseArgs(const CompilerInstance &CI, const std::vector<std::string> &Args) override {
    for (const auto &Arg : Args) {
      size_t pos = Arg.find("-path=");
      if (pos != std::string::npos) {
        // 动态定位 '=' 的位置，取其后的内容
        MetadataDir = Arg.substr(pos + 6);
        // 如果开头还是有 '='，再去掉它 (防御性编程)
        if (!MetadataDir.empty() && MetadataDir[0] == '=') {
          MetadataDir = MetadataDir.substr(1);
        }
      }
    }
    return true;
  }
};
} // namespace

// 注册插件
static FrontendPluginRegistry::Add<IClangAction>
    X("iclang-reducer", "Reduces debug info based on LLD feedback");
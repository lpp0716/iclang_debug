#include "iclang/Driver/ShareDriver.h"

#include "illvm/Support/FileSystem.h"
#include "illvm/Support/Diagnostics.h"
#include "illvm/Support/Strings.h"
#include "illvm/Support/Time.h"

namespace iclang {

int ShareMasterDriver::run(
    Global &global, const llvm::SmallVector<const char *, 128> &originalArgv,
    const clang::driver::Driver &clangDriver) {
  ILLVM_FCHECK(false, "We haven't implemented ShareMasterDriver yet");
  return 0;
}

int ShareClientDriver::run(
    Global &global, const llvm::SmallVector<const char *, 128> &originalArgv,
    const clang::driver::Driver &clangDriver) {
  ILLVM_FCHECK(false, "We haven't implemented ShareClientDriver yet");
  return 0;
}

static void
configPaths(illvm::BPtr<ShareCheckMetaData> &metaData) {
  const auto preWorkPath = metaData->iClangDirPath[PrevDir];
  const auto workPath = metaData->iClangDirPath[CurDir];

  metaData->ppPath = illvm::FileSystem::linkPath(workPath, "iclang.i");
  metaData->ppSPath = illvm::FileSystem::linkPath(workPath, "iclangs.i");
}

// outputPath -> iclang.i
// -emit-obj -> -E
static int
clangECompile(const illvm::BPtr<const ShareCheckMetaData> &metaData,
              const clang::driver::Driver &clangDriver,
              const llvm::SmallVector<const char *, 128> &originalArgv) {
  return DriverBase::compile(clangDriver, originalArgv, -1, "",
                             metaData->outputIdx, metaData->ppPath.c_str(),
                             metaData->emitObjIdx, "-E", {}, {});
}

// inputPath -> iclang.i
static int
clangEOCompile(const illvm::BPtr<const ShareCheckMetaData> &metaData,
               const clang::driver::Driver &clangDriver,
               const llvm::SmallVector<const char *, 128> &originalArgv) {
  return DriverBase::compile(clangDriver, originalArgv, metaData->inputIdx,
                             metaData->ppPath.c_str(), -1, "", -1, "", {}, {});
}

// inputPath -> iclangs.i
static int
clangESOCompile(const illvm::BPtr<const ShareCheckMetaData> &metaData,
                const clang::driver::Driver &clangDriver,
                const llvm::SmallVector<const char *, 128> &originalArgv) {
  return DriverBase::compile(clangDriver, originalArgv, metaData->inputIdx,
                             metaData->ppSPath.c_str(), -1, "", -1, "", {}, {});
}

static void clearInterval(std::string &content,
                          const illvm::Interval &interval) {
  for (long long i = interval.first; i < interval.second; i++) {
    content[i] = ' ';
    // Note: UTF-8 Chinese.
  }
}

// Line of code.
static int getLoc(const std::string &filepath) {
  int res = 0;
  const auto lines = illvm::FileSystem::readLines(filepath);
  for (const auto &line : lines) {
    if (illvm::Strings::trimWhitespace(line).empty()) {
      continue;
    }
    res += 1;
  }
  return res;
}

int ShareCheckDriver::run(
    Global &global,
    const llvm::SmallVector<const char *, 128> &originalArgv,
    const clang::driver::Driver &clangDriver) {

  assert(global.getIClangMode() == IClangMode::ShareCheckMode);

  auto metaData = global.getMetaData<ShareCheckMetaData>();

  configPaths(metaData);

  // clang++ -E -o iclang.i inputPath.
  //   Failed: set recover flag, backup to clang.
  // original: clang++ -c -o outputPath iclang.i.
  //   Failed: set recover flag, backup to clang.
  // master: clang++ -c -o outputPath iclang.i (refed symbol analysis).
  //   Failed: fatal.
  // client:
  //   * iclang.i --(refed symbol analysis results)--> iclangs.i.
  //   * clang++ -c -o outputPath iclangs.i.
  //   Failed: fatal.

  // clang++ -E -o iclang.i inputPath.
  int res = clangECompile(metaData.constCopy(), clangDriver, originalArgv);
  if (res != 0) {
    metaData->recoverFlag = true;
    DriverBase::fini(global);
    return res;
  }

  long long startTsMs = 0;
  long long endTsMs = 0;

  // original.
  startTsMs = illvm::Time::currentTsMs();
  res = clangEOCompile(metaData.constCopy(), clangDriver, originalArgv);
  if (res != 0) {
    metaData->recoverFlag = true;
    DriverBase::fini(global);
    return 0;
  }
  endTsMs = illvm::Time::currentTsMs();
  metaData->originalTimeMs = endTsMs - startTsMs;

  // master.
  startTsMs = illvm::Time::currentTsMs();
  metaData->enableRefedSymbolAnalysisFlag = true;
  res = clangEOCompile(metaData.constCopy(), clangDriver, originalArgv);
  ILLVM_FCHECK(res == 0, "master error!");
  metaData->enableRefedSymbolAnalysisFlag = false;
  endTsMs = illvm::Time::currentTsMs();
  metaData->masterTimeMs = endTsMs - startTsMs;

  // client.
  startTsMs = illvm::Time::currentTsMs();
  auto content = illvm::FileSystem::readAll(metaData->ppPath);
  for (const auto &interval : metaData->unRefedDeclIntervals) {
    clearInterval(content, interval);
  }
  illvm::FileSystem::saveStr(metaData->ppSPath, content);
  // FileSystem::cpFile(global.ppPath, global.ppSPath);

  res = clangESOCompile(metaData.constCopy(), clangDriver, originalArgv);
  ILLVM_FCHECK(res == 0, "client error!");
  endTsMs = illvm::Time::currentTsMs();
  metaData->clientTimeMs = endTsMs - startTsMs;

  metaData->originalPPLoc = getLoc(metaData->ppPath);
  metaData->funcXedPPLoc = getLoc(metaData->ppSPath);

  DriverBase::fini(global);

  return res;
}

} // namespace iclang
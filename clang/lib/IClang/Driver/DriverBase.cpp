#include "iclang/Driver/DriverBase.h"

#include "illvm/Support/Diagnostics.h"
#include "illvm/Support/FileSystem.h"
#include "illvm/Support/Strings.h"
#include "illvm/Support/Time.h"

#include "llvm/ADT/SmallVector.h"

namespace iclang {

static std::string
parseIClangArg(const llvm::SmallVector<const char *, 128> &originalArgv) {
  std::string iClangArg;
  const char *iClangEnvStr = getenv("ICLANG");
  if (iClangEnvStr != nullptr) {
    iClangArg = iClangEnvStr;
  } else {
    for (size_t i = 0; i < originalArgv.size(); i++) {
      const std::string arg = originalArgv[i];
      if (illvm::Strings::hasPrefix(arg, "-iclang=")) {
        iClangArg = arg.substr(8);
      }
    }
  }
  if (iClangArg.size() >= 2) {
    const char beginC = iClangArg[0];
    const char endC = iClangArg[iClangArg.size() - 1];
    if ((beginC == '"' && endC == '"') || (beginC == '\'' && endC == '\'')) {
      iClangArg = iClangArg.substr(1, iClangArg.size() - 2);
    }
  }
  return iClangArg;
}

static bool assembleJobCheck(const clang::driver::Action::ActionClass &kind) {
  return kind == clang::driver::Action::AssembleJobClass;
}

static bool checkArgs(const std::vector<clang::driver::InputInfo> &inputInfos,
                      const std::vector<std::string> &outputFilenames,
                      const llvm::SmallVector<const char *, 128> &originalArgv,
                      std::string &inputPath, std::string &outputPath,
                      int &inputIdx, int &outputIdx, int &emitObjIdx) {
  if (inputInfos.size() != 1 || !inputInfos[0].isFilename() ||
      outputFilenames.size() != 1) {
    return false;
  }

  inputPath = inputInfos[0].getFilename();
  outputPath = outputFilenames[0];

  inputIdx = -1;
  outputIdx = -1;
  emitObjIdx = -1;

  std::string opt;
  std::string prevOpt;
  for (size_t i = 0; i < originalArgv.size(); i++, prevOpt = opt) {
    opt = originalArgv[i];
    if (opt == inputPath && prevOpt != "-main-file-name") {
      if (inputIdx == -1) {
        inputIdx = i;
      } else {
        return false;
      }
    } else if (opt == outputPath && prevOpt == "-o") {
      if (outputIdx == -1) {
        outputIdx = i;
      } else {
        return false;
      }
    } else if (opt == "-emit-obj") {
      if (emitObjIdx == -1) {
        emitObjIdx = i;
      } else {
        return false;
      }
    }
    if (prevOpt == "-x" && opt == "c") {
      return false;
    }
  }
  if (inputIdx == -1 || outputIdx == -1 || emitObjIdx == -1) {
    return false;
  }
  return true;
}

static void
configPaths(Global &global, const std::string &prevWorkPath,
            const std::string &workPath,
            const llvm::SmallVector<const char *, 128> &originalArgv) {
  auto metaData = global.getMetaData<MetaData>();

  metaData->currentPath = illvm::FileSystem::getCurrentPath();
  metaData->originalCommand = illvm::Strings::argVToArgs(originalArgv);

  metaData->iClangDirPath[IClangDir::PrevDir] = prevWorkPath;
  metaData->iClangDirPath[IClangDir::CurDir] = workPath;

  metaData->compileJsonPath[IClangDir::PrevDir] =
      illvm::FileSystem::linkPath(prevWorkPath, "compile.json");
  metaData->compileJsonPath[IClangDir::CurDir] =
      illvm::FileSystem::linkPath(workPath, "compile.json");

  metaData->logPath = illvm::FileSystem::linkPath(workPath, "log.txt");

  illvm::Logger::getInstance().initLogPath(metaData->logPath);
}

bool DriverBase::init(
    Global &global,
    const clang::driver::Action::ActionClass &kind,
    const std::vector<clang::driver::InputInfo> &inputInfos,
    const std::vector<std::string> &outputFilenames,
    const llvm::SmallVector<const char *, 128> &originalArgv) {
  // Load IClang config.
  const auto iClangArg = parseIClangArg(originalArgv);
  global.init(iClangArg);

  if (global.isIClangMode(IClangMode::ClangMode)) {
    return false;
  }

  auto metaData = global.getMetaData<MetaData>();

  // Record Start time stamp.
  metaData->startTs = illvm::Time::currentTsMs();

  // Check:
  // * AssembleJob.
  // * Only 1 matching input / output / '-emit-obj'.
  if (!assembleJobCheck(kind) ||
      !checkArgs(inputInfos, outputFilenames, originalArgv, metaData->inputPath,
                 metaData->outputPath, metaData->inputIdx, metaData->outputIdx,
                 metaData->emitObjIdx)) {
    return false;
  }
  // * Important: concurrent compilation, only one can work.
  const std::string prevWorkPath = metaData->outputPath + ".iclang";
  const std::string workPath = metaData->outputPath + ".iclangtmp";
  if (auto err = illvm::FileSystem::mkdir(workPath)) {
    llvm::consumeError(std::move(err));
    illvm::Logger::getInstance().warning(
        __PRETTY_FUNCTION__, "Detected " + workPath + ", back to Clang");
    return false;
  }

  // (2) Config IClang global path.
  configPaths(global, prevWorkPath, workPath, originalArgv);

  return true;
}

int DriverBase::compile(
    const clang::driver::Driver &clangDriver,
    const llvm::SmallVector<const char *, 128> &originalArgv,
    const int inputIdx, const char *newInputPath, const int outputIdx,
    const char *newOutputPath, const int emitIdx, const char *newEmit,
    const std::unordered_map<std::string, int> &skipArgs,
    const std::vector<const char *> &newArgs) {
  llvm::SmallVector<const char *, 128> argv;

  const int sz = originalArgv.size();
  for (int i = 0; i < sz; i++) {
    if (i == inputIdx) {
      argv.push_back(newInputPath);
      continue;
    }
    if (i == outputIdx) {
      argv.push_back(newOutputPath);
      continue;
    }
    if (i == emitIdx) {
      argv.push_back(newEmit);
      continue;
    }

    std::string arg = originalArgv[i];
    const auto it = skipArgs.find(arg);
    if (it != skipArgs.end()) {
      i += it->second;
      continue;
    }

    // Adjust some args corresponding to io.
    if (arg == "-main-file-name") {
      argv.push_back("-main-file-name");
      argv.push_back(newInputPath);
      i++;
      continue;
    }
    if (arg == "-MT") {
      argv.push_back("-MT");
      argv.push_back(newOutputPath);
      i++;
      continue;
    }

    argv.push_back(originalArgv[i]);
  }

  for (const auto &arg : newArgs) {
    argv.push_back(arg);
  }

  return clangDriver.CC1Main(argv);
}

int DriverBase::clangCompile(
    const clang::driver::Driver &clangDriver,
    const llvm::SmallVector<const char *, 128> &originalArgv) {
  return compile(clangDriver, originalArgv, -1, "", -1, "", -1, "", {}, {});
}

void DriverBase::fini(Global &global) {
  auto metaData = global.getMetaData<MetaData>();

  // End time stamp.
  metaData->endTs = illvm::Time::currentTsMs();
  metaData->totalTimeMs = metaData->endTs - metaData->startTs;
  metaData->backTimeMs = metaData->endTs - metaData->midTs;
  // Gen .iclangtmp/compile.json.
  Global::saveMetaDataToFile(metaData->compileJsonPath[CurDir], metaData);
  // rm .iclang, mv .iclangtmp .iclang
  illvm::FileSystem::mvFile(metaData->iClangDirPath[CurDir],
                            metaData->iClangDirPath[PrevDir]);
}

int DriverBase::recover(
    Global &global, const clang::driver::Driver &clangDriver,
    const llvm::SmallVector<const char *, 128> &originalArgv) {
  const auto &logger = illvm::Logger::getInstance();
  auto metaData = global.getMetaData<MetaData>();

  logger.warning(__PRETTY_FUNCTION__,
                 "Compilation error, try rolling back to Clang.");

  global.resetIClangMode();
  const int res = clangCompile(clangDriver, originalArgv);

  if (res != 0) {
    logger.warning(__PRETTY_FUNCTION__,
                   "Clang also encountered compilation errors, "
                   "please check your source code.");
  } else {
    metaData->recoverFlag = true;
    logger.warning(__PRETTY_FUNCTION__,
                   "IClang internal error, enable recovery mode, "
                   "we will no longer process this file");
  }
  fini(global);
  return res;
}

int DriverBase::runBase(Global &global,
                   const llvm::SmallVector<const char *, 128> &originalArgv,
                   const clang::driver::Driver &clangDriver) {
  const int res = clangCompile(clangDriver, originalArgv);
  fini(global);
  return res;
}

} // namespace iclang
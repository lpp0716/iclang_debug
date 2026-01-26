//===--- Global.hpp - IClang global function, data -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// IClang global function, data.
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_GLOBAL_HPP
#define ICLANG_GLOBAL_HPP

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/Path.h"

#include "clang/AST/AST.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Mangle.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace iclang {

class String {
public:
  static std::string trimWhitespace(const std::string &str) {
    const auto start =
        std::find_if_not(str.begin(), str.end(), [](const unsigned char ch) {
          return std::isspace(ch);
        });

    const auto end =
        std::find_if_not(str.rbegin(), str.rend(), [](const unsigned char ch) {
          return std::isspace(ch);
        }).base();

    if (start >= end) {
      return ""; // All spaces or empty string
    }

    return std::string(start, end);
  }

  static bool hasPrefix(const std::string &str, const std::string &prefix) {
    if (str.size() < prefix.size()) {
      return false;
    }
    return str.compare(0, prefix.size(), prefix) == 0;
  }

  static std::vector<std::string> splitString(const std::string &str,
                                              const char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;

    while (std::getline(ss, item, delimiter)) {
      result.push_back(trimWhitespace(item));
    }

    return result;
  }

  static std::string
  argVToArgs(const llvm::SmallVector<const char *, 128> &argv) {
    std::string args;
    if (!argv.empty()) {
      args.append(argv[0]);
    }
    for (size_t i = 1; i < argv.size(); i++) {
      args.append(" ");
      args.append(argv[i]);
    }
    return args;
  }
};

class Time {
public:
  static long long currentTsMs() {
    const auto now = std::chrono::system_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch());
    return duration.count();
  }

  static std::string currentDateTime() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    const std::tm local_tm = *std::localtime(&now_time_t);
    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
  }
};

class Logger {
private:
  std::string logPath;

  Logger() {}

public:
  static Logger &getInstance() {
    static Logger instance;
    return instance;
  }

  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  void initLogPath(const std::string &_logPath) { logPath = _logPath; }

  void info(const std::string &msg) const {
    llvm::errs() << "[IClang Info] " << msg << "\n";
    writeLog("Info", msg);
  }

  void debug(const std::string &msg) const {
    llvm::errs() << "[IClang Debug] " << msg << "\n";
    writeLog("Debug", msg);
  }

  void warning(const std::string &msg) const {
    llvm::errs() << "[IClang Warning] " << msg << "\n";
    writeLog("Warning", msg);
  }

  void error(const std::string &msg) const {
    llvm::errs() << "[IClang Error] " << msg << "\n";
    writeLog("Error", msg);
  }

  void fatal(const std::string &msg) const {
    llvm::errs() << "[IClang Fatal] " << msg << "\n";
    writeLog("Fatal", msg);
    exit(1);
  }

  void writeLog(const std::string &level, const std::string &msg) const {
    if (logPath.empty()) {
      return;
    }
    // Append IClang log
    const std::string timeLevelMsg =
        Time::currentDateTime() + " [" + level + "] " + msg;
    std::ofstream logFile(logPath, std::ios::app);
    if (!logFile.is_open()) {
      llvm::errs() << "Write IClang log error: can not open " << logPath
                   << "\n";
      logFile.close();
      exit(1);
    }
    logFile << timeLevelMsg << "\n";
    logFile.close();
  }
};

class FileSystem {
public:
  static std::string getCurrentPath() {
    llvm::SmallString<256> res;
    const auto ec = llvm::sys::fs::current_path(res);
    if (ec) {
      Logger::getInstance().fatal("Error getting current path: " +
                                  ec.message());
    }
    return res.str().str();
  }

  // Auto remove dot.
  static std::string toAbsPath(const std::string &filepath) {
    llvm::SmallString<256> absolutePath(filepath);
    if (llvm::sys::fs::make_absolute(absolutePath)) {
      Logger::getInstance().fatal("Make absolute error: " + filepath);
    }
    llvm::sys::path::remove_dots(absolutePath);
    return absolutePath.str().str();
  }

  static std::string linkPath(const std::string &path1,
                              const std::string &path2) {
    llvm::SmallString<256> res(path1);

    llvm::sys::path::append(res, path2);

    return res.str().str();
  }

  static bool pathEqual(const std::string &path1, const std::string &path2) {
    return llvm::sys::fs::equivalent(path1, path2);
  }

  static bool checkFileExists(const std::string &filepath) {
    return llvm::sys::fs::exists(filepath);
  }

  static long long getLastModificationTime(const std::string &filepath) {
    llvm::sys::fs::file_status status;
    if (llvm::sys::fs::status(filepath, status)) {
      Logger::getInstance().fatal("Can not read the status of file " +
                                  filepath);
    }
    const llvm::sys::TimePoint<> time = status.getLastModificationTime();
    const auto duration = time.time_since_epoch();
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    return milliseconds.count();
  }

  static std::string readAll(const std::string &filepath) {
    const std::ifstream inputFile(filepath);
    if (!inputFile.is_open()) {
      Logger::getInstance().fatal("Can not open file " + filepath);
    }

    std::ostringstream buffer;
    buffer << inputFile.rdbuf();

    return buffer.str();
  }

  // Note: check res.size() == n yourself.
  static std::vector<std::string> readFirstNLines(const std::string &filepath,
                                                  const size_t n) {
    std::ifstream file(filepath);
    std::vector<std::string> result;
    std::string line;

    if (!file.is_open()) {
      Logger::getInstance().fatal("Can not open file " + filepath);
    }

    size_t lineCount = 0;
    while (std::getline(file, line) && lineCount < n) {
      result.push_back(line);
      ++lineCount;
    }

    file.close();
    return result;
  }

  static std::vector<std::string> readLines(const std::string &filepath) {
    std::ifstream infile(filepath);

    if (!infile) {
      Logger::getInstance().fatal("Can not open file " + filepath);
    }

    std::vector<std::string> lines;
    std::string line;

    while (std::getline(infile, line)) {
      lines.push_back(line);
    }

    infile.close();

    return lines;
  }

  static std::vector<std::string> readLinesStartFrom(const std::string &filepath,
                                                  const size_t n) {
    std::ifstream file(filepath);
    std::vector<std::string> result;
    std::string line;

    if (!file.is_open()) {
      Logger::getInstance().fatal("Can not open file " + filepath);
    }

    size_t lineCount = 0;
    while (std::getline(file, line)) {
      if (lineCount >= n) {
        result.push_back(line);
      }
      ++lineCount;
    }

    file.close();
    return result;
  }

  // rm -f file or dir.
  static void rmFile(const std::string &filepath) {
    if (!checkFileExists(filepath)) {
      return;
    }
    const bool isDir = llvm::sys::fs::is_directory(filepath);
    if (isDir) {
      const auto ec = llvm::sys::fs::remove_directories(filepath);
      if (ec) {
        Logger::getInstance().fatal("Can not remove directory " + filepath);
      }
    } else {
      const auto ec = llvm::sys::fs::remove(filepath);
      if (ec) {
        Logger::getInstance().fatal("Can not remove file " + filepath);
      }
    }
  }

  // Auto cover.
  static void mvFile(const std::string &from, const std::string &to) {
    rmFile(to);
    const auto ec = llvm::sys::fs::rename(from, to);
    if (ec) {
      Logger::getInstance().fatal("mv " + from + " to " + to + " failed");
    }
  }

  // Auto cover.
  static void cpFile(const std::string &from, const std::string &to) {
    rmFile(to);
    const auto ec = llvm::sys::fs::copy_file(from, to);
    if (ec) {
      Logger::getInstance().fatal("cp " + from + " to " + to + " failed");
    }
  }

  // Compatible with Windows.
  static void mvDir(const std::string &baseFromDirPath,
    const std::string &baseToDirPath,
    const std::string &relDirPath = "") {
    namespace fs = llvm::sys::fs;
    namespace path = llvm::sys::path;

    const auto &logger = Logger::getInstance();

    const std::string fromDirPath = linkPath(baseFromDirPath, relDirPath);
    const std::string toDirPath = linkPath(baseToDirPath, relDirPath);

    std::error_code ec = fs::create_directories(toDirPath);
    if (ec) {
      logger.fatal("Failed to create target directory " + toDirPath);
    }

    for (fs::directory_iterator it(fromDirPath, ec), end; it != end
      && !ec; it.increment(ec)) {
      const auto &entry = *it;
      const std::string fromEntryPath = entry.path();
      const std::string fromEntryName = path::filename(fromEntryPath).str();

      fs::file_status status;
      fs::status(fromEntryPath, status);
      if (fs::is_regular_file(status)) {
        const std::string toEntryPath = linkPath(toDirPath, fromEntryName);
        mvFile(fromEntryPath, toEntryPath);
      } else if (fs::is_directory(status)) {
        const std::string newRelDirPath = linkPath(relDirPath, fromEntryName);
        mvDir(baseFromDirPath, baseToDirPath, newRelDirPath);
      }
    }

    rmFile(fromDirPath);
  }

  // List the first level files (fileFlag) / directories (!fileFlag) under
  // `path`.
  static std::vector<std::string> list(const std::string &path,
    const bool fileFlag) {
    std::vector<std::string> res;

    const auto &logger = Logger::getInstance();

    std::error_code ec;

    llvm::sys::fs::directory_iterator it(path, ec);
    const llvm::sys::fs::directory_iterator end;

    if (ec) {
      logger.fatal("Failed to open directory: " + path + " - " + ec.message());
    }

    for (; it != end; it.increment(ec)) {
      if (ec) {
        logger.fatal("Error during iteration: " + ec.message());
      }

      const std::string entryPath = it->path();
      if (fileFlag) {
        if (!llvm::sys::fs::is_directory(entryPath)) {
          res.push_back(entryPath);
        }
      } else {
        if (llvm::sys::fs::is_directory(entryPath)) {
          res.push_back(entryPath);
        }
      }
    }

    return res;
  }

  static void saveStr(const std::string &filepath, const std::string &str) {
    std::ofstream ofs(filepath);
    if (!ofs.is_open()) {
      Logger::getInstance().fatal("Can not open " + filepath);
    }
    ofs << str;
    ofs.close();
  }

  static void saveVector(const std::string &filepath,
    const std::vector<std::string> &vec) {
    std::ostringstream oss;
    for (const auto &elem : vec) {
      oss << elem << std::endl;
    }
    std::ofstream ofs(filepath);
    if (!ofs.is_open()) {
      Logger::getInstance().fatal("Can not open " + filepath);
    }
    ofs << oss.str();
    ofs.close();
  }

  static void saveSet(const std::string &filepath,
                      const std::unordered_set<std::string> &st,
                      bool ordered = true) {
    std::ostringstream oss;
    if (ordered) {
      const std::set<std::string> orderedSet(st.begin(), st.end());
      for (const auto &elem : orderedSet) {
        oss << elem << std::endl;;
      }
    } else {
      for (const auto &elem : st) {
        oss << elem << std::endl;;
      }
    }
    std::ofstream ofs(filepath);
    if (!ofs.is_open()) {
      Logger::getInstance().fatal("Can not open " + filepath);
    }
    ofs << oss.str();
    ofs.close();
  }
};

class BinFile {
private:
  std::string filePath;

  void *fileData;

  std::streamsize fileSize;

public:
  explicit BinFile(const std::string &path)
      : filePath(path), fileData(nullptr), fileSize(0) {
    const auto &logger = Logger::getInstance();

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
      logger.fatal("Failed to open file: " + filePath);
    }

    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    fileData = malloc(fileSize);
    if (!fileData) {
      logger.fatal("Memory allocation failed for file: " + filePath);
    }

    if (!file.read(static_cast<char *>(fileData), fileSize)) {
      free(fileData);
      logger.fatal("Error reading file: " + filePath);
    }

    file.close();
  }

  ~BinFile() {
    if (fileData) {
      free(fileData);
    }
  }

  std::streamsize getFileSize() const { return fileSize; }

  const std::string &getFilePath() const { return filePath; }

  const char *readBytes(const int offset) const {
    const auto &logger = Logger::getInstance();
    if (offset < 0 || offset >= fileSize) {
      logger.fatal("Offset out of range in file: " + filePath);
    }
    return static_cast<char *>(fileData) + offset;
  }
};

class Global {
public:
  // -Oi.
  bool iClangOpt = false;

  // Enable IClang, otherwise equivalent to Clang.
  bool iClangFlag = false;

  // Can be incrementally compiled safely.
  bool incFlag = false;

  // Roll back to Clang.
  bool recoverFlag = false;

  // ICLANG="-dev"
  // Record some intermediate results for IClang developer.
  bool iClangDevFlag = false;

  // ICLANG="-profile"
  // Only profiling.
  bool iClangProfileFlag = false;

  // The current compilation path.
  std::string currentPath = "";

  // Compilation command.
  std::string originalCommand = "";

  // The index of input file in compilation command.
  int inputIdx = -1;

  // The index of output file in compilation command.
  int outputIdx = -1;

  // The index of '-emit-obj' file in compilation command.
  int emitObjIdx = -1;

  // Input file path.
  std::string inputPath = "";

  // -I + input file parent dir.
  std::string iInputDir = "";

  // Output file path.
  std::string outputPath = "";

  enum IClangDir {
    PrevDir = 0, // .iclang
    CurDir = 1,  // .iclangtmp
  };

  // 0: .iclang 1: .iclangtmp.
  std::string iClangDirPath[2];

  // compile.txt.
  std::string compileTxtPath[2];

  // log.txt
  std::string logPath = "";

  // iclang.cache.
  std::string cachePath[2];

  // iclang.h.
  std::string cacheHeaderPath[2];

  // iclang.cpp.
  std::string cacheSrcPath = "";

  // prev.o.
  std::string prevOPath = "";

  // partial.o.
  std::string partialOPath = "";

  // output.o
  std::string outputOPath = "";

  // funcx.txt, only for dev mode.
  std::string funcXTxtPath = "";

  // Compilation time.
  long long totalTime = 0;

  // IClang funcX time.
  long long funcXTime = 0;

  // IClang funcV time.
  long long funcVTime = 0;

  // IClang start time stamp.
  long long startTs = 0;

  // IClang end time stamp.
  long long endTs = 0;

  // Code region above the first Decl in the source file.
  // For example:
  // #ifndef ICLANG_GLOBAL_HPP               *|
  // #define ICLANG_GLOBAL_HPP                |
  // #include <stirng>                        |
  // #include <vector>                        |-> top include region
  // #include <map>                           |
  // ...                                     *| // the first "#include" before
  // the first Decl. namespace iclang { // the first Decl
  // ...
  // }
  // #endif ICLANG_GLOBAL_HPP
  std::vector<std::string> topIncludeRegion = {};

  // Headers' timestamp, abs path -> timestamp.
  std::map<std::string, long long> headerTs = {};

  // Symbol api names in previous binary file.
  std::unordered_set<std::string> prevAPIs = {};

  // Mangled names of funcXed symbols.
  std::unordered_set<std::string> funcXSet = {};

  // AST context.
  clang::ASTContext *context = nullptr;

  // Mangled name generator.
  std::unique_ptr<clang::ASTNameGenerator> astNameGenerator = nullptr;

public:
  Global() = default;

  void initContext(clang::ASTContext *_context) {
    context = _context;
    astNameGenerator = std::make_unique<clang::ASTNameGenerator>(*context);
  }

  clang::ASTContext &getContext() const { return *context; }

  std::string getMangledName(const clang::NamedDecl *decl) const {
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

  // Format:
  // incFlag
  // recoverFlag
  // currentPath
  // originalCommand
  // inputPath
  // outputPath
  // totalTime
  // funcXTime
  // funcVTime
  // topIncludeRegion
  // headerTs
  std::string serialize() const {
    llvm::json::Object root;

    root["incFlag"] = incFlag;
    root["recoverFlag"] = recoverFlag;

    root["currentPath"] = currentPath;
    root["originalCommand"] = originalCommand;
    root["inputPath"] = inputPath;
    root["outputPath"] = outputPath;

    root["totalTime"] = totalTime;
    root["funcXTime"] = funcXTime;
    root["funcVTime"] = funcVTime;

    llvm::json::Array arr;
    for (const auto &line : topIncludeRegion) {
      arr.push_back(line);
    }
    root["topIncludeRegion"] = llvm::json::Value(std::move(arr));

    llvm::json::Object sub;
    for (auto &kv : headerTs) {
      sub[kv.first] = kv.second;
    }
    root["headerTs"] = llvm::json::Value(std::move(sub));

    const auto rootV = llvm::json::Value(std::move(root));

    return llvm::formatv("{0:2}", rootV).str();
  }

  void deserialize(const std::string &jsonStr) {
    const auto &logger = Logger::getInstance();

    llvm::Expected<llvm::json::Value> valOrErr = llvm::json::parse(jsonStr);
    if (!valOrErr) {
      logger.fatal("Failed to parse JSON: " +
                   llvm::toString(valOrErr.takeError()));
    }

    llvm::json::Value rootV = std::move(*valOrErr);
    auto *root = rootV.getAsObject();
    if (root == nullptr) {
      logger.fatal(
          "Failed to parse JSON: Can not convert jsonStr to json object");
      return;
    }

    incFlag = (*root)["incFlag"].getAsBoolean().value();
    recoverFlag = (*root)["recoverFlag"].getAsBoolean().value();

    currentPath = (*root)["currentPath"].getAsString().value().str();
    originalCommand = (*root)["originalCommand"].getAsString().value().str();
    inputPath = (*root)["inputPath"].getAsString().value().str();
    outputPath = (*root)["outputPath"].getAsString().value().str();

    totalTime = (*root)["totalTime"].getAsInteger().value();
    funcXTime = (*root)["funcXTime"].getAsInteger().value();
    funcVTime = (*root)["funcVTime"].getAsInteger().value();

    auto *arr = (*root)["topIncludeRegion"].getAsArray();
    if (arr == nullptr) {
      logger.fatal("Failed to parse JSON: Can not convert topIncludeRegion to "
                   "json array");
      return;
    }
    topIncludeRegion.clear();
    for (const auto &line : *arr) {
      topIncludeRegion.push_back(line.getAsString().value().str());
    }

    auto *headerTsObj = (*root)["headerTs"].getAsObject();
    if (headerTsObj == nullptr) {
      logger.fatal(
          "Failed to parse JSON: Can not convert headerTs to json object");
      return;
    }
    headerTs.clear();
    for (const auto &kv : *headerTsObj) {
      const std::string key = kv.first.str();
      headerTs[key] = kv.second.getAsInteger().value();
    }
  }

  void serializeToFile(const std::string &filePath) const {
    std::ofstream ofs(filePath);
    if (!ofs.is_open()) {
      Logger::getInstance().fatal("Can not open " + filePath);
    }
    ofs << serialize();
    ofs.close();
  }

  void deserializeFromFile(const std::string &filePath) {
    deserialize(FileSystem::readAll(filePath));
  }

  std::string dumpName(const clang::Decl *decl) const {
    std::string res = "[";
    res += decl->getDeclKindName();
    res += "]";
    if (auto *namedDecl = llvm::dyn_cast<clang::NamedDecl>(decl)) {
      res +=
          namedDecl->getNameAsString() + "(" + getMangledName(namedDecl) + ")";
    }
    return res;
  }
};

class GlobalSingleton {
private:
  GlobalSingleton() {}
  Global global;

public:
  static GlobalSingleton &getInstance() {
    static GlobalSingleton instance;
    return instance;
  }

  GlobalSingleton(const GlobalSingleton &) = delete;
  GlobalSingleton &operator=(const GlobalSingleton &) = delete;

  Global &getGlobal() { return global; }
};

} // namespace iclang

#endif // ICLANG_GLOBAL_HPP

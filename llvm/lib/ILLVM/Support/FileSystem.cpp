#include "illvm/Support/FileSystem.h"

#include <chrono>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "illvm/Support/Diagnostics.h"

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

namespace illvm {

std::string FileSystem::getCurrentPath() {
  llvm::SmallString<256> res;
  const auto ec = llvm::sys::fs::current_path(res);
  ILLVM_FCHECK(!ec, "Get current path error: " + ec.message());
  return res.str().str();
}

std::string FileSystem::toAbsPath(const std::string &filepath) {
  llvm::SmallString<256> absolutePath(filepath);
  const auto ec = llvm::sys::fs::make_absolute(absolutePath);
  ILLVM_FCHECK(!ec,
               "Convert " + filepath + " to abs path error: " + ec.message());
  llvm::sys::path::remove_dots(absolutePath);
  return absolutePath.str().str();
}

std::string FileSystem::linkPath(const std::string &path1,
                                 const std::string &path2) {
  llvm::SmallString<256> res(path1);

  llvm::sys::path::append(res, path2);

  return res.str().str();
}

std::string FileSystem::parentPath(const std::string &path) {
  return llvm::sys::path::parent_path(path).str();
}

bool FileSystem::checkFileExists(const std::string &filepath) {
  return llvm::sys::fs::exists(filepath);
}

long long FileSystem::getLastModificationTime(const std::string &filepath) {
  llvm::sys::fs::file_status status;
  const auto ec = llvm::sys::fs::status(filepath, status);
  ILLVM_FCHECK(!ec, "Get the last modification time of file " + filepath +
                        " error: " + ec.message());
  const llvm::sys::TimePoint<> time = status.getLastModificationTime();
  const auto duration = time.time_since_epoch();
  const auto milliseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(duration);
  return milliseconds.count();
}

std::string FileSystem::readAll(const std::string &filepath) {
  std::ifstream inputFile(filepath);
  ILLVM_FCHECK(inputFile.is_open(), "Can not open file " + filepath);

  std::ostringstream buffer;
  buffer << inputFile.rdbuf();

  inputFile.close();

  return buffer.str();
}

std::vector<std::string> FileSystem::readLines(const std::string &filepath) {
  std::ifstream infile(filepath);

  ILLVM_FCHECK(infile, "Can not open file " + filepath);

  std::vector<std::string> lines;
  std::string line;

  while (std::getline(infile, line)) {
    lines.push_back(line);
  }

  infile.close();

  return lines;
}

// Note: check res.size() == n yourself.
std::vector<std::string>
FileSystem::readFirstNLines(const std::string &filepath, const size_t n) {
  std::ifstream file(filepath);
  std::vector<std::string> result;
  std::string line;

  ILLVM_FCHECK(file.is_open(), "Can not open file " + filepath);

  size_t lineCount = 0;
  while (std::getline(file, line) && lineCount < n) {
    result.push_back(line);
    ++lineCount;
  }

  file.close();
  return result;
}

llvm::Error FileSystem::mkdir(const std::string &dirpath) {
  const std::error_code ec = llvm::sys::fs::create_directory(dirpath, false);
  ILLVM_ECHECK(!ec, "Can not create directory " + dirpath +
                        " error: " + ec.message());
  return llvm::Error::success();
}

static void rmSingleFile(const std::string &filepath) {
  const auto ec = llvm::sys::fs::remove(filepath);
  ILLVM_FCHECK(!ec, "Can not remove file " + filepath + ": " + ec.message());
}

static void rmEmptyDir(const std::string &filepath) {
  const auto ec = llvm::sys::fs::remove_directories(filepath);
  ILLVM_FCHECK(!ec,
               "Can not remove directory " + filepath + ": " + ec.message());
}

static void rmDirDFS(const std::string &curDirPath) {
  namespace fs = llvm::sys::fs;
  namespace path = llvm::sys::path;

  std::error_code ec;

  for (fs::directory_iterator it(curDirPath, ec), end; it != end && !ec;
       it.increment(ec)) {
    const auto &entry = *it;
    const std::string entryPath = entry.path();
    const std::string entryName = path::filename(entryPath).str();

    if (fs::is_regular_file(entryPath)) {
      FileSystem::rmFile(entryPath);
    } else if (fs::is_directory(entryPath)) {
      const std::string newDirPath =
          FileSystem::linkPath(curDirPath, entryName);
      rmDirDFS(newDirPath);
    } else {
      ILLVM_FCHECK(false, "Unknown file type: " + entryPath);
    }
  }

  ILLVM_FCHECK(!ec, "Traverse dir " + curDirPath + " error: " + ec.message());

  rmEmptyDir(curDirPath);
}

void FileSystem::rmFile(const std::string &filepath) {
  if (!checkFileExists(filepath)) {
    return;
  }
  if (llvm::sys::fs::is_regular_file(filepath)) {
    rmSingleFile(filepath);
  } else if (llvm::sys::fs::is_directory(filepath)) {
    rmDirDFS(filepath);
  } else {
    ILLVM_FCHECK(false, "Unknown file type: " + filepath);
  }
}

static void mvSingleFile(const std::string &from,
                                const std::string &to) {
  const auto ec = llvm::sys::fs::rename(from, to);
  ILLVM_FCHECK(!ec, "mv " + from + " to " + to + " error: " + ec.message());
}

static void mvDirDFS(const std::string &baseFromDirPath,
                          const std::string &baseToDirPath,
                          const std::string &relDirPath = "") {
  namespace fs = llvm::sys::fs;
  namespace path = llvm::sys::path;

  const std::string fromDirPath =
      FileSystem::linkPath(baseFromDirPath, relDirPath);
  const std::string toDirPath = FileSystem::linkPath(baseToDirPath, relDirPath);

  ILLVM_FATAL_ON(FileSystem::mkdir(toDirPath), "");

  std::error_code ec;

  for (fs::directory_iterator it(fromDirPath, ec), end; it != end && !ec;
       it.increment(ec)) {
    const auto &entry = *it;
    const std::string fromEntryPath = entry.path();
    const std::string fromEntryName = path::filename(fromEntryPath).str();

    if (fs::is_regular_file(fromEntryPath)) {
      const std::string toEntryPath = FileSystem::linkPath(toDirPath, fromEntryName);
      mvSingleFile(fromEntryPath, toEntryPath);
    } else if (fs::is_directory(fromEntryPath)) {
      const std::string newRelDirPath = FileSystem::linkPath(relDirPath, fromEntryName);
      mvDirDFS(baseFromDirPath, baseToDirPath, newRelDirPath);
    } else {
      ILLVM_FCHECK(false, "Unknown file type: " + fromEntryPath);
    }
  }

  ILLVM_FCHECK(!ec, "Traverse dir " + fromDirPath + " error: " + ec.message());

  rmEmptyDir(fromDirPath);
}

void FileSystem::mvFile(const std::string &from, const std::string &to) {
  ILLVM_FCHECK(checkFileExists(from),
               "mv " + from + " to " + to + " failed: from does not exist")
  rmFile(to);
  if (llvm::sys::fs::is_regular_file(from)) {
    mvSingleFile(from, to);
  } else if (llvm::sys::fs::is_directory(from)) {
    mvDirDFS(from, to);
  } else {
    ILLVM_FCHECK(false, "Unknown file type: " + from);
  }
}

void FileSystem::cpFile(const std::string &from, const std::string &to) {
  rmSingleFile(to);
  const auto ec = llvm::sys::fs::copy_file(from, to);
  ILLVM_FCHECK(!ec, "cp " + from + " to " + to + " error: " + ec.message());
}

void FileSystem::saveStr(const std::string &filepath, const std::string &str) {
  std::ofstream ofs(filepath);
  ILLVM_FCHECK(ofs.is_open(), "Can not open " + filepath);
  ofs << str;
  ofs.close();
}

void FileSystem::saveVector(const std::string &filepath,
                            const std::vector<std::string> &vec) {
  std::ostringstream oss;
  for (const auto &elem : vec) {
    oss << elem << std::endl;
  }
  std::ofstream ofs(filepath);
  ILLVM_FCHECK(ofs.is_open(), "Can not open " + filepath);
  ofs << oss.str();
  ofs.close();
}

void FileSystem::saveSet(const std::string &filepath,
                         const std::unordered_set<std::string> &st,
                         const bool ordered) {
  std::ostringstream oss;
  if (ordered) {
    const std::set<std::string> orderedSet(st.begin(), st.end());
    for (const auto &elem : orderedSet) {
      oss << elem << std::endl;
    }
  } else {
    for (const auto &elem : st) {
      oss << elem << std::endl;
    }
  }
  std::ofstream ofs(filepath);
  ILLVM_FCHECK(ofs.is_open(), "Can not open " + filepath);
  ofs << oss.str();
  ofs.close();
}

} // namespace illvm

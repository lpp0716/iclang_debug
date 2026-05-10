#include "illvm/Support/Logger.h"

#include <fstream>

#include "illvm/Support/Time.h"

#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

namespace illvm {

void Logger::writeLog(const std::string &msg) const {
  if (logPath.empty()) {
    return;
  }
  // Append IClang log
  const std::string timeMsg = Time::currentDateTime() + " " + msg;
  std::ofstream logFile(logPath, std::ios::app);
  if (!logFile.is_open()) {
    llvm::report_fatal_error(llvm::StringRef(
        "[ILLVM Fatal] Write ILLVM log error: can not open " + logPath));
  }
  logFile << timeMsg << std::endl;
  logFile.close();
}

void Logger::initLogPath(const std::string &_logPath) { logPath = _logPath; }

void Logger::info(const std::string &loc, const std::string &msg) const {
  const std::string fmtMsg = "[ILLVM Info] " + loc + ": " + msg;
  llvm::errs() << fmtMsg << "\n";
  writeLog(fmtMsg);
}

void Logger::debug(const std::string &loc, const std::string &msg) const {
  const std::string fmtMsg = "[ILLVM Debug] " + loc + ": " + msg;
  llvm::errs() << fmtMsg << "\n";
  writeLog(fmtMsg);
}

void Logger::warning(const std::string &loc, const std::string &msg) const {
  const std::string fmtMsg = "[ILLVM Warning] " + loc + ": " + msg;
  llvm::errs() << fmtMsg << "\n";
  writeLog(fmtMsg);
}

void Logger::error(const std::string &loc, const std::string &msg) const {
  const std::string fmtMsg = "[ILLVM Error] " + loc + ": " + msg;
  llvm::errs() << fmtMsg << "\n";
  writeLog(fmtMsg);
}

__attribute__((noreturn)) void Logger::fatal(const std::string &loc,
                                             const std::string &msg) const {
  const std::string fmtMsg = "[ILLVM Fatal] " + loc + ": " + msg;
  writeLog(fmtMsg);
  llvm::report_fatal_error(llvm::StringRef(fmtMsg));
}

} // namespace illvm

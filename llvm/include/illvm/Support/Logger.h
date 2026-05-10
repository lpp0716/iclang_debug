//===--- Logger.h - ILLVM logger utils -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// ILLVM logger utils.
//
//===----------------------------------------------------------------------===/

#ifndef ILLVM_LOGGER_H
#define ILLVM_LOGGER_H

#include <string>

namespace illvm {

class Logger {
private:
  std::string logPath = "";

  Logger() {}

  void writeLog(const std::string &msg) const;

public:
  static Logger &getInstance() {
    static Logger instance;
    return instance;
  }

  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  void initLogPath(const std::string &_logPath);

  void info(const std::string &loc, const std::string &msg) const;

  void debug(const std::string &loc, const std::string &msg) const;

  void warning(const std::string &loc, const std::string &msg) const;

  void error(const std::string &loc, const std::string &msg) const;

  __attribute__((noreturn)) void fatal(const std::string &loc,
                                       const std::string &msg) const;
};

} // namespace illvm

#endif // ILLVM_LOGGER_H

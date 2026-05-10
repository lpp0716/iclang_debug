//===--- Interval.h - Interval utils  -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// Interval utils.
//
//===----------------------------------------------------------------------===/

#ifndef ILLVM_INTERVAL_H
#define ILLVM_INTERVAL_H

#include <algorithm>
#include <string>
#include <vector>

namespace illvm {

using Interval = std::pair<long long, long long>;

class IntervalManager {
public:
  std::vector<Interval> intervals;

  void addInterval(const Interval &interval);

  const std::vector<Interval> &getIntervals() const;

  static std::vector<Interval> merge(const std::vector<Interval> &_intervals);
};

struct SourceInterval {
  bool isValid;
  unsigned startLine, startColumn, endLine, endColumn;
  unsigned startOffset, endOffset;
  std::string filename;

  [[nodiscard]] std::string toString() const;

  Interval toInterval() const;
};

} // namespace illvm

#endif //ILLVM_INTERVAL_H

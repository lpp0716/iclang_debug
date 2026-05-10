#include "illvm/Support/Interval.h"

#include <iomanip>
#include <sstream>

namespace illvm {

void IntervalManager::addInterval(const Interval &interval) {
  intervals.emplace_back(interval);
}

const std::vector<Interval> &IntervalManager::getIntervals() const {
  return intervals;
}

std::vector<Interval>
IntervalManager::merge(const std::vector<Interval> &_intervals) {
  std::vector<Interval> baseIntervals(_intervals.begin(), _intervals.end());
  std::vector<Interval> mergedIntervals;

  std::sort(baseIntervals.begin(), baseIntervals.end());

  for (const auto &interval : baseIntervals) {
    const long long left = interval.first;
    const long long right = interval.second;

    if (mergedIntervals.empty()) {
      mergedIntervals.emplace_back(left, right);
    } else {
      const long long prevLeft = mergedIntervals.back().first;
      const long long prevRight = mergedIntervals.back().second;

      if (prevLeft <= left && left < prevRight) {
        mergedIntervals.back().second = std::max(prevRight, right);
      } else {
        mergedIntervals.emplace_back(left, right);
      }
    }
  }

  return mergedIntervals;
}

[[nodiscard]] std::string SourceInterval::toString() const {
  if (!isValid) {
    return "<invalid>";
  }
  std::ostringstream oss;
  oss << "<" << startLine << ":" << startColumn << "," << endLine << ":"
      << endColumn << ">" << "(" << startOffset << "," << endOffset << ")["
      << filename << "]";
  return oss.str();
}

Interval SourceInterval::toInterval() const { return {startOffset, endOffset}; }

} // namespace illvm

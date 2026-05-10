#include "illvm/Support/Time.h"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace illvm {

long long Time::currentTsMs() {
  const auto now = std::chrono::system_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  return duration.count();
}

std::string Time::currentDateTime() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
  const std::tm local_tm = *std::localtime(&now_time_t);
  std::ostringstream oss;
  oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

} // namespace illvm

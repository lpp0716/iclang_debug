#include "illvm/Support/Strings.h"

#include <algorithm>

namespace illvm {

std::string Strings::trimWhitespace(const std::string &str) {
  const auto start =
      std::find_if_not(str.begin(), str.end(),
                       [](const unsigned char ch) { return std::isspace(ch); });

  const auto end =
      std::find_if_not(str.rbegin(), str.rend(), [](const unsigned char ch) {
        return std::isspace(ch);
      }).base();

  if (start >= end) {
    return ""; // All spaces or empty string
  }

  return std::string(start, end);
}

bool Strings::hasPrefix(const std::string &str, const std::string &prefix) {
  if (str.size() < prefix.size()) {
    return false;
  }
  return str.compare(0, prefix.size(), prefix) == 0;
}

std::string
Strings::argVToArgs(const llvm::SmallVector<const char *, 128> &argv) {
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

} // namespace illvm
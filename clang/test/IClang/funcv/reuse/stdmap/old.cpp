#include <unordered_map>
#include <string>

int test() {
  std::unordered_map<std::string, int> m;
  m["hello"] = 1;
  m.emplace("world", 2);
  const std::string x = "hello";
  const auto it = m.find(x);
  if (it != m.end() && m.count("world") != 0) {
    return it->second - m.size() + 1;
  }
  return 1;
}

int main() {
  return test();
}
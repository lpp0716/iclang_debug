#include <unordered_set>

int test() {
  std::unordered_set<int> s;
  s.insert(1);
  s.emplace(2);
  int x = 1;
  auto it = s.find(x);
  if (it != s.end() && s.count(2) != 0) {
    return *it - s.size() + 1;
  }
  return 1;
}

int main() {
  return test();
}
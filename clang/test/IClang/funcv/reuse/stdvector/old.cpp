#include <vector>

int test() {
  std::vector<int> v;
  v.push_back(1);
  return v[0] - v.size();
}

int main() {
  return test();
}
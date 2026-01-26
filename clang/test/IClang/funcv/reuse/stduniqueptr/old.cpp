#include <memory>

int test() {
  const auto p = std::make_unique<int>(1);
  return *p - 1;
}

int main() {
  return test();
}
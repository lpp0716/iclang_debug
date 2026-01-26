#include <vector>
#include <stdexcept>

int test() {
  std::vector<int> vec = {1, 2, 3};

  try {
    vec.at(5);
  } catch (const std::out_of_range& e) {
    return 0;
  }

  return 1;
}

int main() {
  return test();
}
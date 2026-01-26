#include <fstream>

int test() {
  std::ofstream ofs("test.log");
  if (!ofs.is_open()) {
    return 1;
  }
  ofs << 1;
  ofs.close();

  std::ifstream ifs("test.log");
  if (!ifs.is_open()) {
    return 1;
  }
  int x;
  ifs >> x;
  ifs.close();

  return x - 1;
}

int main() {
  return test();
}
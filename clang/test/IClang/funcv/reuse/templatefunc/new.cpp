template<typename T> int foo3() { return 0; }

int test();

int main() {
  return test()-1+foo3<double>();
}
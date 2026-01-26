int test() {
  auto foo = []() { return 0; };
  return foo();
}

int main() {
  return test();
}
void foo3() {
  throw 3;
}

int test1();

int test2();

int test3() {
  try {
    foo3();
  } catch (...) {
    return 0;
  }
  return 1;
}

int main() {
  return test1() + test2() + test3();
}
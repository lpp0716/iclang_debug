void foo1() {
  throw 1;
}

void foo2() {
  throw 2;
}

void foo3() {
  throw 3;
}

int test1() {
  try {
    foo1();
  } catch (...) {
    return 0;
  }
  return 1;
}

int test2() {
  try {
    foo2();
  } catch (...) {
    return 0;
  }
  return 1;
}

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
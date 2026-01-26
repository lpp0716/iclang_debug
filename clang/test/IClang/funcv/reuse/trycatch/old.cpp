void foo() {
  throw 1;
}

int test() {
  try {
    foo();
  } catch (...) {
    return 0;
  }
  return 1;
}

int main() {
  return test();
}
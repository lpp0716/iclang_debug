int test() {
  class Inner {
  public:
    int foo() { return 0; }
  };
  Inner inner;
  return inner.foo();
}

int main() {
  return test();
}
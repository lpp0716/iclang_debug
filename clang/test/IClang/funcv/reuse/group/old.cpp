class A {
public:
  void test() {}
};

void foo() {}

int test() {
  foo();
  return 0;
}

int main() {
  A a;
  a.test();
  return test();
}
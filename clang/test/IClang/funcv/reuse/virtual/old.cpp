class A {
public:
  virtual int foo1() { return 100; }
  virtual int foo2() = 0;
};

class B : public A {
public:
  int foo1() override { return 1; }
  int foo2() override { return -1; }
};

int test() {
  A *a = new B();
  return a->foo1() + a->foo2();
}

int main() {
  return test();
}


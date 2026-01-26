class A {
public:
  void test() {}
};

int test();

int main() {
  A a;
  a.test();
  return test();
}
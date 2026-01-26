template <typename Derived>
class A {
public:
  int foo1() {
    return static_cast<Derived*>(this)->foo1_impl();
  }

  int foo2() {
    return static_cast<Derived*>(this)->foo2_impl();
  }
};

class B : public A<B> {
public:
  int foo1_impl() { return 1; }
  int foo2_impl() { return -1; }
};

int test() {
  A<B> a;
  return a.foo1() + a.foo2();
}

int main() {
  return test();
}
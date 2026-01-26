template<typename T> int foo3() { return 0; }

template<typename T> int foo2() { return foo3<T>() + foo3<double>() + 1; }

template<typename T> int foo() { return foo2<T>(); }

int test() {
  return foo<int>();
}

int main() {
  return test()-1+foo3<double>();
}
int x;
int y = 0;
const int vec[] = {1, 2, 3};

class A {
public:
  void test() {
    x = 1;
  }
};

inline int test1() {
  return 0;
}

int test2() {
  return test1() + x + y;
}

int main() {
  A a;
  a.test();
  const char *s = "123";
  const double d = 1.1;
  return test1() + test2() + vec[0] + (s[0] - '0') + (int)d;
}
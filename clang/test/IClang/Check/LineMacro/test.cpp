#include <cassert>

#define MY_LINE 1 + __LINE__

void test() {
  assert(false);
}

int test1() {
  return __LINE__;
}

int test2() {
  return MY_LINE;
}

int test3() {
  return __ASSERT_LINE;
}

int test4() {
  return __builtin_LINE();
}

int main() {
  return test1() + test2() + test3() + test4();
}
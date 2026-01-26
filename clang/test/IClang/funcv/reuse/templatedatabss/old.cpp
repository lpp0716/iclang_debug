template<typename T> T x;

int test() {
  return x<int> - x<int>;
}

int main() {
  return test();
}
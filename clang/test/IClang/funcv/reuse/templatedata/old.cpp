template<typename T> T x = 1;

int test() {
  return x<int>;
}

int main() {
  return test()-1;
}
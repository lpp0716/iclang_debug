int foo() { return 0; }

template<typename T>
class Temp {
public:
  static int x;
};

template<typename T>
int Temp<T>::x = foo();

int test() {
  Temp<int> temp;
  return temp.x;
}

int main() {
  Temp<int> temp;
  return test() + temp.x;
}
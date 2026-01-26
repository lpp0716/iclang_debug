int x = 1;

__attribute__((constructor))
void before_main() {}

int test();

int main() {
  return test() + x - 1;
}
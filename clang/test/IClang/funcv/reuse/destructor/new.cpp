__attribute__((destructor))
void after_main() {}

int test();

int main() {
  return test();
}
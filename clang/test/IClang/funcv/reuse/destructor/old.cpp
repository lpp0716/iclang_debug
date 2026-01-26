__attribute__((destructor))
void after_main() {}

int test() {
  return 0;
}

int main() {
    return test();
}
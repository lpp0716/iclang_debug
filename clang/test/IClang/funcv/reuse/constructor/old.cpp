int x = 1;

__attribute__((constructor))
void before_main() {
  x -= 1;
}

int test() {
  return 0;
}

int main() {
    return test() + x;
}
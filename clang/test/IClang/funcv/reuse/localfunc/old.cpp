namespace {
int local1() { return 1; }
}

static int local2() { return -1; }

int test() {
  return local1() + local2();
}

int main() {
  return test();
}
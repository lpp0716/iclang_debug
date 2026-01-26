int test() {
  const char16_t* str = u"0你好";
  return str[0]-'0';
}

int main() {
  return test();
}
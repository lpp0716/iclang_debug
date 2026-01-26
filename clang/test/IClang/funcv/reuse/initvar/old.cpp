int test() {
    return 1;
}

int x = test();
int y = test();

int main() {
    return x + y - 2;
}
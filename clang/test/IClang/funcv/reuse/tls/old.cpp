__thread int thread_variable = 1;

int test() {
    return thread_variable;
}

int main() {
    return test()-1;
}
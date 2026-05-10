// test.cpp
#include <cstdio>

// 1. 普通函数
void skipped_function() {
    printf("This should be skipped and not exist in binary.\n");
}

// 2. 成员函数
struct Tester {
    void skipped_method() {
        printf("This method body should be skipped.\n");
    }
};

// 3. 模板函数
template<typename T>
void skipped_template(T t) {
    printf("Template body should be skipped.\n");
}

int main() {
    skipped_function();
    Tester t;
    t.skipped_method();
    skipped_template(100);
    return 0;
}
#include "implicit_test.h"

void caller() {
    // 这里触发了 my_implicit_template_func<int> 的隐式实例化
    my_implicit_template_func(42);
}
// Enumeration
enum Color {
    RED = 1,
    GREEN = 2,
    BLUE = 3
};

// Union
union Data {
    int i;
    float f;
};

// Class
class MyClass {
public:
    int id;
    void doSomething() {} // 成员函数
};

// 嵌套结构测试：结构体里引用了上面的 enum 和 class
struct Container {
    Color color;
    MyClass obj;
};
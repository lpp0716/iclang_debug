// vtable_test.cpp
class MyBase {
public:
    // 虚析构函数和虚函数会触发 VTable (_ZTV6MyBase)
    // 以及 RTTI (_ZTI6MyBase, _ZTS6MyBase) 的生成
    virtual ~MyBase() {}
    virtual void doSomething() {}
};

void test_use() {
    MyBase b;
    b.doSomething();
}
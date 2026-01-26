#include <thread>
#include <future>

int thread_task() {
  return 1;
}

int test() {
  std::future<int> result1 = std::async(std::launch::async, thread_task);
  std::future<int> result2 = std::async(std::launch::async, thread_task);

  int total = result1.get() + result2.get();

  return total - 2;
}

int main() {
  return test();
}
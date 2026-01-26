#include <thread>
#include <future>

void thread_task(std::promise<int>&& prom) {
  prom.set_value(1);
}

int test() {
  std::promise<int> p1, p2;
  std::future<int> f1 = p1.get_future();
  std::future<int> f2 = p2.get_future();

  std::thread t1(thread_task, std::move(p1));
  std::thread t2(thread_task, std::move(p2));

  t1.join();
  t2.join();

  int total = f1.get() + f2.get();
  return total-2;
}

int main() {
  return test();
}
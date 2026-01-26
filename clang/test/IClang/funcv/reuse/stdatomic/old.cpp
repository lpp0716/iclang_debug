#include <thread>
#include <atomic>

std::atomic<int> globalCounter(0);

void incrementCounter() {
  globalCounter++;
}

int test() {
  std::thread t1(incrementCounter);
  std::thread t2(incrementCounter);

  t1.join();
  t2.join();

  return globalCounter - 2;
}

int main() {
  return test();
}
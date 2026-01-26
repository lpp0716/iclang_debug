#include <thread>
#include <mutex>

int globalCounter = 0;
std::mutex counterMutex;

void incrementCounter() {
  {
    std::lock_guard<std::mutex> lock(counterMutex);
    ++globalCounter;
  }
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
#include "../include/ConcurrentAlloc.h"
#include "../include/ObjectPool.h"

void Alloc1() {
  for (size_t i = 0; i < 5; i++) {
    void *ptr = ConcurrentAlloc(6);
    cout << "Thread 1: " << ptr << endl;
  }
}
void Alloc2() {
  for (size_t i = 0; i < 5; i++) {
    void *ptr = ConcurrentAlloc(7);
    cout << "Thread 2: " << ptr << endl;
  }
}
void TLSTest() {
  std::thread t1(Alloc1);
  std::thread t2(Alloc1);

  t1.join();
  t2.join();
}

int main() {
  // TestObjetPool();
  TLSTest();
  return 0;
}

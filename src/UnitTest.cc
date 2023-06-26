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

void TestConcurrentAlloc1() {
  void *p1 = ConcurrentAlloc(6);
  void *p2 = ConcurrentAlloc(8);
  void *p3 = ConcurrentAlloc(1);
  void *p4 = ConcurrentAlloc(7);
  void *p5 = ConcurrentAlloc(8);
}

void TestConcurrentAlloc2() {
  for (size_t i = 0; i < 1024; i++) {
    void *p1 = ConcurrentAlloc(6);
    cout << p1 << endl;
  }

  void *p2 = ConcurrentAlloc(8);
  cout << p2 << endl;
}

void TestAddressShift() {
  PAGE_ID id1 = 2000;
  PAGE_ID id2 = 2001;
  char *p1 = (char *)(id1 << PAGE_SHIFT);
  char *p2 = (char *)(id2 << PAGE_SHIFT);

  while (p1 < p2) {
    cout << (void *)p1 << ":" << ((PAGE_ID)p1 >> PAGE_SHIFT) << endl;
    p1 += 8;
  }
}

int main() {
  // TestObjetPool();
  // TLSTest();
  TestAddressShift();
  return 0;
}

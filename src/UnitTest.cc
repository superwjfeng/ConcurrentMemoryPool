#include "ConcurrentAlloc.h"
#include "ObjectPool.h"

struct TreeNode {
  int _val;
  TreeNode *_left;
  TreeNode *_right;
  TreeNode() : _val(0), _left(nullptr), _right(nullptr) {}
};

void TestObjectPool() {
  // 申请释放的轮次
  const size_t Rounds = 3;
  // 每轮申请释放多少次
  const size_t N = 100000;
  size_t begin1 = clock();
  std::vector<TreeNode *> v1;
  v1.reserve(N);
  for (size_t j = 0; j < Rounds; ++j) {
    for (int i = 0; i < N; ++i) {
      v1.push_back(new TreeNode);
    }
    for (int i = 0; i < N; ++i) {
      delete v1[i];
    }
    v1.clear();
  }
  size_t end1 = clock();
  ObjectPool<TreeNode> TNPool;
  size_t begin2 = clock();
  std::vector<TreeNode *> v2;
  v2.reserve(N);
  for (size_t j = 0; j < Rounds; ++j) {
    for (int i = 0; i < N; ++i) {
      v2.push_back(TNPool.New());
    }
    for (int i = 0; i < 100000; ++i) {
      TNPool.Delete(v2[i]);
    }
    v2.clear();
  }
  size_t end2 = clock();
  cout << "new cost time:" << end1 - begin1 << endl;
  cout << "object pool cost time:" << end2 - begin2 << endl;
}

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

  cout << p1 << endl;
  cout << p2 << endl;
  cout << p3 << endl;
  cout << p4 << endl;
  cout << p5 << endl;
}

// 测试在使用完一个Page后会如何
void TestConcurrentAlloc2() {
  for (size_t i = 0; i < 1024; i++) {
    void *p1 = ConcurrentAlloc(6);
    cout << p1 << endl;
  }

  void *p2 = ConcurrentAlloc(8);
  cout << p2 << endl;
}

void TestConcurrentFree1() {
  void *p1 = ConcurrentAlloc(6);
  void *p2 = ConcurrentAlloc(8);
  void *p3 = ConcurrentAlloc(1);
  void *p4 = ConcurrentAlloc(7);
  void *p5 = ConcurrentAlloc(8);
  void *p6 = ConcurrentAlloc(8);
  void *p7 = ConcurrentAlloc(8);

  cout << p1 << endl;
  cout << p2 << endl;
  cout << p3 << endl;
  cout << p4 << endl;
  cout << p5 << endl;

  ConcurrentFree(p1);
  ConcurrentFree(p2);
  ConcurrentFree(p3);
  ConcurrentFree(p4);
  ConcurrentFree(p5);
  ConcurrentFree(p6);
  ConcurrentFree(p7);
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

void MultiThreadAlloc1() {
  std::vector<void *> v;
  for (size_t i = 0; i < 5; i++) {
    void *ptr = ConcurrentAlloc(6);
    v.push_back(ptr);
  }

  for (auto e : v) {
    ConcurrentFree(e);
  }
}

void MultiThreadAlloc2() {
  std::vector<void *> v;
  for (size_t i = 0; i < 5; i++) {
    void *ptr = ConcurrentAlloc(6);
    v.push_back(ptr);
  }

  for (auto e : v) {
    ConcurrentFree(e);
  }
}

void TestMultiThread() {
  std::thread t1(MultiThreadAlloc1);
  std::thread t2(MultiThreadAlloc2);

  t1.join();
  t2.join();
}

void BigAlloc() {
  void *p1 = ConcurrentAlloc(257 * 1024);
  ConcurrentFree(p1);

  void *p2 = ConcurrentAlloc(129 * 8 * 1024);
  ConcurrentFree(p2);
}

int main() {
  //TestObjectPool();
  //TLSTest();
  //TestAddressShift();
  //TestConcurrentAlloc1();
  //TestConcurrentAlloc2();
  //TestConcurrentFree1();
  //TestMultiThread();
  BigAlloc();
  return 0;
}

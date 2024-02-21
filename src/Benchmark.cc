#include "Common.h"
#include "ConcurrentAlloc.h"

// ntimes: number of memory allocation and deallocation per round
// nworks: number of threads per round
// rounds
void BenchmarkMalloc(size_t ntimes, size_t nworks, size_t rounds) {
  std::vector<std::thread> vthread(nworks);
  std::atomic<size_t> malloc_costtime = 0;
  std::atomic<size_t> free_costtime = 0;
  for (size_t k = 0; k < nworks; ++k) {
    // Every thread runs a Lambda expression
    vthread[k] = std::thread([&, k]() {
      std::vector<void *> v;
      v.reserve(ntimes);
      for (size_t j = 0; j < rounds; ++j) {
        size_t begin1 = clock();
        for (size_t i = 0; i < ntimes; i++) {
          // v.push_back(malloc(16));
          v.push_back(malloc((16 + i) % 8191 + 1));
        }
        size_t end1 = clock();
        size_t begin2 = clock();
        for (size_t i = 0; i < ntimes; i++) {
          free(v[i]);
        }
        size_t end2 = clock();
        v.clear();
        malloc_costtime += (end1 - begin1);
        free_costtime += (end2 - begin2);
      }
    });
  }
  for (auto &t : vthread) {
    t.join();
  }

  printf("Malloc test setup: alloc/dealloc times %d, threads %d, round: %d\n",
         ntimes, nworks, rounds);
  printf(
      "%u threads run %u rounds concurrently, each thread ConcurrentAlloc %u "
      "times every round takes: %u ms\n",
      nworks, rounds, ntimes, (unsigned int)malloc_costtime);
  printf(
      "%u threads run %u rounds concurrently, each thread ConcurrentFree %u "
      "times every round takes: %u ms\n",
      nworks, rounds, ntimes, (unsigned int)free_costtime);

  unsigned int totalTime = (unsigned int)(malloc_costtime + free_costtime);
  printf(
      "%u threads run ConcurrentAlloc&Free %u times concurrently: takes "
      "totally %u ms\n",
      nworks, nworks * rounds * ntimes, totalTime);
  std::fstream file("dataMalloc.txt", std::ios::out | std::ios::app);
  if (file.is_open()) {
    file << nworks << ":" << totalTime << endl;
    file.close();
  } else {
    cout << "File open failed" << endl;
  }
}

// 单轮次申请释放次数 线程数 轮次
void BenchmarkConcurrentMalloc(size_t ntimes, size_t nworks, size_t rounds) {
  std::vector<std::thread> vthread(nworks);
  std::atomic<size_t> malloc_costtime = 0;
  std::atomic<size_t> free_costtime = 0;
  for (size_t k = 0; k < nworks; ++k) {
    vthread[k] = std::thread([&]() {
      std::vector<void *> v;
      v.reserve(ntimes);
      for (size_t j = 0; j < rounds; ++j) {
        size_t begin1 = clock();
        for (size_t i = 0; i < ntimes; i++) {
          // v.push_back(ConcurrentAlloc(16));
          v.push_back(ConcurrentAlloc((16 + i) % 8191 + 1));
        }
        size_t end1 = clock();
        size_t begin2 = clock();
        for (size_t i = 0; i < ntimes; i++) {
          ConcurrentFree(v[i]);
        }
        size_t end2 = clock();
        v.clear();
        malloc_costtime += (end1 - begin1);
        free_costtime += (end2 - begin2);
      }
    });
  }
  for (auto &t : vthread) {
    t.join();
  }
  printf(
      "ConcurentMalloc test setup: alloc/dealloc times %d, threads %d, round: "
      "%d\n",
      ntimes, nworks, rounds);
  printf(
      "%u threads run %u rounds concurrently, each thread ConcurrentAlloc %u "
      "times every round takes: %u ms\n",
      nworks, rounds, ntimes, (unsigned int)malloc_costtime);
  printf(
      "%u threads run %u rounds concurrently, each thread ConcurrentFree %u "
      "times every round takes: %u ms\n",
      nworks, rounds, ntimes, (unsigned int)free_costtime);

  unsigned int totalTime = (unsigned int)(malloc_costtime + free_costtime);
  printf(
      "%u threads run ConcurrentAlloc&Free %u times concurrently: takes "
      "totally %u ms\n",
      nworks, nworks * rounds * ntimes, totalTime);

  // std::fstream file("dataConcurrentMalloc.txt", std::ios::out |
  // std::ios::app);
  std::fstream file("dataConcurrentMallocRadixTree.txt",
                    std::ios::out | std::ios::app);
  if (file.is_open()) {
    file << nworks << ":" << totalTime << endl;
    file.close();
  } else {
    cout << "File open failed" << endl;
  }
}

int main() {
  size_t ntimes = 10000;

  for (size_t nworks = 1; nworks <= 50; nworks++) {
    cout << "=========================================================="
         << endl;
    BenchmarkConcurrentMalloc(ntimes, nworks, 10);
    cout << endl << endl;
    BenchmarkMalloc(ntimes, nworks, 10);
    cout << "=========================================================="
         << endl;
  }

  // BenchmarkConcurrentMalloc(ntimes, 4, 10);
  return 0;
}

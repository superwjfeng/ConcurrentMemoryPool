#pragma once

#include "Common.h"
#include "ThreadCache.h"

static void *ConcurrentAlloc(size_t size) {
  // 通过TLS每个线程无锁的获取自己专属的ThreadCache对象
  if (pTLS_thread_cache == nullptr) {
    pTLS_thread_cache = new ThreadCache;
  }

  // 获取线程号
  cout << std::this_thread::get_id() << ": " << pTLS_thread_cache << endl;

  return pTLS_thread_cache->Allocate(size);
}

static void ConcurrentFree(void *ptr, size_t size) {
  assert(pTLS_thread_cache);
  pTLS_thread_cache->Deallocate(ptr, size);
}

/**
 * @file ThreadCache.h
 * @author Weijian Feng (wj.feng@tum.de)
 * @brief Thread Cache uses TLS to avoid lock contention 
 * @version 0.1
 * @date 2024-02-21
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef THREADCACHE_H_
#define THREADCACHE_H_
#include "Common.h"

class ThreadCache {
 public:
  // 申请和释放空间
  void *Allocate(size_t size);
  void Deallocate(void *ptr, size_t size);

  // 从central cache获取对象
  void *FetchFromCentralCache(size_t index, size_t size);

  // 释放对象时，若链表过长则可以回收内存到中心缓存
  void ListTooLong(FreeList &list, size_t size);

 private:
  FreeList _freeLists[NFREELISTS];
};

// TLS Thread Local Storage
#ifdef _WIN64
static _declspec(thread) ThreadCache *pTLS_thread_cache = nullptr;
#elif _WIN32
static _declspec(thread) ThreadCache *pTLS_thread_cache = nullptr;
#else  // linux 32
static __thread ThreadCache *pTLS_thread_cache = nullptr;
#endif

#endif  // THREADCACHE_H_
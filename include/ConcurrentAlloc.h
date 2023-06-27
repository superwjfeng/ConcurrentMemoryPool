#pragma once
#include "ThreadCache.h"
#include "PageCache.h"
#include "ObjectPool.h"

// tcmalloc中直接把这个函数命名为tcmalloc
static void *ConcurrentAlloc(size_t size) {
  // 大于MAX_BYTES的内存不能走内存池
  if (size > MAX_BYTES) {
    size_t alignSize = SizeClass::RoundUp(size);
    //注意：RoundUp返回的是对齐到的字节数，而我们要对齐到的是页数，所以>>转换为页数
    size_t kpage = alignSize >> PAGE_SHIFT;
    PageCache::GetInstance()->getPageMtx().lock();
    Span *span = PageCache::GetInstance()->NewSpan(kpage);
    span->_objSize = size;
    PageCache::GetInstance()->getPageMtx().unlock();
    void *ptr = (void*)((span->_pageID) << PAGE_SHIFT);
    return ptr;
  }

  // 通过TLS每个线程无锁的获取自己专属的ThreadCache对象
  if (pTLS_thread_cache == nullptr) {
    static ObjectPool<ThreadCache> tcPool;
    pTLS_thread_cache = tcPool.New();
  }

  // Get thread ID
  // cout << std::this_thread::get_id() << ": " << pTLS_thread_cache << endl;

  return pTLS_thread_cache->Allocate(size);
}

// TODO: 还内存块的时候要知道还多大，以此来确定是哪一个桶
static void ConcurrentFree(void *ptr) {
  Span *span = PageCache::GetInstance()->MapObjectToSpan(ptr);
  size_t size = span->_objSize;
  if (size > MAX_BYTES) {
    PageCache::GetInstance()->getPageMtx().lock();
    PageCache::GetInstance()->ReleaseSpanToPageCache(span);
    PageCache::GetInstance()->getPageMtx().unlock();
  }
  else {
		assert(pTLS_thread_cache);
		pTLS_thread_cache->Deallocate(ptr, size);
  }
}

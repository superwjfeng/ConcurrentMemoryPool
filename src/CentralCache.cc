#include "../include/CentralCache.h"
#include "../include/PageCache.h"

CentralCache CentralCache::_sInst; //定义

Span *CentralCache::GetOneSpan(SpanList &list, size_t size) {
  // 查看当前的spanList中是否有还有未分配对象的span
  Span *it = list.Begin();
  while (it != list.End()) {
    //只要不为空就说明有空闲的span挂着
    if (it->_freeList != nullptr) {
      return it;
    } else {
      it = it->_next;
    }
  }
  // 先把Central Cache的桶锁解掉，这样若其他线程释放内存对象回来，不会阻塞住
  list._mtx.unlock();

  // 没有空闲的span，只能去找Page Cache要
  // size越小分配的page越少，size越大分配的page越大
  PageCache::GetInstance()->_pageMtx.lock();
  Span *span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
  span->_isUse = true;
  PageCache::GetInstance()->_pageMtx.unlock();
  // 切分获取到的span不需要加锁，因为其他线程访问不到这个span

  // 切割span，首先计算page的起始地址和大块内存的大小（字节数）
  char *start = (char *)(span->_pageID << PAGE_SHIFT);
  size_t bytes = span->_n << PAGE_SHIFT;

  // 把大块内存切成自由链表连接起来
  char *end = start + bytes;
  // 先切一块下来去做头，方便尾插
  span->_freeList = start;
  start += size;
  void *tail = span->_freeList;

  while (start < end) {
    NextObj(tail) = start;
    tail = NextObj(tail);
  }

  // 当切好span以后，需要把span挂到桶里面去的时候再加锁
  list._mtx.lock();
  list.PushFront(span); // 头插进list中

  return span;
}

// Thread Cache调用该接口，从central cache获取batchNum个size大小的对象
size_t CentralCache::FetchRangeObj(void *&start, void *&end, size_t batchNum,
                                   size_t size) {
  // 计算是哪一个桶里的
  size_t index = SizeClass::Index(size);

  _spanLists[index]._mtx.lock();

  // 获取一个非空的span
  Span *span = GetOneSpan(_spanLists[index], size);
  assert(span);
  assert(span->_freeList);

  // 从span中获取batchNum个对象
  // 若不够batchNum个，就有多少拿多少
  start = span->_freeList;
  end = start;
  //// 用for有越界问题，不好控制
  // for (size_t i = 0; i < batchNum - 1; i++) {
  //   end = NextObj(end);
  // }
  size_t i = 0;
  size_t actualNum = 1; //实际获取了多少个
  while (i < batchNum - 1 && NextObj(end) != nullptr) {
    end = NextObj(end);
    i++;
    actualNum++;
  }

  // 把span取走内存块之后的剩余内存块重新连接起来
  span->_freeList = NextObj(end);
  NextObj(end) = nullptr;

  _spanLists[index]._mtx.unlock();

  return actualNum;
}

void CentralCache::ReleaseListToSpans(void *start, size_t size) {
  size_t index = SizeClass::Index(size);

  _spanLists[index]._mtx.lock();

  while (start) {
    void *next = NextObj(start);
    Span *span = PageCache::GetInstance()->MapObjectToSpan(start);
    NextObj(start) = span->_freeList;
    span->_freeList = start;
    start = next;

    // 说明span切分出去的所有小块内存都回来了
    // 这个span就可以再回收给Page Cache，Page Cache可以再尝试去做前后页的合并
    if (span->_useCount == 0) {
      _spanLists[index].Erase(span);
      span->_freeList = nullptr;
      span->_next = nullptr;
      span->_prev = nullptr;

      // 释放span给Page Cache时，使用Page Cache的锁就可以了
      // 这时把桶锁解掉
      _spanLists[index]._mtx.unlock();

      PageCache::GetInstance()->_pageMtx.lock();
      PageCache::GetInstance()->ReleaseSpanToPageCache(span);
      PageCache::GetInstance()->_pageMtx.unlock();

      _spanLists[index]._mtx.lock();
    }
  }

  _spanLists[index]._mtx.unlock();
}

#include "../include/CentralCache.h"

CentralCache CentralCache::_sInst; //定义

Span *CentralCache::GetOneSpan(SpanList &list, size_t byte_size) {
  return nullptr;
}

// 从 central cache 获取一定数量的对象给 thread cache
size_t CentralCache::FetchRangeObj(void *&start, void *&end, size_t batchNum,
                                   size_t size) {
  // 计算是哪一个桶里的
  size_t index = SizeClass::Index(size);
  _spanLists[index]._mtx.lock();

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
  span->_freeList = NextObj(end);
  NextObj(end) = nullptr;

  _spanLists[index]._mtx.unlock();

  return actualNum;
}

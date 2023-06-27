#include "../include/ThreadCache.h"
#include "../include/CentralCache.h"

void *ThreadCache::FetchFromCentralCache(size_t index, size_t size) {
  // 慢开始反馈调整算法：大对象给多一点，小对象给少一点

  // 一开始给少一点，MaxSize慢慢加
  size_t batchNum =
      min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(size));

  if (_freeLists[index].MaxSize() == batchNum) {
    _freeLists[index].MaxSize() += 1;
  }

  // start和end用来表示获取到的第一个和最后一个内存块
  void *start = nullptr;
  void *end = nullptr;
  // actualNum是实际返回的内存块的数量，若有的话一次会多给几个，至少得给一个
  // start和end都是输出型参数，调用者可以直接使用
  size_t actualNum =
      CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, size);
  assert(actualNum > 0); //至少得给1个

  if (actualNum == 1) {
    assert(start == end);
    return start;
  } else {
    _freeLists[index].PushRange(NextObj(start), end, actualNum - 1);
    return start;
  }
}

void *ThreadCache::Allocate(size_t size) {
  assert(size <= MAX_BYTES);
  size_t align_size = SizeClass::RoundUp(size);
  size_t index = SizeClass::Index(size);

  if (!_freeLists[index].Empty()) {
    // pop from free list to fetch some prepared free block to use
    return _freeLists[index].Pop();
  } else {
    return FetchFromCentralCache(index, align_size);
  }
}

void ThreadCache::Deallocate(void *ptr, size_t size) {
  assert(ptr);
  assert(size <= MAX_BYTES);

  // 找出映射的自由链表桶，把对象插入进去
  size_t index = SizeClass::Index(size);
  _freeLists[index].Push(ptr);

  // 当链表长度大于一次批量申请的内存时就还一段list给Central Cache
  if (_freeLists[index].Size() >= _freeLists[index].MaxSize()) {
    ListTooLong(_freeLists[index], size);
  }
}
// 自由链表太长了，往回回收内存
void ThreadCache::ListTooLong(FreeList &list, size_t size) {
  void *start = nullptr;
  void *end = nullptr;
  // start是输出型参数，从list中拿下来后再给ReleaseListToSpans用
  list.PopRange(start, end, list.MaxSize());

  // 返回给Central Cache
  CentralCache::GetInstance()->ReleaseListToSpans(start, size);
}

#include "../include/ThreadCache.h"
#include "../include/CentralCache.h"

void *ThreadCache::FetchFromCentralCache(size_t index, size_t size) {
  // 慢开始反馈调整算法：大对象给多一点，小对象给少一点
  size_t batchNum =
      std::min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(size));

  if (_freeLists[index].MaxSize() == batchNum) {
    _freeLists[index].MaxSize() += 1;
  }

  void *start = nullptr;
  void *end = nullptr;
  size_t actualNum =
      CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, size);
  assert(actualNum > 1); //至少得给1个

  if (actualNum == 1) {
    assert(start == end);
    return start;
  } else {
    _freeLists[index].PushRange(NextObj(start), end);
    return start;
  }
  return nullptr;
}

void *ThreadCache::Allocate(size_t size) {
  assert(size <= MAX_BYTES);
  size_t align_size = SizeClass::RoundUp(size);
  size_t index = SizeClass::Index(size);

  if (!_freeLists[index].Empty()) {
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
}

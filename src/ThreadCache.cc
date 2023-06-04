#include "../include/ThreadCache.h"

void *ThreadCache::FetchFromCentralCache(size_t index, size_t size) {
  //...
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

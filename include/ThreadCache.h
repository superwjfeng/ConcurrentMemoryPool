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
static __thread ThreadCache *pTLS_thread_cache = nullptr;

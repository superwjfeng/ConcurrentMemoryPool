#include "Common.h"

class ThreadCache {
public:
  // 申请和释放空间
  void *Allocate(size_t size);
  void Deallocate(void *ptr, size_t size);

private:
  FreeList _freeList[];
};

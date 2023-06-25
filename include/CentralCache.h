#pragma once

#include "Common.h"

// 设计为饿汉的单例模式
class CentralCache {
public:
  static CentralCache *GetInstance() { return &_sInst; }

  // 获取一个非空的Span
  Span *GetOneSpan(SpanList &list, size_t byte_size);

  // 从中心缓存获取一定数量的对象给thread cache
  size_t FetchRangeObj(void *&start, void *&end, size_t batchNum, size_t size);

  // 将一定数量的对象释放到span
  void ReleaseListToSpans(void *start, size_t byte_size);

private:
  SpanList _spanLists[NFREELISTS];

private:
  CentralCache() {}                            // 私有构造
  CentralCache(const CentralCache &) = delete; // 禁止拷贝构造

  static CentralCache _sInst; // 声明，为了防止重复包头文件，定义到cc中
};

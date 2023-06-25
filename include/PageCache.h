#pragma once

#include "Common.h"

class PageCache {
public:
  static PageCache *GetInstance() { return &_sInst; }

  // 获取从对象到span的映射
  Span *MapObjectToSpan(void *obj);

  // 释放空闲span回到Page Cache，并合并相邻的span
  void ReleaseSpanToPageCache(Span *span);

  // 获取一个k页的span
  Span *NewSpan(size_t k);
  std::mutex _pageMtx;

private:
  SpanList _spanLists[NPAGES];
  std::unordered_map<PAGE_ID, Span *> _idSpanMap;

  // Singeleton
  PageCache(){};
  PageCache(const PageCache &) = delete;

  static PageCache _sInst; //声明
};

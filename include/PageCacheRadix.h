/**
 * @file PageCacheRadix.h
 * @author Weijian Feng (wj.feng@tum.de)
 * @brief Page Cache uses Radix Tree to improve performance
 * @version 0.2
 * @date 2024-02-21
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef PAGE_CACHE_RADIX_H_
#define PAGE_CACHE_RADIX_H_

#include "Common.h"
#include "ObjectPool.h"
#include "PageMap.h"

// Modify with Radix Tree
class PageCache {
 public:
  static PageCache *GetInstance() { return &_sInst; }

  // 获取从对象到span的映射
  Span *MapObjectToSpan(void *obj);

  // 释放空闲span回到Page Cache，并合并相邻的span
  void ReleaseSpanToPageCache(Span *span);

  // 获取一个k页的span
  Span *NewSpan(size_t k);

  std::mutex &getPageMtx() { return _pageMtx; }

 private:
  SpanList _spanLists[NPAGES];
  ObjectPool<Span> _spanPool;
  // 建立映射，找内存块属于那一个span
  // 之所以放在PageCache里，是之后PageCache往内存还数据的时候还要用到
  // std::unordered_map<PAGE_ID, Span *> _idSpanMap;
  TCMalloc_PageMap1<32 - PAGE_SHIFT> _idSpanMap;

  // Singeleton
  PageCache(){};
  PageCache(const PageCache &) = delete;

  static PageCache _sInst;  // 声明
  std::mutex _pageMtx;
};

#endif  // PAGE_CACHE_RADIX_H_
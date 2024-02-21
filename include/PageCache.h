/**
 * @file PageCache.h
 * @author Weijian Feng (wj.feng@tum.de)
 * @brief Page Cache singleton & gloabl lock
 * @version 0.1
 * @date 2024-02-21
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef PAGE_CACHE_H_
#define PAGE_CACHE_H_

#include "Common.h"
#include "ObjectPool.h"
#include "PageMap.h"

class PageCache {
 public:
  static PageCache *GetInstance() { return &_sInst; }

  // 获取从对象到span的映射
  Span *MapObjectToSpan(void *obj);

  /**
   * @brief 释放空闲span回到Page Cache，并合并相邻的span
   * 
   * @param span we want to release
   */
  void ReleaseSpanToPageCache(Span *span);

  /**
   * @brief 获取一个k页的span
   * 
   * @param k number of pages of the span
   * @return Span* 
   */
  Span *NewSpan(size_t k);

  /**
   * @brief Get the Page Mtx object
   * 
   * @return std::mutex& 
   */
  std::mutex &getPageMtx() { return _pageMtx; }

 private:
  SpanList _spanLists[NPAGES];
  ObjectPool<Span> _spanPool;
  // 建立映射，找内存块属于那一个span
  // 之所以放在PageCache里，是之后PageCache往内存还数据的时候还要用到
  // 注意：std库用的空间配置器最后还是去调了malloc
  std::unordered_map<PAGE_ID, Span *> _idSpanMap;

  // Singeleton
  PageCache(){};
  PageCache(const PageCache &) = delete;

  static PageCache _sInst;  // 声明
  std::mutex _pageMtx;
};

#endif  // PAGE_CACHE_H_
#include "PageCacheRadix.h"

// Modify with Radix Tree
PageCache PageCache::_sInst;  // 定义

// 获取一个k页的span
Span *PageCache::NewSpan(size_t k) {
  assert(k > 0);

  // 大块内存直接找堆要
  if (k > NPAGES - 1) {
    void *ptr = SystemAlloc(k);
    Span *span = _spanPool.New();
    span->_pageID = (PAGE_ID)ptr >> PAGE_SHIFT;
    span->_n = k;
    //_idSpanMap[span->_pageID] = span;
    _idSpanMap.set(span->_pageID, span);

    return span;
  }

  // 先检查第k个桶里面有没有span
  if (!_spanLists[k].Empty()) {
    Span *kSpan = _spanLists[k].PopFront();
    // 建立id和span的映射，方便Central Cache回收小块时，查找对应的span
    for (PAGE_ID i = 0; i < kSpan->_n; i++) {
      //_idSpanMap[kSpan->_pageID + i] = kSpan;
      _idSpanMap.set(kSpan->_pageID + i, kSpan);
    }
    return kSpan;
  }
  // 检查一下后面的桶里面有没有span，如果有可以把它切分
  for (size_t i = k + 1; i < NPAGES; i++) {
    if (!_spanLists[i].Empty()) {  // 有桶不为空
      Span *nSpan = _spanLists[i].PopFront();
      Span *kSpan = _spanPool.New();
      // 在nSpan的头部切一个k页下来（尾切也可以）
      kSpan->_pageID = nSpan->_pageID;
      kSpan->_n = k;

      nSpan->_pageID += k;
      nSpan->_n -= k;

      // nSpan被切走k页后，要重新挂到n号桶上
      _spanLists[nSpan->_n].PushFront(nSpan);

      // 存储nSpan的首尾页号与nSpan的映射，方便Page
      // Cache回收内存时进行的合并查找
      //_idSpanMap[nSpan->_pageID] = nSpan;

      //_idSpanMap[nSpan->_pageID + nSpan->_n - 1] = nSpan;
      _idSpanMap.set(nSpan->_pageID, nSpan);
      _idSpanMap.set(nSpan->_pageID + nSpan->_n - 1, nSpan);

      // 建立id和span的映射，方便Central Cache回收小块时，查找对应的span
      for (PAGE_ID i = 0; i < kSpan->_n; i++) {
        //_idSpanMap[kSpan->_pageID + i] = kSpan;
        _idSpanMap.set(kSpan->_pageID + i, kSpan);
      }

      // k页span返回
      return kSpan;
    }
  }
  // 走到这个位置就说明后面没有大页的span了
  // 这时就去找堆要一个128页的span

  Span *bigSpan = _spanPool.New();
  void *ptr = SystemAlloc(NPAGES - 1);
  bigSpan->_pageID = (PAGE_ID)ptr >> PAGE_SHIFT;
  bigSpan->_n = NPAGES - 1;

  _spanLists[bigSpan->_n].PushFront(bigSpan);

  // 递归重新分割需要的span
  return NewSpan(k);
}

Span *PageCache::MapObjectToSpan(void *obj) {
  PAGE_ID id = ((PAGE_ID)obj >> PAGE_SHIFT);
  // STL容器线程不安全，需要加锁，这里用RAII锁
  // std::unique_lock<std::mutex> lock(_pageMtx);
  // auto ret = _idSpanMap.find(id);
  // if (ret != _idSpanMap.end()) {
  //  return ret->second;
  //}
  // else {
  //  //一定能找到，找不到说明NewSpan里错了
  //  assert(false);
  //  return nullptr;
  //}
  auto ret = (Span *)_idSpanMap.get(id);
  assert(ret != nullptr);
  return ret;
}

void PageCache::ReleaseSpanToPageCache(Span *span) {
  // 大于128页的直接还给堆
  if (span->_n > NPAGES - 1) {
    void *ptr = (void *)(span->_pageID << PAGE_SHIFT);
    SystemFree(ptr);
    _spanPool.Delete(span);
    return;
  }

  // 对span前后的页，尝试进行合并，缓解外内存碎片问题
  while (1) {
    PAGE_ID prevID = span->_pageID - 1;
    // auto ret = _idSpanMap.find(prevID);
    //// 前面的页号没有，不合并了
    // if (ret == _idSpanMap.end()) {
    //   break;
    // }

    auto ret = (Span *)_idSpanMap.get(prevID);
    if (ret == nullptr) {
      break;
    }

    // 前面相邻页的span在使用，不合并了
    Span *prevSpan = ret;
    if (prevSpan->_isUse == true) {
      break;
    }

    // 合并出超过128页的span，没办法管理，不合并
    if (prevSpan->_n + span->_n > NPAGES - 1) {
      break;
    }

    span->_pageID = prevSpan->_pageID;
    span->_n += prevSpan->_n;

    _spanLists[prevSpan->_n].Erase(prevSpan);
    // 为什么要delete？因为Span结构体是我们自己创造出来管理span对象的
    _spanPool.Delete(prevSpan);
  }

  // 向后合并
  while (1) {
    PAGE_ID nextID = span->_pageID + span->_n;
    // auto ret = _idSpanMap.find(nextID);
    // if (ret == _idSpanMap.end()) {
    //   break;
    // }

    auto ret = (Span *)_idSpanMap.get(nextID);
    if (ret == nullptr) {
      break;
    }

    Span *nextSpan = ret;
    if (nextSpan->_isUse == true) {
      break;
    }

    if (nextSpan->_n + span->_n > NPAGES - 1) {
      break;
    }

    span->_n += nextSpan->_n;

    _spanLists[nextSpan->_n].Erase(nextSpan);
    _spanPool.Delete(nextSpan);
  }

  _spanLists[span->_n].PushFront(span);
  span->_isUse = false;  // 置为false，可以被别人继续合并
  // 将首尾放进映射
  //_idSpanMap[span->_pageID] = span;
  //_idSpanMap[span->_pageID + span->_n - 1] = span;
  _idSpanMap.set(span->_pageID, span);
  _idSpanMap.set(span->_pageID + span->_n - 1, span);
}

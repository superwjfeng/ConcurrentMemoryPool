#pragma once
#include <cassert>
#include <iostream>
#include <time.h>
#include <vector>

using std::cout;
using std::endl;

inline void *&NextObj(void *obj) { return *(void **)obj; }

// 管理切分好的小对象的自由列表
class FreeList {
public:
  void Push(void *obj) {
    assert(obj);
    // 头插
    //*(void **)obj = _freeList;
    NextObj(obj) = _freeList;
    _freeList = obj;
  };
  void *Pop() {
    //头删
    assert(_freeList);
    void *obj = _freeList;
    _freeList = NextObj(obj);
  };

private:
  void *_freeList;
};

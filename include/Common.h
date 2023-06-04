#pragma once
#include <cassert>
#include <iostream>
#include <thread>
#include <time.h>
#include <vector>

#include <pthread.h>
#include <unistd.h>

using std::cout;
using std::endl;

static const size_t MAX_BYTES = 256 * 1024; // Thread Cache最大可以取256 KB
static const size_t NFREELISTS = 208;

//找下一个对象，void**强转后解引用可以适用于32和64位
static void *&NextObj(void *obj) { return *(void **)obj; }

// 管理切分好的小对象的自由列表
class FreeList {
public:
  // 自由链表头插
  void Push(void *obj) {
    assert(obj);
    // 头插
    //*(void **)obj = _freeList;
    NextObj(obj) = _freeList;
    _freeList = obj;
  };
  // 自由链表头删
  void *Pop() {
    //头删
    assert(_freeList);
    void *obj = _freeList;
    _freeList = NextObj(obj);
    return obj;
  };

  bool Empty() { return _freeList == nullptr; }

private:
  void *_freeList = nullptr;
};

//管理对象大小的对齐映射规则
class SizeClass {
public:
  // 整体控制在最多10%左右的内碎片浪费
  // [1,128]                 8byte对齐         freelist[0,16)
  // [128+1,1024]            16byte对齐        freelist[16,72)
  // [1024+1,8*1024]         128byte对齐       freelist[72,128)
  // [8*1024+1,64*1024]      1024byte对齐      freelist[128,184)
  // [64*1024+1,256*1024]    8*1024byte对齐    freelist[184,208)
  static inline size_t _RoundUp(size_t size, size_t align_num) {
    size_t align_size;
    if (size % 8 != 0) {
      align_size = (size / align_num + 1) * align_num;
    } else {
      align_size = size;
    }
    return align_size;
  }

  //// 一种非常巧妙的算法
  // size_t _RoundUp(size_t size, size_t align_num) {
  //   return ((size + align_num - 1) & ~(align_num - 1));
  // }

  static inline size_t RoundUp(size_t size) {
    if (size <= 128) {
      return _RoundUp(size, 8);
    } else if (size <= 1024) {
      return _RoundUp(size, 16);
    } else if (size <= 8 * 1024) {
      return _RoundUp(size, 128);
    } else if (size <= 64 * 1024) {
      return _RoundUp(size, 1024);
    } else if (size <= 256 * 1024) {
      return _RoundUp(size, 8 * 1024);
    } else {
      assert(false);
      return -1;
    }
  }

  //计算映射的是哪一个自由链表桶
  // static inline size_t _Index(size_t size, size_t align_num) {
  //  if (size % align_num == 0) {
  //    return size / align_num - 1;
  //  } else {
  //    return size / align_num;
  //  }
  //}
  // 一种非常巧妙的算法
  static inline size_t _Index(size_t size, size_t align_shift) {
    return ((size + (1 << align_shift) - 1) >> align_shift) - 1;
  }
  static inline size_t Index(size_t size) {
    assert(size <= MAX_BYTES);
    //每个区间有多少条链
    static int group_array[4] = {16, 56, 56, 56};
    if (size <= 128) {
      return _Index(size, 3);
    } else if (size <= 1024) {
      return _Index(size - 128, 4) + group_array[0];
    } else if (size <= 8 * 1024) {
      return _Index(size - 1024, 7) + group_array[1] + group_array[0];
    } else if (size <= 64 * 1024) {
      return _Index(size - 8 * 1024, 10) + group_array[2] + group_array[1] +
             group_array[0];
    } else if (size <= 256 * 1024) {
      return _Index(size - 64 * 1024, 13) + group_array[3] + group_array[2] +
             group_array[1] + group_array[0];
    } else
      assert(false);
    return -1;
  }
};

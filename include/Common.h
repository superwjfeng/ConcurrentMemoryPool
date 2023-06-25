#pragma once
#include <algorithm>
#include <cassert>
#include <iostream>
#include <thread>
#include <time.h>
#include <unordered_map>
#include <vector>

#include <mutex>
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>

using std::cout;
using std::endl;

// different page ID for different OS
#ifdef _WIN64
typedef unsigned long long PAGE_ID;
#elif _WIN32
typedef size_t PAGE_ID;
#elif __LP64__
typedef unsigned long long PAGE_ID;
#else // linux 32
typedef size_t PAGE_ID;
#endif

inline static void *SystemAlloc(size_t kpage) {
#ifdef _WIN32
  void *ptr =
      VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
  // linux下brk mmap等
  size_t size_byte = kpage << 13;
  void *ptr = nullptr;
  ptr = sbrk(0);
  if (sbrk(size_byte) == (void *)-1) {
    ptr = mmap(nullptr, size_byte, PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == (void *)-1) {
      ptr = nullptr;
    }
  }

#endif

  if (ptr == nullptr)
    throw std::bad_alloc();

  return ptr;
}

static const size_t MAX_BYTES = 256 * 1024; // Thread Cache最大可以取256 KB
static const size_t NFREELISTS = 208;
static const size_t NPAGES = 129;
static const size_t PAGE_SHIFT = 13;

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
    _size++;
  };

  //支持范围内push多个对象
  void PushRange(void *start, void *end, size_t n) {
    NextObj(end) = _freeList;
    _freeList = start;
    _size += n;
  }

  void PopRange(void *&start, void *&end, size_t n) {
    assert(n >= _size);
    start = _freeList;
    end = start;

    for (size_t i = 0; i < n - 1; i++) {
      end = NextObj(end);
    }

    _freeList = NextObj(end);
    NextObj(end) = nullptr;
    _size -= n;
  }

  // 自由链表头删
  void *Pop() {
    //头删
    assert(_freeList);
    void *obj = _freeList;
    _freeList = NextObj(obj);
    --_size;
    return obj;
  };

  bool Empty() { return _freeList == nullptr; }

  size_t &MaxSize() { return _maxSize; }

  size_t Size() { return _size; }

private:
  void *_freeList = nullptr;
  size_t _maxSize = 1; //用于慢启动调整算法
  size_t _size;
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

  //一次thread cache 从 central cache 获取多少个对象
  static size_t NumMoveSize(size_t size) {
    assert(size > 0);
    //[2, 512]，一次批量移动多少个对象的（慢启动）上限值
    // 小对象一次批量上限高
    int num = MAX_BYTES / size;
    if (num < 2)
      num = 2;
    if (num > 512)
      num = 512;
    return num;
  }

  // 一次NewSpan要给多少page
  static size_t NumMovePage(size_t size) {
    size_t num = NumMoveSize(size);
    // num*size 是总的字节数，PAGE_SHIFT是字节到页的转换
    size_t npage = num * size;
    npage >>= PAGE_SHIFT;
    // 至少给一页
    if (npage == 0) {
      npage = 1;
    }

    return npage;
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
      // align_shift == 4 means align to 2^4 Byte
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

// 管理多个连续页的大块内存结构
struct Span {
  PAGE_ID _pageID = 0; // 大块内存起始页的页号
  size_t _n = 0;       // 页数

  Span *_next = nullptr; // 双向链表的机构
  Span *_prev = nullptr;

  size_t _useCount = 0; // 切成的小块内存，被分配给 thread cache 的计数
  void *_freeList = nullptr; // 切好的小块内存的自由链表

  bool _isUse = false; // 是否在被使用
};

// 带头双向循环链表
class SpanList {
public:
  // std::mutex GetMutex() {
  //   return this->_mtx;
  // }
public:
  // 必须要用构造函数初始化
  SpanList() {
    _head = new Span;
    _head->_next = _head;
    _head->_prev = _head;
  }
  // 遍历，不封装迭代器，直接用节点指针
  Span *Begin() { return _head->_next; }

  Span *End() { return _head; }

  bool Empty() { return _head->_next == _head; }

  void PushFront(Span *span) { Insert(Begin(), span); }

  Span *PopFront() {
    Span *front = _head->_next;
    Erase(front); //记住Erase中并没有delete pos
    return front;
  }

  // pos前插入
  void Insert(Span *pos, Span *newSpan) {
    assert(pos);
    assert(newSpan);
    Span *prev = pos->_prev;
    prev->_next = newSpan;
    newSpan->_prev = prev;
    newSpan->_next = pos;
    pos->_prev = newSpan;
  }

  void Erase(Span *pos) {
    assert(pos);
    assert(pos != _head);
    Span *prev = pos->_prev;
    Span *next = pos->_next;
    prev->_next = next;
    next->_prev = prev;
    //不要delete pos，因为之后要还给Page Cache
  }

private:
  Span *_head = nullptr; // 哨兵位

public:
  std::mutex _mtx; // 桶锁
};

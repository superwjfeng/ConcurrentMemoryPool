#pragma once
#include "Common.h"
// 定长对象的内存池，非类型模版
// template<size_t N>
// class ObjectPool {};

// 为了方便后面内存池复用，定义成模版
template <class T> class ObjectPool {
public:
  // 申请一个对象T
  T *New() {
    T *obj = nullptr;
    //优先重复利用还回来的freeList内存块
    if (_freeList) {
      void *next = *(static_cast<void **>(_freeList));
      obj = static_cast<T *>(_freeList);
      _freeList = next;
    } else {
      //剩余内存不够一个对象大小时，就放弃它，然后重新申请大块内存
      if (_remainBytes < sizeof(T)) {
        _remainBytes = 128 * 1024;
        //_memory = static_cast<char *>(malloc(_remainBytes));
        _memory = static_cast<char *>(SystemAlloc(_remainBytes >> 13));
        if (_memory == nullptr)
          throw std::bad_alloc();
      }

      // obj = static_cast<T *>(_memory);
      obj = (T *)_memory;
      //  申请的对象至少得放一个指针，比如64位下申请一个int对象就不行
      size_t objSize = sizeof(T) < sizeof(void *) ? sizeof(void *) : sizeof(T);
      _memory += objSize;
      _remainBytes -= objSize;
    }

    //定位new，显式调用T的构造函数初始化
    new(obj)T;

    return obj;
  }

  //还对象T
  void Delete(T *obj) {
    // 显式调用T的析构函数清理
    obj->~T();
    // freeList头插，无论是空链表还是非空都适用
    // 无论是32位还是64位都能用，因为解引用得到的是一个指针

    // 不可以在 TreeNode* 和 void** 这两种没有继承关系的类型之间转换
    //*(static_cast<void **>(obj)) = _freeList;
    *((void **)obj) = _freeList;
    _freeList = obj;
  }

private:
  //不定义成void*了，不然之后用都要强转
  char *_memory = nullptr;   // 指向大块内存的指针
  size_t _remainBytes = 0;   // 大块内存中还剩余的空间
  void *_freeList = nullptr; // 用于管理被切下来的内存块
};

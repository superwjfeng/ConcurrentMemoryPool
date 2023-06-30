https://blog.csdn.net/ETalien_/article/details/88832703

https://jiajunhuang.com/articles/2020_10_10-tcmalloc.md.html

## *定长内存池*

### 为什么需要内存池

内存池是一种管理计算机内存的技术，它通过预先分配一块连续的内存块，并在需要时动态地将其分配给程序使用。以下是需要使用内存池的几个主要原因：

1. 提高内存分配效率：传统的内存分配操作（如malloc和free）需要频繁地向操作系统申请和释放内存，**系统调用涉及到较高的系统调用开销**。内存池通过事先分配好一块内存区域，避免了频繁的系统调用，提高了内存分配和释放的效率
2. 减少内存碎片：在使用传统的内存分配方式时，频繁的内存分配和释放可能导致内存碎片的产生。内存碎片是指内存空间中被分割成小块而无法被充分利用的情况，它会降低内存的利用率并增加内存分配的复杂度。内存池可以减少内存碎片的产生，因为它们分配的内存块大小是固定的，不会产生碎片化的问题
3. 控制内存分配的策略：使用内存池可以更好地控制内存分配的策略，例如可以选择预分配一定数量的内存块，避免内存不足的情况发生，或者可以根据需求动态地扩展内存池的大小。这种控制可以提高程序的性能和稳定性
4. 适用于特定的应用场景：某些应用程序或算法对内存的需求是连续且频繁的，传统的内存分配方式可能无法满足其性能要求。内存池可以在这些特定场景下提供更高效的内存管理，比如STL库的空间配置器就是内存池

总的来说，内存池的主要目的是提高内存分配和释放的效率，减少内存碎片，并在特定的应用场景下提供更好的性能和稳定性

### intro

定长对象的内存池，每次申请或者归还一个固定大小的内存对象T，定长内存池可以满足固定大小的内存申请释放需求，并且定长内存池在高并发内存池中可以被复用

定长内存池的特点是

* 性能达到极致
* 不考虑内存碎片等问题

### 实现

<img src="C:\Users\Weijian Feng\iCloudDrive\Desktop\Notes\CS\内存池\doc\img\定长内存池.drawio.png">

用自由链表 `void *_freeList` 来管理切好的小块内存，内存块头上存下一个内存块的地址，因此一个内存块至少要存4字节（32位）或8字节（64位）

```c++
// 获取内存对象中存储的头4 or 8字节值，即链接的下一个对象的地址
inline void*& NextObj(void* obj) { 
    return *((void**)obj);
}
```

`void **` 在32位下是4 Byte，64位下是8 Byte。先将 `void *` 强转为 `void **` 对它再解引用一次的时候就可以得到 `void *` 的内容，这和对 `void *` 解引用得到 `void` 的效果是一样的。如果不这么做，就要通过32位用 `int *` 解引用和 64位用 `long *` 解引用来分别了，这很不灵活

可以用这种方式来判断当前系统是32位还是64位，然后强转

```cpp
if (sizeof(int *) == 32) //32位
    *((int *)obj) = nullptr; 
else //64位
    *((long long*)obj) = nullptr; 
```

若剩余的空间不够分配一块内存了该怎么办？引入 `_remainByte` 管理 

## *整体设计框架*

### tcmalloc 介绍

tcmalloc（Thread-Caching Malloc）是Google开发的一种用于多线程应用程序的内存分配器，在许多Google项目中得到广泛应用。tcmalloc旨在提供高效的内存分配和释放，**以减少多线程应用程序中的锁竞争和内存碎片化**

tcmalloc的设计目标是在**多线程环境下**最大限度地减少内存分配和释放的开销。它采用了许多优化策略来提高性能和可伸缩性

其中一个关键特性是线程本地缓存（Thread-Caching），它为每个线程维护了一个本地内存缓存，用于快速分配和释放内存。通过避免对全局数据结构的频繁访问，减少了锁竞争的情况，从而提高了性能

另一个重要的特性是分离的内存池（Central Cache），它用于处理大于某个阈值的内存分配请求。这些请求在被满足之前不会返回到操作系统，而是在内存池中进行高效的重用。这有助于减少对操作系统的系统调用次数，提高了性能

此外，tcmalloc还使用了一些其他的优化技术，如高效的内存块分配策略、精细的大小分类等，以提高内存分配的效率和内存利用率。

总的来说，tcmalloc是一种针对多线程应用程序的高性能内存分配器，通过利用线程本地缓存、分离的内存池和其他优化策略，提供了快速的内存分配和释放，并减少了锁竞争和内存碎片化的问题

### 三层设计

<img src="C:\Users\Weijian Feng\iCloudDrive\Desktop\Notes\CS\内存池\doc\img\内存池结构.drawio.png" width="50%">

* Thread cache 解决锁竞争的问题：线程缓存是每个线程独有的，用于小于256KB的内存的分配，线程从这里申请内存不需要加锁，每个线程独享一个cache，这也就是这个并发线程池高效的地方

* Central cache 居中调度

  * central cache 是所有线程所共享的，thread cache按需从central cache中获取对象，central cache也会在合适的时机回收thread cache中的对象。central cache有负载均衡的作用，可以避免一个线程抢占过多内存

  * 因为central cache是共享的，所有存在竞争。但是通过哈希桶的设计，这里竞争不会很激烈。其次只有本身只有thread cache的

    没有内存对象时才会找central cache，所以更降低了竞争烈度

* Page cache 以页为单位管理大内存，用brk或mmap直接找OS的堆要内存

### 主要API结构大纲

<img src="C:\Users\Weijian Feng\iCloudDrive\Desktop\Notes\CS\内存池\doc\img\内存池结构API.drawio.png" width="80%">

## *Thread Cache*

### Thread Cache 的结构设计

<img src="C:\Users\Weijian Feng\iCloudDrive\Desktop\Notes\CS\内存池\doc\img\ThreadCache.drawio.png" width="60%">

```c++
class ThreadCache {
public:
  // 申请和释放空间
  void *Allocate(size_t size);
  void Deallocate(void *ptr, size_t size);
  // 从central cache获取对象
  void *FetchFromCentralCache(size_t index, size_t size);

private:
  FreeList _freeLists[NFREELISTS];
};
```

### 哈希桶映射与内存块对齐规则

Thread Cache是哈希桶结构，每个桶是一个按桶的位置映射大小的内存块对象的自由链表，相当于是**直接定址法**。但问题是要如何设计设计映射，或者说如何设计内存分配方式。最Naïve的想法是精确到每一个Byte都分配，但这样挂的自由链表将会非常多，即需要 `256 * 1024 = 262144` 个哈希桶来放自由链表

所以为了平衡效率，需要做出一些空间上的浪费（内碎片）。设计成以8 Byte为一个间隔作为哈希桶。若要1 Byte，给8 Byte；要2 Byte，也给8 Byte；要8 Byte，还给8 Byte。也就是说在这种情况下，内存全部对齐到8 Byte

为了存下64位指针，毋庸置疑最少值肯定是8字节。若按8字节递增对齐，那么到256KB的时候，总共需要 `256 * 1024 / 8 = 32768` 个哈希桶来放自由链表

这感觉还是太多了，在tcmalloc的实现中实际采用的是下面这种，整体控制在最多10%左右的内碎片浪费。按下面的对齐方法，一共是有208个哈希桶来放自由链表

```
// [1,128]                 8byte对齐         freelist[0,16)
// [128+1,1024]            16byte对齐        freelist[16,72)
// [1024+1,8*1024]         128byte对齐       freelist[72,128)
// [8*1024+1,64*1024]      1024byte对齐      freelist[128,184)
// [64*1024+1,256*1024]    8*1024byte对齐    freelist[184,208)
```

使用一个专门的类 `SizeClass` 来进行管理

### 管理自由链表

<img src="C:\Users\Weijian Feng\iCloudDrive\Desktop\Notes\CS\内存池\doc\img\FreeList头插.drawio.png">

```c++
//获取内存对象中存储的头4或头8字节，即连接的下一个对象的地址
//void**强转后解引用可以适用于32和64位
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

  //支持范围内push多个对象
  void PushRange(void *start, void *end) {
    NextObj(end) = _freeList;
    _freeList = start;
  }

  // 自由链表头删
  void *Pop() {
    //头删
    assert(_freeList);
    void *obj = _freeList;
    _freeList = NextObj(obj);
    return obj;
  };

  bool Empty() { return _freeList == nullptr; }

  size_t &MaxSize() { return _maxSize; }

private:
  void *_freeList = nullptr;
  size_t _maxSize = 1;
};
```

### TLS无锁访问

Thread-Local Storage 是一种线程级别的存储机制，它允许每个线程在共享的内存空间中拥有自己独立的变量副本。每个线程都可以访问自己的TLS变量副本，而不会干扰其他线程的副本

TLS的主要目的是提供一种线程隔离的机制，使得每个线程可以独立地使用一组变量，**而不需要使用全局变量或上锁**。这对于多线程应用程序非常有用，因为它可以避免并发访问共享变量所带来的竞争条件和同步开销

每一个Thread Cache都是TLS，可以不受影响的并发申请资源。我们把TLS的申请封装到ConcurrentAlloc.h中。和封装的malloc函数或者tcmalloc函数一样，申请内存的时候直接用 `static void *ConcurrentAlloc(size_t size);`

Thread Local Storage（线程局部存储）TLS - 一束灵光的文章 - 知乎 https://zhuanlan.zhihu.com/p/142418922

根据上文，TLS在Linux中有两种使用方式，API方式和语言级别的方式，在我们的实现中采用语言级别的实现，即用 `__thread` 来声明需要用TLS管理的资源

```c++
// 在tcmalloc中这个函数被命名为tcmalloc
static void *ConcurrentAlloc(size_t size) {
  // 通过TLS每个线程无锁的获取自己专属的ThreadCache对象
  if (pTLS_thread_cache == nullptr) {
    pTLS_thread_cache = new ThreadCache;
  }

  // 获取线程号
  cout << std::this_thread::get_id() << ": " << pTLS_thread_cache << endl;

  return pTLS_thread_cache->Allocate(size);
}

static void ConcurrentFree(void *ptr, size_t size) {
  assert(pTLS_thread_cache);
  pTLS_thread_cache->Deallocate(ptr, size);
}
```

注意：在ThreadCache.h中为了避免头文件多次引入引发链接错误，所以将pTLS_thread_cache定义为static。这也意味着这意味着每个线程都会有自己独立的`pTLS_thread_cache`变量副本。每个线程在首次访问`pTLS_thread_cache`时，会进行初始化，并且每个线程的初始化操作都是独立的。因此，每个线程的`pTLS_thread_cache`变量在初始化之后都不会再为`nullptr`，并且每个线程都有自己独立的`ThreadCache`对象。并独立地使用它进行内存分配和释放操作

### 内存回收

Tcmalloc考虑了自由链表长度和总占用内存两个方面，我们这里简化一下，只考虑链表长度

## *Central Cache*

### Central Cache的结构设计

<img src="C:\Users\Weijian Feng\iCloudDrive\Desktop\Notes\CS\内存池\doc\img\CentralCache.drawio.png"  width="60%">

Central Cache和Thread Cache相似都采用了哈希桶的结构，并且它的哈希桶映射与内存块对齐规则也与Thread Cache一样。不同的是哈希桶挂的不是内存块的自由链表，而是SpanList

Span管理以页为单位的大块内存，每个span又会被切成对应的小块挂在span上，然后供Thread Cache取用或者回收并进行负载均衡。若所有Span都没了，那就再去找Page Cache要。Span及其管理结构SpanList既要给Central Cache用，也要给之后的Page Cache用，所以定义到Common.h中

首先要条件编译为不同的OS设置不同的page ID，若是在Win系统下，注意要先写宏 `_WIN64` 再写宏 `_WIN32` 的顺序<https://blog.csdn.net/chunfangzhang/article/details/87895833>

不同span里到底挂了多少个小的内存块是不知道的，因为时刻可能有新的内存块被Thread Cache还回来

**span设计为一个带头双向循环链表**，方便当span的 `_useCount==0` 时，Central Cache把Span还给Page Cache的时候方便找到对应的Span，然后重新做被删除Span前后的Span的连接

Central Cache是管理多个Thread Cache的，因此它里面是有线程竞争的

### 申请内存

Central Cache只有一个，所以把Central Cache设计成饿汉单例（注意为了防止头文件重复包含，将_sInst定义到cc中）

桶锁：每个哈希桶有一个锁。所以只有在找的是同一个锁的时候才会有竞争，否则会去找不同的桶







<img src="C:\Users\Weijian Feng\iCloudDrive\Desktop\Notes\CS\内存池\doc\img\FetchRangeObj.drawio.png" width="60%">



一个Span每次给Thread Cache多少个切好的内存块合适呢？给固定数量是不合适的，一是因为不同的Thread要的频率不同，二是因为大的内存块给的数量跟小的一样可能会造成严重的浪费。采用**慢开始反馈调节算法**：最开始不会一次向central cache 要太多，要太多了可能会用不完造成浪费。但是如果不断地往central cache要，每要一次maxSize++（maxSize是FreeList的属性），那么每次要的batchNum会越来越多，直到上限。而且内存块越小，上限就越高。

慢开始反馈调节算法写在 `ThreadCache::FetchFromCentralCache` 里，规定不同大小内存块上限的逻辑写在Common.h里，如下

```c++
//一次thread cache 从 central cache 获取多少个对象
static size_t NumMoveSize(size_t size) {
    assert(size > 0);
    //[2, 512]，一次批量移动多少个对象的（慢启动）上限值
    // 小对象一次批量上限高
    int num = MAX_BYTES / size;
    if (num < 2)
    	num = 2;
    // 测试得出的512
    if (num > 512)
    	num = 512;
    return num;
}
```





以8字节为例，要去256*1024/8=32768个，此时又太多了，所以定了个上限500。若取一个256KB的，至少要取2个

### GetOneSpan：从Central Cache向Page Cache要一块span并切割内存

每次要多少页比较好，也设计成自适应的方法。size越小分配的page越少，size越大分配的page越大。`num*size` 是总的字节数，PAGE_SHIFT是字节到页的转换。若定义一页为8KB，则 `PAGE_SHIFT=13`；若定义一页为4KB，则 `PAGE_SHIFT=12`

```c++
static size_t NumMovePage(size_t size)
{
    size_t num = NumMoveSize(size);
    size_t nBytge = num*size;
    size_t nPage = nByte >> PAGE_SHIFT;
    if (nPage == 0)
        nPage = 1;
    return nPage;
}
```

上面的过程中是通过页号，计算页的起始地址，即 `PageNum << PAGE_SHIFT`。假设页号为100，假设一页为8K

```c++
100 * 8K
100 << PAGE_SHIFT
```

<img src="C:\Users\Weijian Feng\iCloudDrive\Desktop\Notes\CS\内存池\doc\img\span切分为小内存.drawio.png" width="70%">

把大块span切成小块内存后以FreeList的方式连接起来，注意第一次连接是把span的 `_freeList` 跟内存块连接起来。尾插效果会比较好，头插的话SpanList中的地址会倒过来

### 内存回收

当申请了一个span，span上有多个切好的内存块，但是没人知道中间用户到底是怎么用了这些小内存块，所以问题在于还回来的长列表并不能确定是属于哪一个span的

小块内存肯定是由某个页切出来的，所以tcmalloc的解决方案是通过 `_pageID` 来定位属于哪一个span

来一个内存块遍历一遍span暴力确定属于哪个span是一个***O(N^2)***的算法，因此不采用这种方法，而是用 `unordered_map<PAGE_ID, Span*> _idSpanMap` 建立映射的***O(1)***算法，因为通过地址算PAGE_ID是很容易的，那么映射后直接可以找到对应的span

这个映射在向系统获取128页的NewSpan，或者是每次切割大小Span的时候建立





若Central Cache中的span useCount都等于0，说明切分给Thread Cache的小块内存都还回来了，那么Central Cache把这个span还给 Page Cache。Page Cache会通过页号，查看前后的相邻页是否空闲，是的话就合并，合并出更大的页解决内存碎片问题

先把Central Cache的桶锁解掉，这样若其他线程释放内存对象回来，不会阻塞住



## *Page Cache*

### Page Cache的结构设计

<img src="C:\Users\Weijian Feng\iCloudDrive\Desktop\Notes\CS\内存池\doc\img\PageCache.drawio.png" width="60%">

Page Cache也一样设计成单例模式

Page Cache也是哈希桶结构，但是它的映射规则跟Central Cache与Thread Cache不同。每个桶是一个按桶的位置映射页数的自由链表，一共有128个哈希桶来存放自由链表。Page Cache也采用SpanList进行管理，每个上挂的是span对象，不会切割，因为Page Cache服务的是Central Cache，切割的工作由Central Cache拿到span后自己完成



Page Cache中不能再实现成桶锁了，要实现成全局锁，因此Page Cache要做span的分裂与合并，桶锁只能制约不同哈希桶下的span取用，但是不能限制跨桶之间span的分裂与合并。除此之外，当没有对应page的span时，每往后找一次大的span都要上锁、解锁，这种消耗可能会很大，干脆用一把大的全局锁



### NewSpan：获取一个k页的span

直接去跟OS要的时候不要一直要小块的内存，为了减少外碎片，尽量每次要就要比较大块的内存。所以没有合适的span时是先去找大的span然后进行分裂，而不是直接去向OS申请。若真的一个大的都没有找到，就向系统申请一个128页的内存块，并挂到128 page的哈希桶上

以申请一个2 page的内存块为例，若一个都没有找到，就去申请一个128 page的内存块，然后切一个2 page出来给Central Page，另外126 page的内存块挂到126 page的哈希桶上。其实刚开始的时候就是这种情况，一个span都没有



切分成一个k页的span和一个n-k页的span，k页的span返回给Central Cache，n-k页的span挂到第n-k桶上去



NewSpan的递归可能涉及到递归死锁的问题。可以通过递归互斥锁 `recursive_mutex` 或者分离一个调用函数来解决死锁问题

还涉及到一个锁的问题，当Central Cache调用Page Cache的内容时，因为Central Cache是桶锁，而Page Cache是全局锁，要不要先解开桶锁再加全局锁呢。这个问题比较有争议，但总的来说解了比较好。虽然本身是因为没有空闲的span了才会去找Page Cache要span，所以对于其他线程的申请没什么价值，因为反正也是申请不到span的，但是如果是其他线程想要归还span呢？把释放也给堵住了不太好

最后考虑到这两个问题，Page Cache的全局锁加再调用 `NewSpan` 的 `CentralCache::GetOneSpan` 里面，虽然粒度大一点，但可以避免递归死锁



有没有这样一种可能，当把桶锁解开后，有很多线程访问这个桶都发现没有空的span，所以它们都会去调Page Cache的NewSpan，但是因为Page Cache这时候已经上了全局锁，所以他们也申请不到。然后等到申请完后解开全局锁，大家都申请到了新的span要挂到Central Cache中对应的桶上，然后就有太多内存了？

### 内存回收：ReleaseSpanToPageCache

若直接把从Central Cache返回的切小的Span挂到对应Page桶上的话，会造成小Page的很多，而大Page的很少，所以也要对span前后的页尝试进行合并，最后也还给系统，缓解外内存碎片问题

借助 `_idSpanMap` 映射的帮助，可以根据 `PageID` 往前往后不断合并 `_n` 页，合并完了之后可能还可以继续合并，但如果是还在Central Cache 中的就不能合并了。但是如何判断是在Central Cache中还是在Page Cache中呢？不能使用 `_useCount` 来判断，因为存在线程安全问题，之前锁过在NewSpan里桶锁解开到重新上锁那段时间间隔中，从Page Cache刚拿过来的正在切分的 `_useCount` 也为0。若此时把这段刚拿过来准过要分开的span给合并了那就完了

解决方法是在Span里增加一个 `bool _isUse=false;` 属性来表明是否在使用，只要分配给了Central Cache，那么就要变成 `true`

<img src="C:\Users\Weijian Feng\iCloudDrive\Desktop\Notes\CS\内存池\doc\img\span合并.drawio.png" width="70%">

注意和Central Cache中的不同，kSpan是返回给Central Cache的，它之后会被切分为小块内存，所以每一个页都要建立映射，可以让它在在从Thread Cache返回给Central Cache的时候确认是属于哪一个Span的。而nSpan暂时是留在Page Cache里的，它暂时不需要被切分。但是我们也要通过它来进行合并以返回给系统，返回的时候确认id和span的映射只需要找首尾巴的页就行了，因为需要向前向后合并，既然还没有被喂给Central Cache，把中间必然是连续的





为什么要 `delete prevSpan`？因为Span结构体是我们自己创造出来管理span对象的，当被合并了之后也就没有用了，所以delete掉。注意不要和申请的内存混淆，通过内存池申请的内存在程序运行期间不会被还给内存，就算不用了也都是集中到Page Cache手里。只有最后程序终止后，才会被还给OS

### 大于256KB的内存回收

因为Page Cache最高可以管理128页的内存块，所以当申请64~128页（假设一页为4KB，`256/4=64`）的内存时，还是找的Page Cache；若直接申请超过128页的那么就得去找堆要了

## *Benchmark*

### 多线程环境对比malloc测试

统一设定参数每轮申请10000次，一共进行10轮，观察不同线程的申请效果

单进程实验：可以看到，我自己实现的tcmalloc和glibc的ptmalloc2相比，尽管实际实现的tcmalloc代码量大概在十万行细节，我这里仅仅实现了核心的一两千行代码，很多细节都没有考虑在内，但性能已经相差不多了

<img src="C:\Users\Weijian Feng\iCloudDrive\Desktop\Notes\CS\内存池\doc\img\BenchmarkBeforeRadix.png">



### 性能瓶颈分析

<img src="C:\Users\Weijian Feng\iCloudDrive\Desktop\Notes\CS\内存池\doc\img\性能测试.png">

直接利用vs提供的性能分析工具，其中第一项是Benchmark的加锁，但是可以发现第三项占用了大量时间

可以发现大量的时间浪费在了锁竞争上，这个锁竞争是在Page Cache去 `std::unordered_map<PAGE_ID, Span *>` 中寻找span的时候产生的，之所以这时候要加锁解锁是因为其他线程可能会增删底层的红黑树（旋转平衡等会更改指针关系），所以优化的方向是如何去减轻这里的锁竞争程度

## *优化*

### 使用定长内存池替换new

tcmalloc是要替换ptmalloc的，但是之前的代码里仍然用到了ptmalloc，也就是 `new Span` 的时候

我们可以采用为PageCache添加一个高效的定长内存池 `ObjectPool<Span> _spanPool;` 来专门获取 Span 对象

ConcurrentAlloc.h和ObjectPool.h里获取ThreadCache同理

### Free不传对象大小

每一个span里的管的小内存块都是切成同样大小的小块，所以干脆让span记录一个它管的小块内存的大小，方便归还的时候找对应哪一个哈希桶

### 字典树和基数树

Trie（re**Trie**val）**前缀树**或**字典树**，是一种有序树，用于保存关联数组，其中的键通常是字符串。与二叉查找树不同，键不是直接保存在节点中，而是由节点在树中的位置决定。一个节点的所有子孙都有相同的前缀，也就是这个节点对应的字符串，而根节点对应空字符串。一般情况下，不是所有的节点都有对应的值，只有叶子节点和部分内部节点所对应的键才有相关的值

<img src="C:\Users\Weijian Feng\iCloudDrive\Desktop\Notes\CS\内存池\doc\img\字典树.png" width="30%">

* 根节点不包含字符，除根节点外每一个节点都**只包含一个Key的一个字符**
* 从根节点到某一节点，路径上经过的字符连接起来，为该节点对应的Key
* 每个节点的所有子节点包含的字符都不相同，即Key不重复

<https://juejin.cn/post/6933244263241089037>

**基数树Radix Tree/Patricia Tree**就是带压缩的字典树，它的计数统计原理和Trie Tree极为相似，一个最大的区别点在于它不是每一个节点保存一个字符Key，而是可以以1个或多个字符叠加作为一个分支。这就避免了长字符key会分出深度很深的节点。**Trie和Radix Tree实际上都是分层哈希的思想，哈希Key是由多层合并组成的**

树的叶子结点是实际的数据条目。每个结点有固定数量 `2^n`指针指向子结点（每个指针称为槽slot，n为划分的基的大小）

<img src="C:\Users\Weijian Feng\iCloudDrive\Desktop\Notes\CS\内存池\doc\img\RadixTree.png" width="50%">

### 用基数树优化

假设一页是 `2^13=8KB`，那么一共有 `2^32/2^13=2^19=524,288` 页。占用内存为 `2^19*4 Byte=2^21=2MB`。基数树会建立页号 `_pageID` 和指针 `Span *` 的映射，`BITS = 32/64-PAGE_SHIFT`，这里一层基数树的基为 `BITS=19`

分别设计了三种基数树，即一层、两层和三层的，这三种基数树使用的空间没有变化的。32位下的时候一层或两层基数树就够了，但64位下是不够的  `2^64/2^13=2^51`，直接开一个连续的 `2^51` 的Vector 是不可能的，所以得用3层。一层的优势在于直接就开好了，访问非常简单，**一层本质上就是一个一次性开完的有 `2^19` 个元素的大哈希vector** 。多层的是多层哈希，稍微麻烦了一点，实际上两层也是直接开好，但是三层就要按需开了

<img src="C:\Users\Weijian Feng\iCloudDrive\Desktop\Notes\CS\内存池\doc\img\两层基数树.drawio.png">

**为什么此时去基数树里操作就不需要加锁解锁了？**

1. 首先基数树是直接把整棵树给开出来，或者当三层的时候是提前通过ensure来开好内存的
2. 设计成了读写分离的模式，首先可以分析一下只有两个地方需要写，即 `Span *PageCache::NewSpan(size_t k);` 和 `void PageCache::ReleaseSpanToPageCache(Span *span);` 的时候需要写（即建立 `PAGE_ID` 和 `Span *` 的映射关系），这时候是加了Page Cache的全局锁的。实际上不加锁都可以，因为不可能同时对一块Span既申请又释放

### 采用基数树后再次进行Benchmark

多线程实验，在实现了基数树管理Span之后效率提升十分明显，尤其是在多线程条件性能差异巨大

<img src="C:\Users\Weijian Feng\iCloudDrive\Desktop\Notes\CS\内存池\doc\img\BenchmarkAfterRadix.png">

## *工程管理*

### Cmake编写

### 打包成库

## *调研与对比内存分配器ptmalloc、jemalloc和tcmalloc*

<https://www.kandaoni.com/news/22268.html>

### jemalloc

 jemalloc 是由 Jason Evans 在 FreeBSD 项目中引入的新一代内存分配器。它是一个通用的 malloc 实现，侧重于减少内存碎片和提升高并发场景下内存的分配效率，其目标是能够替代 malloc。jemalloc 在 2005 年首次作为 FreeBSD libc 分配器使用，2010年，jemalloc 的功能延伸到如堆分析和监控/调优等。现代的 jemalloc 版本依然集成在 FreeBSD 中。jemalloc目前在firefox、facebook服务器各种组件中大量使用

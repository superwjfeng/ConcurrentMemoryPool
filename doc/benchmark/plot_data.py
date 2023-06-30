import matplotlib.pyplot as plt

threads = []
times_malloc = []
times_concurrentMalloc = []
times_concurrentMallocRadixTree = []

# 读取文件并解析线程数和执行时间
with open("dataMalloc.txt", "r") as file:
    for line in file:
        thread, time_malloc = line.strip().split(":")
        threads.append(int(thread))
        times_malloc.append(int(time_malloc))

with open("dataConcurrentMalloc.txt", "r") as file:
    for line in file:
        _, time_concurrentMalloc = line.strip().split(":")
        times_concurrentMalloc.append(int(time_concurrentMalloc))

with open("dataConcurrentMallocRadixTree.txt", "r") as file:
    for line in file:
        _, time_concurrentMallocRadixTree = line.strip().split(":")
        times_concurrentMallocRadixTree.append(int(time_concurrentMallocRadixTree))

# 去重
threads_unique = list(set(threads))
threads_unique.sort()


# 绘制折线图
plt.figure()
plt.plot(threads_unique, times_malloc, marker='*', label='malloc', color='red');
plt.plot(threads_unique, times_concurrentMalloc, marker='^', label='ConcurrentMalloc', color='blue')
plt.legend()
plt.xlabel("Threads")
plt.ylabel("Time (ms)")
plt.title("Benchmark Results")
plt.savefig("BenchmarkBeforeRadix.png")
#plt.show()

plt.figure()
plt.plot(threads_unique, times_concurrentMalloc, marker='^', label='ConcurrentMalloc', color='blue');
plt.plot(threads_unique, times_concurrentMallocRadixTree, marker='d', label='ConcurrentMallocRadixTree', color='green')
plt.legend()
plt.xlabel("Threads")
plt.ylabel("Time (ms)")
plt.title("Benchmark Results")
plt.savefig("BenchmarkAfterRadix.png")
#plt.show()

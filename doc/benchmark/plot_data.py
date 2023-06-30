import matplotlib.pyplot as plt

threads = []
times = []

# 读取文件并解析线程数和执行时间
with open("dataMalloc.txt", "r") as file:
    for line in file:
        thread, time = line.strip().split(",")
        threads.append(int(thread))
        times.append(int(time))

# 去重
threads_unique = list(set(threads))
threads_unique.sort()

# 为每个线程数计算平均执行时间
avg_times = []
for thread in threads_unique:
    thread_times = [time for t, time in zip(threads, times) if t == thread]
    avg_time = sum(thread_times) / len(thread_times)
    avg_times.append(avg_time)

# 绘制折线图
plt.plot(threads_unique, avg_times)
plt.xlabel("Threads")
plt.ylabel("Time (ms)")
plt.title("Benchmark Results")
plt.show()

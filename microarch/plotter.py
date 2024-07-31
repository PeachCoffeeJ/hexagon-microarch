import sys
import collections
import matplotlib.pyplot as plt

hit_times = []
miss_times = []
for l in sys.stdin:
    if "cycles: " in l:
        triplet = l.split("cycles: ")
        t = int(triplet[1])
        if t < 100:
            hit_times.append(t)
        elif t < 1000:
            miss_times.append(t)

hit_times_counter = collections.Counter(hit_times)
hit_times_counter_dict = dict(hit_times_counter)
sorted_hit_times_counter_dict = collections.OrderedDict(sorted(hit_times_counter_dict.items()))

miss_times_counter = collections.Counter(miss_times)
miss_times_counter_dict = dict(miss_times_counter)
sorted_miss_times_counter_dict = collections.OrderedDict(sorted(miss_times_counter_dict.items()))

fig, ax = plt.subplots(figsize=(10, 3))

ax.bar(list(sorted_hit_times_counter_dict.keys()), list(sorted_hit_times_counter_dict.values()), color='b', label = "Cache Hit")
ax.bar(list(sorted_miss_times_counter_dict.keys()), list(sorted_miss_times_counter_dict.values()), color='r', label = "Cache Miss")
xticks = list(map(lambda t: 100 * t, range(500 // 100 + 1)))
#xtick_labels = [str(t) if t % 2 == 0 else "" for t in range(1000//1+1)]
ax.set_xticks(xticks)
#ax.set_xticklabels(xtick_labels)
ax.set_xlim(xmin=0, xmax=500)
#fig.legend()
fig.supxlabel("Access Time (cycles)", fontsize="large")
plt.subplots_adjust(bottom = 0.15)
plt.legend(loc='upper right')
#ax.grid()
plt.savefig("distribution.pdf")
print("Histogram saved as distribution.pdf.")
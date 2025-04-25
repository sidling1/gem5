import subprocess
bench = ["cactus",  "leela" , "sjeng", "lbm"]

["xalancbmk", "gcc", "namd"]
def get_stats(path, cycles, name):
    f = open(path)
    misses = 0
    local_replies = 0
    stored = 0
    total_instr = 0
    for line in f:
        parts = line.split()
        if len(parts) > 0:
            if parts[0].startswith("system.ruby.l1") and parts[0].endswith("m_demand_misses"):
                # print(line)
                misses += int(parts[1])
            if parts[0].endswith("packets_stored"):
                # print(line)
                stored += int(parts[1])
            if parts[0].endswith("local_replies"):
                # print(line)
                local_replies += int(parts[1])
            if parts[0].startswith("system.switch") and parts[0].endswith("fetchStats0.numInsts"):
                # print(line)
                total_instr += int(parts[1])
    # print(f"-------- {path} ------------")
    # print("MPKI : " , (misses / total_instr) * 1000)
    # print("% local replies : ", (local_replies / misses) * 100)
    # print("% stored that were used : ", (local_replies / stored) * 100)
    return (name, cycles,(local_replies / misses) * 100)
    f.close()
# Analyze these benchmarks
("cactus", 21_000_000, 500_000, 6.768414067684141)
("cactus", 100_000_000, 20_000_000, 15.29504371937339)
("cactus", 100_000_000, 150_000_000, 17.63206340621725)
("leela", 100_000_000, 20_000_000, 10.597140454163162)
# Question 1 : What is the load type ? misses/per kilo instructions , MPKI
fast_fwd = f"--fast-forward={110_000_000}"
# fast_fwd = ""
# instr_cnt = "" 308 
instr_cnt = f"-I {1_000_000}"
num_cpus = [1, 4, 16, 64]
benches = ["cactus", "leela"]
bench = "cactus"
n = 1
rows = 1
times = [256, 512, 1024, 2048, 10000, 20000]
time_to_store = 2048
# for time_to_store in times:
#     dir_name = f"-d /home/siddhant/Desktop/btp/v23/gem5/gem5_outputs/{bench}_{time_to_store}"
#     cmd = f"build/X86_MOESI_CMP_directory/gem5.opt {dir_name} configs/deprecated/example/se.py --num-cpus={n} --num-dirs={n} --sys-clock=2GHz --topology=Mesh_XY --mesh-rows={rows} --ruby --num-l2caches={n} --network=garnet --mem-size=8GB --caches --l2cache --routing-algorithm=1 --router-latency=1 {instr_cnt} --bench={bench} {fast_fwd} --enable-eviction-store --time-to-store={time_to_store}"
#     subprocess.run(cmd, shell=True, check=True)

data = []
for bench in benches:
    for time_to_store in times:
        dir_name = f"-d /home/siddhant/Desktop/btp/v23/gem5/gem5_outputs/{bench}_{time_to_store}"
        data.append(get_stats(dir_name[3:] + "/stats.txt", time_to_store, bench))

print(data)


import pandas as pd
import matplotlib.pyplot as plt

# # Example data: [(name, cycles, (local_replies / misses) * 100)]
# data = [
#     ('name1', 1, 80),
#     ('name1', 2, 90),
#     ('name1', 3, 85),
#     ('name2', 1, 70),
#     ('name2', 2, 75),
#     ('name2', 3, 78),
#     ('name3', 1, 60),
#     ('name3', 2, 65),
#     ('name3', 3, 68)
# ]

# Convert the list into a pandas DataFrame
df = pd.DataFrame(data, columns=['name', 'cycles', 'value'])

# Grouping the data by 'name' and plotting the cycles vs. value for each name
plt.figure(figsize=(10,6))

# Loop through each unique name and plot its cycle vs. value
for name in df['name'].unique():
    subset = df[df['name'] == name]
    # Plot the line
    plt.plot(subset['cycles'], subset['value'], label=name, marker='o', markersize=1)
    # Scatter points for clear visibility
    plt.scatter(subset['cycles'], subset['value'], marker='o', s=100)

# Adding labels and title
plt.xlabel('Time to Store (Cycles)')
plt.ylabel('% of Misses satisfied by Local Reply')
plt.title('Time to Store vs % of Misses satisfied by Local Reply')

# Place the legend at the top-left
plt.legend(loc='upper left', title='Name')

# Display the plot
plt.show()



# Question 2 : Re-reference interval for this program

# Question 3 : VC availability and usage stats

# bench = "-".join([bench for i in range(16)])
# cmd = f"build/X86_MESI_Three_Level/gem5.opt -d gem5_outputs/prefetched configs/deprecated/example/se.py --num-cpus=16 --num-dirs=16 --sys-clock=2GHz --topology=Mesh_XY --mesh-rows=4 --ruby --num-l2caches=16 --network=garnet --mem-size=8GB --caches --l2cache --routing-algorithm=1 --router-latency=1 {instr_cnt} --bench={bench} {fast_fwd} --enable-prefetch --time-to-store={time_to_store}"
# subprocess.run(cmd, shell=True, check=True)
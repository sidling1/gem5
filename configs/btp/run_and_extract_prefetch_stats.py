import subprocess
import os
import re

# sudo build/X86_MESI_Three_Level/gem5.opt --debug-flags=RubyPrefetcher configs/deprecated/example/se.py --num-cpus=16 --num-dirs=16 --sys-clock=2GHz --topology=Mesh_XY --mesh-rows=4 --ruby --num-l2caches=16 --network=garnet --mem-size=8GB --caches --l2cache --routing-algorithm=1 --router-latency=1 -I 1000000 --fast-forward=100000 --bench=namd-namd-namd-namd-namd-namd-namd-namd-namd-namd-namd-namd-namd-namd-namd-namd --enable-prefetch

# Paths (adjust as per your setup)
GEM5_BINARY = "./build/X86_MESI_Three_Level/gem5.opt"
CONFIG_SCRIPT = "configs/deprecated/example/se.py"
OUTPUT_BASE = "gem5_outputs"

CONST_OPTIONS = "--num-cpus=16 --num-dirs=16 --sys-clock=2GHz --topology=Mesh_XY --mesh-rows=4 --ruby --num-l2caches=16 --network=garnet --mem-size=8GB --caches --l2cache --routing-algorithm=1 --router-latency=1 -I 1000000 --fast-forward=10000 --enable-prefetch"

# Configurations to test

benchmark_options = [
    ["namd" for i in range(16)],
    ["gcc" for i in range(16)],
    ["namd" if i % 2 else "gcc" for i in range(16)],
    ["namd" if i < 8 else "gcc" for i in range(16)],
    ["namd" if i >= 8 else "gcc" for i in range(16)],
]

# Stat parameters to extract
# stat_keys = ["sim_ticks", "sim_insts", "system.cpu.ipc"]

def run_gem5(benchmark_options, run_name):
    output_dir = os.path.join(OUTPUT_BASE, run_name)
    os.makedirs(output_dir, exist_ok=True)

    benchmark_string = "-".join(benchmark_options)

    command = f"{GEM5_BINARY} -d {output_dir} {CONFIG_SCRIPT} {CONST_OPTIONS} --bench={benchmark_string}"
    # print(command)
    # command = [GEM5_BINARY, "-d", output_dir, CONFIG_SCRIPT, CONST_OPTIONS, f"--bench={benchmark_string}"]
    print(f"Running: {command}")
    subprocess.run(command, shell=True, check=True)

def extract_stats(stats_file):
    results = {}
    with open(stats_file, "r") as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) > 0 :
                first_parts = parts[0].split('.')
                if len(first_parts) >= 2:
                    if first_parts[-2] == 'RubyPrefetcher':
                        # print(first_parts[-1], parts[1])
                        results[line] = parts[1]

    return results

def main():
    all_results = []

    for i, opts in enumerate(benchmark_options):
        run_name = f"run_{i}"
        run_gem5(opts, run_name)

        stats_path = os.path.join(OUTPUT_BASE, run_name, "stats.txt")
        if os.path.exists(stats_path):
            stats = extract_stats(stats_path)
            all_results.append((run_name, stats))
        else:
            print(f"Stats file not found for {run_name}")

    # Display collected results
    f = open('configs/btp/results.txt', "w+")

    f.write("\n=== Summary ===\n")    
    for run_name, stats in all_results:
        f.write(f"{run_name}:\n")
        for k, v in stats.items():
            f.write(f"  {k}: {v}\n")
        f.write("\n")

if __name__ == "__main__":
    main()

import subprocess
import os
import re

bench_path = f"/home/siddhant/Desktop/btp/gem5/benchmarks/cpu2017/"

benchmarks = os.listdir(f"{bench_path}/binaries")
print(benchmarks)

with open("configs/btp/output.txt", "w+") as w:
    for bench in benchmarks:
        print(bench)
        # Run the command here

        # This is a single core command
        command = f"./build/X86_MESI_Three_Level/gem5.opt configs/deprecated/example/se.py -I 50000000 --sys-clock=2GHz --mem-size=8GB --caches --l2cache --bench={bench} --topology=Mesh_XY --mesh-rows=1 --num-cpus=1 --num-dirs=1 --ruby --num-l2caches=1 --network=garnet --routing-algorithm=1 --enable-prefetch"

        # Running the command
        try :
            subprocess.run(command, shell=True, check=True, timeout=5*60)

            # Now check the results file
            stats_file = "./m5out/stats.txt"
            stats_to_get = [
                "system.ruby.l0_cntrl0.prefetcher.RubyPrefetcher",
                "system.ruby.l1_cntrl0.cache",
            ]

            # total_evictions = m_evictions + m_prefetch_evictions
            with open(stats_file, "r") as f:
                w.write(f"----------------{bench}-------------------\n")
                for line in f:
                    for stat in stats_to_get:
                        if line.startswith(stat):
                            w.write(line)
        except : 
            w.write(f"----------------{bench}-------------------\n")
            w.write("!! some error occured !!\n")

        

            

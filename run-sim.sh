# ~/bin/bash

./build/X86/gem5.opt ./configs/example/garnet_synth_traffic.py --network=garnet --num-cpus=16 --num-dirs=16 --l2cache --num-l2cache=16 --ruby --topology=Mesh_XY --sim-cycles=10000 --mesh-rows=4 --routing-algorithm=1

./build/X86_MOESI_CMP_directory/gem5.opt ./configs/example/btp_run_progs.py --network=garnet --num-cpus=16 --num-dirs=16 --l2cache --num-l2cache=16 --ruby --topology=Mesh_XY --mesh-rows=4 --routing-algorithm=1

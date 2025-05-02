#!/bin/bash -l
#$ -l h_rt=96:00:00
#$ -N omnetpp_arc_4MB
#$ -pe omp 1

cd /projectnb/ec513/students/${USER}/PROJ/spec-2017/gem5_arc
source /projectnb/ec513/materials/HW2/spec-2017/sourceme
mkdir -p ../results/omnetpp_arc_4MB_proj
build/X86/gem5.opt \
--outdir=../results/omnetpp_arc_4MB_proj \
configs/example/gem5_library/x86-spec-cpu2017-benchmarks-4MB.py \
--image ../disk-image/spec-2017/spec-2017-image/spec-2017 \
--partition 1 \
--benchmark 520.omnetpp_r \
--size test


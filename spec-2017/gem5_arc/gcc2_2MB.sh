#!/bin/bash -l
#$ -l h_rt=96:00:00
#$ -N gcc2_arc_2MB
#$ -pe omp 1

cd /projectnb/ec513/students/${USER}/PROJ/spec-2017/gem5_arc
source /projectnb/ec513/materials/HW2/spec-2017/sourceme
mkdir -p ../results/gcc2_arc_2MB_proj
build/X86/gem5.opt \
--outdir=../results/gcc2_arc_2MB_proj \
configs/example/gem5_library/x86-2MB-spec-cpu2017-benchmarks.py \
--image ../disk-image/spec-2017/spec-2017-image/spec-2017 \
--partition 1 \
--benchmark 502.gcc_r \
--size test


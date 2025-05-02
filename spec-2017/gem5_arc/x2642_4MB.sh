#!/bin/bash -l
#$ -l h_rt=96:00:00
#$ -N x2642_arc_4MB
#$ -pe omp 1

cd /projectnb/ec513/students/${USER}/PROJ/spec-2017/gem5_arc
source /projectnb/ec513/materials/HW2/spec-2017/sourceme
mkdir -p ../results/x2642_arc_4MB_proj
build/X86/gem5.opt \
--outdir=../results/x2642_arc_4MB_proj \
configs/example/gem5_library/x86-4MB-spec-cpu2017-benchmarks.py \
--image ../disk-image/spec-2017/spec-2017-image/spec-2017 \
--partition 1 \
--benchmark 525.x264_r \
--size test


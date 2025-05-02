#!/bin/bash -l
#$ -l h_rt=49:00:00
#$ -N cam4_lru
#$ -pe omp 1

cd /projectnb/ec513/students/${USER}/HW4/spec-2017/gem5_lru
source /projectnb/ec513/materials/HW2/spec-2017/sourceme
mkdir -p ../results/cam4_lru
build/X86/gem5.opt \
--outdir=../results/cam4_lru \
configs/example/gem5_library/x86-spec-cpu2017-benchmarks.py \
--image ../disk-image/spec-2017/spec-2017-image/spec-2017 \
--partition 1 \
--benchmark 527.cam4_r \
--size test


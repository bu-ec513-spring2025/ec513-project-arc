#!/bin/bash -l
#$ -l h_rt=72:00:00
#$ -N bwaves
#$ -pe omp 1

cd /projectnb/ec513/students/${USER}/HW2/spec-2017/gem5
source /projectnb/ec513/materials/HW2/spec-2017/sourceme
mkdir -p ../results/bwaves
build/X86/gem5.opt \
--outdir=../results/bwaves \
configs/example/gem5_library/x86-spec-cpu2017-benchmarks.py \
--image ../disk-image/spec-2017/spec-2017-image/spec-2017 \
--partition 1 \
--benchmark 503.bwaves_r \
--size test

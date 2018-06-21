#!/bin/bash
#SBATCH --nodes=2
#SBATCH --ntasks=128
#SBATCH --partition=knl

echo rank iter size bufsize noise time
for i in `awk 'BEGIN {for(i = 1; i < 2**16; i = i*2) print i}'`; do
	for j in `awk 'BEGIN {for(i = 4096; i < 2**24; i = i*2) print i}'`; do
		srun ./a.out -n $i -z $j
		sleep 1
	done
done

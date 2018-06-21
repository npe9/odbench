#!/bin/bash

echo rank iter size bufsize noise time
for i in `awk 'BEGIN {for(i = 1; i < 2**16; i = i*2) print i}'`; do
	srun -p knl -N 2 -n 128 ./a.out -n $i
done

#!/bin/bash
echo "EDGE DETECTION - CUDA - VARIABLE THREADS - START"
echo "run;image;threadsperblock;time" > results_cuda_vt.csv
for (( i=1; i<=5; i++ ));
do
	for (( j=1; j<=3; j=j+1 ));
	do
		for (( k=32; k<=1024; k=k*2 ));
		do
			output=$(eval "time ./EdgeDetectionCudaVariableThreads image_"${j}".pgm image_"${j}"_out_cuda.pgm "${k}"" 2>&1)
			realTime=${output:8:5}
			echo ""$i";"$j";"$k";"$output >> results_cuda_vt.csv
		done
	done
done
echo "EDGE DETECTION - CUDA - VARIABLE THREADS - END"

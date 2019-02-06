#!/bin/bash
echo "EDGE DETECTION - CUDA - START"
echo "run;image;time" > results_cuda.csv
for (( i=1; i<=5; i++ ));
do
	for (( j=1; j<=3; j=j+1 ));
	do
		output=$(eval "time ./EdgeDetectionCuda image_"${j}".pgm image_"${j}"_out_cuda.pgm" 2>&1)
		realTime=${output:8:5}
		echo ""$i";"$j";"$output >> results_cuda.csv
	done
done
echo "EDGE DETECTION - CUDA - END"

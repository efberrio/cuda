#!/bin/bash
echo "EDGE DETECTION - OPENCL - START"
echo "run;image;time" > results_opencl.csv
for (( i=1; i<=5; i++ ));
do
	for (( j=1; j<=3; j=j+1 ));
	do
		output=$(eval "time ./EdgeDetectionOpenCL image_"${j}".pgm image_"${j}"_out_opencl.pgm" 2>&1)
		realTime=${output:8:5}
		echo ""$i";"$j";"$output >> results_opencl.csv
	done
done
echo "EDGE DETECTION - OPENCL - END"

#!/bin/bash
echo "EDGE DETECTION - SEQUENTIAL O3 - START"
echo "run;image;time" > results_sequential_o3.csv
for (( i=1; i<=5; i++ ));
do
	for (( j=1; j<=3; j=j+1 ));
	do
		output=$(eval "time ./EdgeDetectionO3 image_"${j}".pgm image_"${j}"_out_seq_o3.pgm" 2>&1)
		realTime=${output:8:5}
		echo ""$i";"$j";"$output >> results_sequential_o3.csv
	done
done
echo "EDGE DETECTION - SEQUENTIAL O3 - END"

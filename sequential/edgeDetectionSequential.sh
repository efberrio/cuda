#!/bin/bash
echo "EDGE DETECTION - SEQUENTIAL - START"
echo "run;image;time" > results_sequential.csv
for (( i=1; i<=5; i++ ));
do
	for (( j=1; j<=3; j=j+1 ));
	do
		output=$(eval "time ./EdgeDetection image_"${j}".pgm image_"${j}"_out_seq.pgm" 2>&1)
		realTime=${output:8:5}
		echo ""$i";"$j";"$output >> results_sequential.csv
	done
done
echo "EDGE DETECTION - SEQUENTIAL - END"

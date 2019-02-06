#!/bin/bash
echo "EDGE DETECTION - OMP PAD - START"
echo "threads;run;image;time" > results_omp_pad.csv
for (( i=2; i<=8; i=i*2 ));
do
	for (( j=1; j<=5; j=j+1 ));
	do
		for (( k=1; k<=3; k=k+1 ));
		do
			output=$(eval "time ./EdgeDetectionPAD image_"${k}".pgm image_"${k}"_out.pgm" ${i} 2>&1)
			realTime=${output:8:5}
			echo ""$i";"$j";"$k";"$output >> results_omp_pad.csv
		done
	done
done
echo "EDGE DETECTION - OMP PAD - END"

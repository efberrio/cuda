#!/bin/bash
echo "EDGE DETECTION - OMP - START"
echo "threads;run;image;time" > results_omp.csv
for (( i=2; i<=8; i=i*2 ));
do
	for (( j=1; j<=5; j=j+1 ));
	do
		for (( k=1; k<=3; k=k+1 ));
		do
			output=$(eval "time ./EdgeDetection image_"${k}".pgm image_"${k}"_out.pgm" ${i} 2>&1)
			realTime=${output:8:5}
			echo ""$i";"$j";"$k";"$output >> results_omp.csv
		done
	done
done
echo "EDGE DETECTION - OMP - END"

#!/bin/bash
echo "EDGE DETECTION - MPI - SINGLE MACHINE - START"
echo "run;image;threadsperblock;time" > results_mpi_single.csv
for (( i=1; i<=5; i++ ));
do
	for (( j=1; j<=3; j=j+1 ));
	do
		for (( k=3; k<=12; k=k*2 ));
		do
			output=$(eval "time mpirun -np "${k}" -mca btl ^openib EdgeDetectionMPI image_"${j}".pgm image_"${j}"_out_mpi.pgm" 2>&1)
			realTime=${output:8:5}
			echo ""$i";"$j";"$k";"$output >> results_mpi_single.csv
		done
	done
done
echo "EDGE DETECTION - MPI - SINGLE MACHINE - END"

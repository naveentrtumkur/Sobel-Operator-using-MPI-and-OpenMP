/*
 * Program-5: Sobel edge detection MPI with OpenMp Paralellization  
 * */

#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <math.h>
#include "read_bmp.h"
#include <stdlib.h>
#include <time.h>

// Define master thread as 0
#define MASTER	0

// Tested with 64 other threads for OMP usage.
#define NUM_OF_THREADS 64 

//Currently I'm not calling this function in main.
//The problem statement is to use MPI and parallelise convergence loop
//using OpenMP. This is done within the main program.
/*
 *   Serial function.
 *   in_file - The input image file.
 *   Returns the threshold value. 
 * 
 * */
int serial_processing(FILE *in_file) 
{
    int i,j;
    uint8_t *bmp_data = (uint8_t *) read_bmp_file(in_file);

    //Allocate new output buffer of same size
    uint8_t* new_bmp_img = (uint8_t*)malloc(get_num_pixel());
    //Initialize the array to 0.

    for (i = 0 ;i < get_num_pixel(); i++) 
    {
	new_bmp_img[i] = 0;
    }

    //Get image attributes
	
    uint32_t wd = get_image_width();
    uint32_t ht = get_image_height();

    uint32_t threshold = 0;
    uint32_t black_cell_count = 0;

    // Serial version
    while (black_cell_count < (75 * wd * ht/ 100))
	{
		black_cell_count = 0;
		threshold += 1;
		for (i=1; i < (ht-1); i++)
		{
			for (j=1; j < (wd-1); j++)
			{
				float Gx = bmp_data[ (i-1)*wd + (j+1) ] - bmp_data[ (i-1)*wd + (j-1) ]
					+ 2*bmp_data[ (i)*wd + (j+1) ] - 2*bmp_data[ (i)*wd + (j-1) ]
					+ bmp_data[ (i+1)*wd + (j+1) ] - bmp_data[ (i+1)*wd + (j-1) ];
				float Gy = bmp_data[ (i-1)*wd + (j-1) ] + 2*bmp_data[ (i-1)*wd + (j) ]
					+ bmp_data[ (i-1)*wd + (j+1) ] - bmp_data[ (i+1)*wd + (j-1) ]
					- 2*bmp_data[ (i+1)*wd + (j) ] - bmp_data[ (i+1)*wd + (j+1) ];
				float  mag = sqrt(Gx * Gx + Gy * Gy);

				if (mag > threshold) {
					new_bmp_img[ i * wd + j] = 255;
				} else {
					new_bmp_img[ i * wd + j] = 0;
					black_cell_count++;
				}
			}
		}

	}

	//free(bmp_data);
	//free(new_bmp_img);
	return threshold;
}


// Main program which calls the serial and the Parallel MPI version of sobel program.
int main(int argc, char* argv[]) {
	// Declare the tiem variables.
	struct timespec start, end;
	//Open the input and output files. 
	FILE *in_file = fopen(argv[1], "rb");
	FILE *out_file = fopen(argv[2], "wb");
	
	//Define ranks and number of threads.
	int rank;
	int slaves;
	MPI_Status status;
	uint8_t *bmp_data, *new_bmp_serial, *new_bmp_img;
	uint32_t wd, ht, threshold; 
	
	//Define the blackcell count and global black cell count.
	unsigned int  black_cell_count, global_black_cell_count;
	int i, j, sourceProcess;
	int type = 1;
	uint32_t avg_rows, extra_rows, rows, start_row, end_row; 

	//Initialise MPI, CommSize and CommRank.
	MPI_Init(NULL, NULL);	
	MPI_Comm_size(MPI_COMM_WORLD, &slaves);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	// Barrier Let master finish the Serial version.
	if (rank == MASTER) 
	{	
		printf("\n******* Parallel version *******\n");
	        //printf("%d slaves are reading the image\n", slaves);
	}
	//Insert a barrier.
	MPI_Barrier(MPI_COMM_WORLD);

	// Parallel version
	bmp_data = (uint8_t *) read_bmp_file(in_file);
	
	// memory for the new processed image.
	new_bmp_img = (uint8_t*) malloc(get_num_pixel());
	for (i = 0; i < get_num_pixel(); i++) 
	{
	new_bmp_img[i] = 0;
	}
	wd = get_image_width();
	ht = get_image_height();
	threshold = 0;
	black_cell_count = 0;
	global_black_cell_count = 0;

	//put a barrier
	MPI_Barrier(MPI_COMM_WORLD);	
	
	if (rank == MASTER) {
		clock_gettime(CLOCK_REALTIME, &start);
	}

	// Load distribution logic.
	avg_rows = ht / slaves;
	//After equal distribution, extra rows handles left over rows.
	extra_rows = ht % slaves;
	
	if(rank == slaves-1)
	rows = avg_rows + extra_rows;
	else
	rows = avg_rows;
	//rows  = (rank == slaves-1) ? avg_rows + extra_rows : avg_rows;
	// Based on the ran number, appropriate count of input cells are sent.
	start_row = rank * avg_rows;
	end_row = start_row + rows -1;
	float Gx, Gy, mag;
	//Start the convergence loop.
	while (global_black_cell_count < (75 * wd * ht/ 100)) 
        {
		//local blck_cell_count initialised to zero every iteration.
		black_cell_count = 0;
		// Insert a barrier so that all processes start fresh from here.
		MPI_Barrier(MPI_COMM_WORLD);
		threshold += 1;
		#pragma omp parallel num_threads(NUM_OF_THREADS)
		#pragma omp for reduction(+:black_cell_count)
		for (i = start_row; i <=  (end_row); i++)
		{
			//If first row or last row ignore it.
			if (i == 0 || i == ht-1)
				continue;
			for (j = 1; j < (wd-1); j++)
			{
				//calculation of Gx and Gy values.
				Gx = bmp_data[ (i-1)*wd + (j+1) ] - bmp_data[ (i-1)*wd + (j-1) ]
					+ 2*bmp_data[ (i)*wd + (j+1) ] - 2*bmp_data[ (i)*wd + (j-1) ]
					+ bmp_data[ (i+1)*wd + (j+1) ] - bmp_data[ (i+1)*wd + (j-1) ];
				Gy = bmp_data[ (i-1)*wd + (j-1) ] + 2*bmp_data[ (i-1)*wd + (j) ]
					+ bmp_data[ (i-1)*wd + (j+1) ] - bmp_data[ (i+1)*wd + (j-1) ]
					- 2*bmp_data[ (i+1)*wd + (j) ] - bmp_data[ (i+1)*wd + (j+1) ];
				mag = sqrt(Gx * Gx + Gy * Gy);
				if (mag > threshold) {
					new_bmp_img[ i * wd + j] = 255;
				} 
				else {
					new_bmp_img[ i * wd + j] = 0;
					// this increment is done by individual threads.
					black_cell_count++;
				}
			}
		}
		//Barrier so that all threads complete execution and arrive at this point.
		#pragma omp barrier
        
	// ALL processes Reduce the black cell count. global_black_cell count will have final computed value.
	MPI_Allreduce(&black_cell_count, &global_black_cell_count, 1, MPI_UNSIGNED, MPI_SUM,
				MPI_COMM_WORLD);
	}
	
	//If not a master just send the value.
	if (rank != MASTER) {

	MPI_Send(&new_bmp_img[start_row * wd], (rows)*wd, MPI_UNSIGNED_CHAR, MASTER,
			type, MPI_COMM_WORLD);
	//printf("Sending the computed image by %d", rank);
	} 
	//If it is a master thread receive the computation and image pixels from individual processes.
	else if (rank == MASTER){
		for (sourceProcess = 1 ; sourceProcess < slaves; sourceProcess++) {
			if(sourceProcess == slaves -1)
			rows = avg_rows + extra_rows;
			else
			rows = avg_rows;
			//rows  = (sourceProcess== slaves-1) ? avg_rows + extra_rows : avg_rows;
			start_row = sourceProcess * avg_rows;

	//Gather the image pixels values from the slaves.
	MPI_Recv(&new_bmp_img[start_row * wd],  (rows)*wd, MPI_UNSIGNED_CHAR, sourceProcess, type, 
				MPI_COMM_WORLD, &status);
	//printf("Received the image from %d rank, rows = %d",rank, rows*wd);	
	}

	// Write the output to Bmp output file(output_MPI.bmp)
	write_bmp_file(out_file, new_bmp_img);
	
	//Stop the timer and calculate the time taken.	
	clock_gettime(CLOCK_REALTIME, &end);
	double time_taken = ((double)end.tv_sec + 1.0e-9*end.tv_nsec) - \
			    ((double)start.tv_sec + 1.0e-9*start.tv_nsec);
	printf("\nTime taken for parallel sobel operation using MPI and OMP: %lf sec",time_taken);
	printf("\nThreshold during convergence: %d", threshold );
	printf("\n**************************************************\n");
	}
	
	//fclose(in_file);
	//fclose(out_file);
	//free(bmp_data);
	//free(new_bmp_img);
	
	// MPI finalise function to cleanup all the MPI processes.
	MPI_Finalize();

	return 0;

	}

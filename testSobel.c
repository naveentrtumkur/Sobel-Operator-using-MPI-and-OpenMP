/***************************************************************************************\
* CSE 5441	lab4									*
*											*
* sample serial sobel 									*
*    using bmpReader.o									*
*    functions provided:								*
*		(uint8_t *) read_bmp_file(FILE *inFile);				*
*		(uint32_t)  get_image_width();    					*
*		(uint32_t)  get_image_height();						*
*		(void)	    write_bmp_file(FILE *out_file, uint8_t *new_bmp_img);	*
\***************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "time.h"
#include <math.h>

#define MAX_ALLOWED_CMD_ARGS  3

#define HELP_MESSAGE 	"Check the command line arguments \n"

#include "read_bmp.h"

int main(int argc, char* argv[])
{
	int cmd_arg;
	uint8_t *bmp_data;
	uint8_t *new_bmp_img;
	uint32_t wd, ht;
	uint32_t i, j;
	uint32_t sum1, sum2, mag;
	uint32_t threshold;
	FILE *out_file, *inFile;
	time_t start_time, end_time;
	uint32_t percent_black_cells = 0;
	uint32_t total_cells;
	
	/*First Check if no of arguments is permissible to execute*/
	if (argc > MAX_ALLOWED_CMD_ARGS)
	{
		perror(HELP_MESSAGE);
		exit(-1);
	}
	
	/*Roll over the command line arguments and obtain the values*/
	for (cmd_arg = 1; cmd_arg < argc; cmd_arg++)
	{

		/*Switch execution based on the pass of commad line arguments*/
		switch (cmd_arg)
		{
			case 1: 
                inFile = fopen(argv[cmd_arg], "rb");
				break;
			
			case 2: 
                out_file = fopen(argv[cmd_arg], "wb");
				break;
		}
	}
	
	//Read the binary bmp file into buffer
	bmp_data = (uint8_t *)read_bmp_file(inFile);	
	
	//Allocate space for new sobel image
	new_bmp_img = (uint8_t *)malloc(get_num_pixel());
	wd = get_image_width();    
	ht = get_image_height();
	
	//start measurement for sobel loop only
	time(&start_time);
	
	threshold   = 0;
	total_cells = wd * ht;
	while(percent_black_cells < 75)
	{	

		percent_black_cells = 0;
		threshold += 1;
		
		for(i=1; i < (ht-1); i++)
		{
			for(j=1; j < (wd-1); j++)
			{
				sum1 = bmp_data[ (i-1)*wd + (j+1) ] - bmp_data[ (i-1)*wd + (j-1) ] \
						+ 2*bmp_data[ (i)*wd + (j+1) ] - 2*bmp_data[ (i)*wd + (j-1) ] \
						+ bmp_data[ (i+1)*wd + (j+1) ] - bmp_data[ (i+1)*wd + (j-1) ];
						
				sum2 = bmp_data[ (i-1)*wd + (j-1) ] + 2*bmp_data[ (i-1)*wd + (j) ] \
						+ bmp_data[ (i-1)*wd + (j+1) ] - bmp_data[ (i+1)*wd + (j-1) ] \
						- 2*bmp_data[ (i+1)*wd + (j) ] - bmp_data[ (i+1)*wd + (j+1) ];
						
				mag = sqrt(sum1 * sum1 + sum2 * sum2);
				if(mag > threshold)
				{
					new_bmp_img[ i*wd + j] = 255;
				}
				else
				{
					new_bmp_img[ i*wd + j] = 0;
					percent_black_cells++;
				}
			}
		}
		percent_black_cells = (percent_black_cells * 100) / total_cells;
	}
		
	//end bechmark measurement prior to writing out file
	time(&end_time);
	printf("Elapsed time for Sobel Operation time (time): %f\n", difftime(end_time,start_time));
	printf("Theshold: %d\n",threshold);
	
	//Write the buffer into the bmp file
	write_bmp_file(out_file, new_bmp_img);

    return 0;
}

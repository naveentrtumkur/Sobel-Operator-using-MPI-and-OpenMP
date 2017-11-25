# Use mpicc for compilation.
CC := mpicc

#flags to remove warning during compilation.
flags := -fopenmp -O3 

#header file to be included.
Head := read_bmp.h 


all: lab5p

lab5p:$(Head)
	$(CC) -o lab5p bmpReader.o tumkurrameshbabu_naveen_lab5p.c $(flags)
clean :
	rm -rf ./lab5p



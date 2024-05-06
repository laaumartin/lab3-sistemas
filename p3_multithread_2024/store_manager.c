//SSOO-P3 23/24

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <pthread.h>
#include "queue.h"
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>


//Global variables needed on producers and consumers
struct element *data; 
queue *buff;
int num; // Number of operations (first line of the input file)

typedef struct arguments{
	//fileInfo *arr;
	int first, last;
}arguments;

struct element{
  int product_id;
  char operation[20];
  int units
}

struct element{
  int product_id;
  char operation[20];
  int units
}

//Initialize mutex and condition variables
pthread_cond_t non_full; /* can we add more elements? */
pthread_cond_t non_empty; /* can we remove elements? */
pthread_mutex_t mutex;

void *producers(void *args){}

void *consumers(void *args){}


int main (int argc, const char * argv[])
{
  int profits = 0;
  int product_stock [5] = {0};

  	// Reading Input arguments
	if (argc !=5){ // check correct number of inputs
		perror("./store manager <file name><num producers><num consumers><buff size>");
		return -1;
	}

	const char *fileName = argv[1];
	int prods = atoi(argv[2]);
	int cons = atoi(argv[3]);
	int bsize = atoi(argv[4]);
		
	if(prods<=0){
		perror("Invalid number of producers");
		return -1;
	}
	
	if(cons<=0){
		perror("Invalid number of consumers");
		return -1;
	}
	if(bsize<=0){
		perror("Invalid size of buffer");
		return -1;
	}
	
	// ***************** Loading data from input file into memory ****************
	// Reading the input file
	FILE *fidin = NULL;
	
	fidin = fopen(fileName,"r"); // Open the file
	if (!fidin){
		perror("Error opening the file");
		return 1;
	}
	if(fscanf(fidin,"%d", &num)!=1){// first get number of processes
		perror("Error reading number of processes");
		return -1;
	} 
	
	
	
	data =(struct element*) malloc(num*sizeof(struct element)); // Rerserve memory for array of structure

	if(data==NULL){
		perror("Error allocating data structure");
		free(data);
		return -1;
	}

	for (int i =0; i<num; i++){ // Store the data of the file int the array
		if(fscanf(fidin, "%d %d %d", &data[i].product_id, &data[i].op, &data[i].units) ==3);
		else{
			perror("Error reading the file");
			return 1;
		}
	}
	fclose(fidin);



  // Output
  printf("Total: %d euros\n", profits);
  printf("Stock:\n");
  printf("  Product 1: %d\n", product_stock[0]);
  printf("  Product 2: %d\n", product_stock[1]);
  printf("  Product 3: %d\n", product_stock[2]);
  printf("  Product 4: %d\n", product_stock[3]);
  printf("  Product 5: %d\n", product_stock[4]);

  return 0;
}

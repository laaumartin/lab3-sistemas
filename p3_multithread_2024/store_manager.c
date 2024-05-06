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

//Initialize mutex and condition variables
pthread_cond_t non_full; /* can we add more elements? */
pthread_cond_t non_empty; /* can we remove elements? */
pthread_mutex_t mutex;

void *producers(void *args){
	arguments *sizep= (arguments *)args
	for (k=0, k<sizep->end, i++){
		pthread_mutex_lock(&mutex) // access to buffer 
		if (pthread_mutex_lock(&mutex)!=0){ 
			printf('ups there has been an error on the execution of the mutex thread')
				exit(1)
		}
		if (pthread_mutex_lock(&mutex)!=0){
			print ('there has been an error executing the mutex unlock')
			exit(2)
		while queue_full(buff){// the buffer is full so the producer has to wait until the condition of not being full.
				pthread_cond_wait(&non_full,&mutex)
				if (pthread_cond_wait!=0)
					print('there has been an error with the condition thread')
					exit(3)
		}
		if (pthread_cond_signal(&non_empty)!=0){ //signal for indicating that the buffer is not empty fails 
			print('There has been an error when producing the signal for non empty')
			exit(4)
		}
		if (queue_put(buff, &data[k])<0){ // trying to save data[k] on the buffer
		
			print('there has been an error trying to save data on the buffer')
		}
				
		}
	}
}

void *consumers(void *args){
		arguments *sizec= (arguments *)args
		int val
		struct element *elementsc
		for(i=0, i<sizec->end,i++){
			pthread_mutex_lock(&mutex)//access to buffer
			if (pthread_mutex_lock(&mutex)!=0){
				print('there has been an error with mutex lock')
				exit(1)
			}
		}
}


int main (int argc, const char * argv[])
{
	//Initialize mutex and cond_vars
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&non_full, NULL);
	pthread_cond_init(&non_empty, NULL);

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

		//Creating the consumers and producers
		pthread_t thr[prods];
		pthread_t ths[cons];
		for (i=0;i<prods;i++) {
			pthread_create(&thr[i], NULL, producers, NULL);
		}
		for (i=0;i<cons;i++) {
			pthread_create(&ths[i], NULL, consumers, NULL);
		}

		//Running the consumers and producers
		for (i=0;i<prods;i++) {
			pthread_join(thr[i],NULL);
		}
		for (i=0;i<cons;i++) {
			pthread_join(ths[i],NULL);
		}



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

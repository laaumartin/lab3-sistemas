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
#include <math.h>


//Global variables needed on producers and consumers
struct element *data; 
queue *buffer;
int num; // Number of operations (first line of the input file)
int profits;
int *product_stock;
int producWorking=1;

typedef struct arguments{
	//fileInfo *arr;
	int first, last;
}arguments;

//Initialize mutex and condition variables
pthread_cond_t non_full; /* can we add more elements? */
pthread_cond_t non_empty; /* can we remove elements? */
pthread_mutex_t mutex;


void *producers(void *args){
	arguments *argsProducer = (arguments *)args; // Casting the void pointer to arguments pointer
	for(int i=argsProducer->first; i < argsProducer->last ; i++ ) {
		
		if (pthread_mutex_lock(&mutex) != 0) {  /* access to buffer*/
            perror("There has been an error executing the mutex lock");
            exit(1);
        }
		while (queue_full(buffer)){ /* when buffer is full*/
			
			if (pthread_cond_wait(&non_full, &mutex) != 0) { 
                perror("Waiting on the condition variable has failed");
                exit(2);
            }
		}
		if (queue_put(buffer, &data[i])<0){ // trying to save data[i] on the buffer
			perror("there has been an error trying to save data on the buffer");
		}
		
		if (pthread_cond_signal(&non_empty)!=0){ /* buffer is not empty */
			perror("There has been an error when producing the signal for non empty");
			exit(3);
		}
		
		if (pthread_mutex_unlock(&mutex)!=0){ 
			printf("There has been an error executing the mutex unlock\n");
			exit(4);
		}
	}
	pthread_exit(0);
}

void *consumers(void *args) {
	arguments *argsConsumer = (arguments *)args;
	for(int i=0; i < argsConsumer->last ; i++ ){
		
		if (pthread_mutex_lock(&mutex) != 0) { // access to buffer
            perror("There has been an error executing the mutex lock");
            exit(1);
        }
		while (queue_empty(buffer)){ //buffer is empty
			
			if (pthread_cond_wait(&non_empty, &mutex) != 0) { 
                perror("Waiting on the condition variable has failed");
                exit(2);
            }
		}
		//Now we compute profit and stock
		int price;
		struct element* Actelem ;
		Actelem = queue_get(buffer);
		if(Actelem==NULL){
			perror("there has been an error trying to get data from the buffer");
			exit(3);
		}
		//let's compute the purchase cost or the sale price of the element obtained from buffer
		switch (Actelem->product_id)
		{
		case 1:
			if(Actelem->op ==0){ //purchase cost for product 1
				price=2;
			}
			else if(Actelem->op ==1){ //sale price for product 1
				price=3;
			}
			break;
		case 2:
			if(Actelem->op ==0){ //purchase cost for product 2
				price=5;
			}
			else if(Actelem->op ==1){ //sale price for product 2
				price=10;
			}
			break;
		case 3:
			if(Actelem->op ==0){ //purchase cost for product 3
				price=15;
			}
			else if(Actelem->op ==1){ //sale price for product 3
				price=20;
			}
			break;
		case 4:
			if(Actelem->op ==0){ //purchase cost for product 4
				price=25;
			}
			else if(Actelem->op ==1){ //sale price for product 4
				price=40;
			}
			break;
		case 5:
			if(Actelem->op ==0){ //purchase cost for product 5
				price=100;
			}
			else if(Actelem->op ==1){ //sale price for product 5
				price=125;
			}
			break;
		default: //case where product_id is not valid
			perror("Invalid product_id");
			break;
		}
		//Now we compute the profits and product_stock after the operation
		if(Actelem->op==0){ //Case where operation is PURCHASE
			product_stock[Actelem->product_id - 1] += Actelem->units;
			profits -= price*Actelem->units;
		}
		else if(Actelem->op==1){ //Case where operation is SALE
			if(product_stock[Actelem->product_id - 1] < Actelem->units){
				//sale is not performed as there are NOT enough stock
				printf("The product stock is less than the requested quantity.");
			}
			else{
				//sale is performed as there are enough stock
				product_stock[Actelem->product_id - 1] -= Actelem->units;
				profits += price*Actelem->units;
			}
		}
		else{ //If operation is neither PURCHASE nor SALE
			perror("Invalid operation");
		}
		
		if (pthread_cond_signal(&non_full)!=0){ //producing signal non_full
			perror("There has been an error when producing the signal for non full");
			exit(4);
		}
		
		if (pthread_mutex_unlock(&mutex)!=0){ //unlocking mutex
			printf("There has been an error executing the mutex unlock\n");
			exit(5);
		}

	}
	pthread_exit(0);
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
		int producer_num = atoi(argv[2]);
		int consumer_num = atoi(argv[3]);
		int buffer_size = atoi(argv[4]);
			
		if(producer_num<=0){
			perror("Invalid number of producers");
			return -1;
		}
		
		if(consumer_num<=0){
			perror("Invalid number of consumers");
			return -1;
		}
		if(buffer_size<=0){
			perror("Invalid size of buffer");
			return -1;
		}
		
		
		// Reading the input file
		FILE *file = NULL;
		
		file = fopen(fileName,"r"); // Open the file
		if (!file){
			perror("Error opening the file");
			return 1;
		}
		if(fscanf(file,"%d", &num)!=1){// first get number of processes
			perror("Error reading number of processes");
			return -1;
		} 
		
		
		data =(struct element*) malloc(num*sizeof(struct element)); // Rerserve memory for array of structure

		if(data==NULL){
			perror("Error allocating data structure");
			free(data);
			return -1;
		}

		for(int i=0;i<num;i++){
			
			char operation[20];

			if(fscanf(file, "%d %s %d", &data[i].product_id, operation, &data[i].units) != 3) {
				perror("Error reading the file");
				return 1;
			}
		
			if (strcmp(operation, "PURCHASE") == 0) {
				data[i].op = 0; 
			} else if (strcmp(operation, "SALE") == 0) {
				data[i].op = 1; 
			} else {
				perror("Invalid operation");
				return 1;
			}
		}
		fclose(file);

		//Creating the circular buffer
		queue* buffer = queue_init(buffer_size);
		if(buffer == NULL) { // Checking if queue initialization was successful
			printf("Queue initialization failed\n");
			return 1;
		}

		//Creating the consumers and producers
		pthread_t thrProducer[producer_num];
		pthread_t thrConsumer[consumer_num];
		arguments *argsProducer = malloc(num*sizeof(arguments));
		arguments *argsConsumer = malloc(num*sizeof(arguments));
		for (int i=0;i<producer_num;i++) {
			//here we will asign the range of data that will be processed for each producer
			if (i != producer_num - 1) {
				argsProducer[i].first = ceil(i * num / producer_num);
				argsProducer[i].last = ceil((i + 1) * num / producer_num);
			} else {
				argsProducer[i].first = ceil(i * num / producer_num);
				argsProducer[i].last = num;
			}
			pthread_create(&thrProducer[i], NULL, producers, &argsProducer[i]);
		}
		for (int i=0;i<consumer_num;i++) {
			if (i!= consumer_num-1){
				argsConsumer[i].first =0;
				argsConsumer[i].last  = ceil(num/consumer_num);
			} else{
				argsConsumer[i].first = 0;
				argsConsumer[i].last  = ceil(num/consumer_num) + num % consumer_num;
			}
			pthread_create(&thrConsumer[i], NULL, consumers, &argsConsumer[i]);
		}

		//Waiting the consumers and producers to finish execution
		for (int i=0;i<producer_num;i++) {
			pthread_join(thrProducer[i],NULL);
		}
		producWorking=0;
		pthread_cond_broadcast(&non_empty);
		for (int i=0;i<consumer_num;i++) {
			pthread_join(thrConsumer[i],NULL);
		}

		//Free the reserved memory
		free(data);
		free(argsProducer);
		pthread_mutex_destroy(&mutex);
		pthread_cond_destroy(&non_full);
		pthread_cond_destroy(&non_empty);

		//Output
		printf("Total: %d euros\n", profits);
		printf("Stock:\n");
		printf("  Product 1: %d\n", product_stock[0]);
		printf("  Product 2: %d\n", product_stock[1]);
		printf("  Product 3: %d\n", product_stock[2]);
		printf("  Product 4: %d\n", product_stock[3]);
		printf("  Product 5: %d\n", product_stock[4]);


	return 0;
	}

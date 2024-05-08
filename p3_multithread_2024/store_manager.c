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
		pthread_mutex_lock(&mutex); /* access to buffer*/
		if (pthread_mutex_lock(&mutex) != 0) { 
            perror("There has been an error executing the mutex lock");
            exit(1);
        }
		while (queue_full(buffer)){ /* when buffer is full*/
			pthread_cond_wait(&non_full, &mutex); 
			if (pthread_cond_wait(&non_full, &mutex) != 0) { 
                perror("Waiting on the condition variable has failed");
                exit(2);
            }
		}
		if (queue_put(buffer, &data[i])<0){ // trying to save data[i] on the buffer
			perror("there has been an error trying to save data on the buffer");
		}
		pthread_cond_signal(&non_empty); /* buffer is not empty */
		if (pthread_cond_signal(&non_empty)!=0){ 
			perror("There has been an error when producing the signal for non empty");
			exit(3);
		}
		pthread_mutex_unlock(&mutex);
		if (pthread_mutex_unlock(&mutex)!=0){ 
			printf("There has been an error executing the mutex unlock\n");
			exit(4);
		}
	}
	pthread_exit(0);
}

void *consumers() {
	struct element *Actelem;
	do{
		pthread_mutex_lock(&mutex); // access to buffer
		if (pthread_mutex_lock(&mutex) != 0) { 
            perror("There has been an error executing the mutex lock");
            exit(1);
        }
		while (queue_empty(buffer)){ //buffer is empty
			pthread_cond_wait(&non_empty, &mutex); 
			if (pthread_cond_wait(&non_empty, &mutex) != 0) { 
                perror("Waiting on the condition variable has failed");
                exit(2);
            }
		}
		//Now we compute profit and stock
		int price;
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
		pthread_cond_signal(&non_full); //producing signal non_full
		if (pthread_cond_signal(&non_full)!=0){ 
			perror("There has been an error when producing the signal for non full");
			exit(4);
		}
		pthread_mutex_unlock(&mutex); //unlocking mutex
		if (pthread_mutex_unlock(&mutex)!=0){ 
			printf("There has been an error executing the mutex unlock\n");
			exit(5);
		}

	}while(queue_empty(buffer)==0); //the process will be repeated until the buffer is empty and producers has not more operations to add
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

		for(int i=0;i<num;i++){
			
			char operation[20];

			if(fscanf(fidin, "%d %s %d", &data[i].product_id, operation, &data[i].units) != 3) {
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
		fclose(fidin);

		//Creating the circular buffer
		queue* buffer = queue_init(bsize);
		if(buffer == NULL) { // Checking if queue initialization was successful
			printf("Queue initialization failed\n");
			return 1;
		}

		//Creating the consumers and producers
		pthread_t thrProducer[prods];
		pthread_t thrConsumer[cons];
		arguments *argsProducer = malloc(num*sizeof(arguments));
		for (int i=0;i<prods;i++) {
			//here we will asign the range of data that will be processed for each producer
			if (i != prods - 1) {
				argsProducer[i].first = ceil(i * num / prods);
				argsProducer[i].last = ceil((i + 1) * num / prods);
			} else {
				argsProducer[i].first = ceil(i * num / prods);
				argsProducer[i].last = num;
			}
			pthread_create(&thrProducer[i], NULL, producers, &argsProducer[i]);
		}
		for (int i=0;i<cons;i++) {
			pthread_create(&thrConsumer[i], NULL, consumers, NULL);
		}

		//Running the consumers and producers
		for (int i=0;i<prods;i++) {
			pthread_join(thrProducer[i],NULL);
		}


		for (int i=0;i<cons;i++) {
			pthread_join(thrConsumer[i],NULL);
		}


		//Output
		printf("Total: %d euros\n", profits);
		printf("Stock:\n");
		printf("  Product 1: %d\n", product_stock[0]);
		printf("  Product 2: %d\n", product_stock[1]);
		printf("  Product 3: %d\n", product_stock[2]);
		printf("  Product 4: %d\n", product_stock[3]);
		printf("  Product 5: %d\n", product_stock[4]);

		//Free the reserved memory
		free(data);
		free(argsProducer);

	return 0;
	}

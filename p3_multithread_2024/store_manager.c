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
int numOperations; // Number of operations (first line of the input file)
int count=0;
int producWorking=1;

typedef struct arguments{
	int first, last;
}arguments;

typedef struct result{
	int partial_profit;
	int product_stock[5];
}result;

//Initialize mutex and condition variables
pthread_cond_t non_full; /* can we add more elements? */
pthread_cond_t non_empty; /* can we remove elements? */
pthread_mutex_t mutex;


void *producers(void *args){
	arguments *argsProducer = (arguments *)args; // Casting the void pointer to arguments pointer
	for(int i=argsProducer->first; i < argsProducer->last ; i++ ) {
		/* access to buffer*/
		if (pthread_mutex_lock(&mutex) != 0) { 
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
		/* buffer is not empty */
		if (pthread_cond_signal(&non_empty)!=0){ 
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

void *consumers() {
	struct element *Actelem;
	struct result *result = malloc(sizeof(struct result));
	result->partial_profit = 0;
	memcpy(result->product_stock, (int[5]){0},sizeof(result->product_stock));
	while(queue_empty(buffer)==0 || producWorking==1){
		 // access to buffer
		if (pthread_mutex_lock(&mutex) != 0) { 
            perror("There has been an error executing the mutex lock");
            exit(1);
        }
		while (queue_empty(buffer) && producWorking==1 ){ //buffer is empty
			if (pthread_cond_wait(&non_empty, &mutex) != 0) { 
                perror("Waiting on the condition variable has failed");
                exit(2);
            }
		}
		//Now we compute profit and stock
		if(queue_empty(buffer)==0){
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
				result->product_stock[Actelem->product_id - 1] += Actelem->units;
				result->partial_profit -= price*Actelem->units;
			}
			else if(Actelem->op==1){ //Case where operation is SALE
				if(result->product_stock[Actelem->product_id - 1] < Actelem->units){
					//sale is not performed as there are NOT enough stock
					printf("The product stock is less than the requested quantity.");
				}
				else{
					//sale is performed as there are enough stock
					result->product_stock[Actelem->product_id - 1] -= Actelem->units;
					result->partial_profit += price*Actelem->units;
				}
			}
			else{ //If operation is neither PURCHASE nor SALE
				perror("Invalid operation");
			}
			free(Actelem);
			count ++;
			if (count>=numOperations){
				producWorking=0;
				pthread_cond_broadcast(&non_empty);
				pthread_mutex_unlock(&mutex);
				break;
			}
			//producing signal non_full
			if (pthread_cond_signal(&non_full)!=0){ 
				perror("There has been an error when producing the signal for non full");
				exit(4);
			}
			//unlocking mutex
			if (pthread_mutex_unlock(&mutex)!=0){ 
				printf("There has been an error executing the mutex unlock\n");
				exit(5);
			}
		} else{
			pthread_mutex_unlock(&mutex);
			break;
		}
	}
	pthread_exit(result);
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
		int numP = atoi(argv[2]);
		int numC = atoi(argv[3]);
		int buffsize = atoi(argv[4]);
			
		if(numP<=0){
			perror("Invalid number of producers");
			return -1;
		}
		
		if(numC<=0){
			perror("Invalid number of consumers");
			return -1;
		}
		if(buffsize<=0){
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
		if(fscanf(fidin,"%d", &numOperations)!=1){// first get number of operations
			perror("Error reading number of operations");
			return -1;
		} 
		
		
		data =(struct element*) malloc(numOperations*sizeof(struct element)); // Rerserve memory for array of structure

		if(data==NULL){
			perror("Error allocating data structure");
			free(data);
			return -1;
		}

		for(int i=0;i<numOperations;i++){
			
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
		queue* buffer = queue_init(buffsize);
		if(buffer == NULL) { // Checking if queue initialization was successful
			printf("Queue initialization failed\n");
			return 1;
		}

		//Creating the consumers and producers
		pthread_t thrProducer[numP];
		pthread_t thrConsumer[numC];
		arguments *argsProducer = malloc(numOperations*sizeof(arguments));
		for (int i=0;i<numP;i++) {
			//here we will asign the range of data that will be processed for each producer
			if (i != numP - 1) {
				argsProducer[i].first = ceil(i * (numOperations-1) / numP);
				argsProducer[i].last = ceil((i + 1) * (numOperations-1) / numP);
			} else {
				argsProducer[i].first = ceil(i * (numOperations-1) / numP);
				argsProducer[i].last = numOperations-1;
			}
			pthread_create(&thrProducer[i], NULL, producers, &argsProducer[i]);
		}
		for (int i=0;i<numC;i++) {
			pthread_create(&thrConsumer[i], NULL, consumers, NULL);
		}

		//Waiting the consumers and producers to finish execution
		for (int i=0;i<numP;i++) {
			pthread_join(thrProducer[i],NULL);
		}

		struct result **result = (struct result **)malloc(numC * sizeof(struct result));
		for (int i=0;i<numC;i++) {
			pthread_join(thrConsumer[i],(void **)&result[i]);
			profits += result[i]->partial_profit;
			for(int j=0;j<5;j++){
				product_stock[j] += result[i]->product_stock[j];
			}
		}
		//Free reserved data
		free(data);
		free(argsProducer);
		free(result);
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

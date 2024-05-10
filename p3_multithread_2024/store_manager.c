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
	int partial_profit; // variable to hold the profit value
	int product_stock[5]; // stock of the five different products
}result;

//Initialize mutex and condition variables
pthread_cond_t non_full; /* can we add more elements? */
pthread_cond_t non_empty; /* can we remove elements? */
pthread_mutex_t mutex;


void *producers(void *args){ // producers function
	arguments *argsProducer = (arguments *)args; // Casting the void pointer to arguments pointer
	for(int i=argsProducer->first; i < argsProducer->last ; i++ ) {
		/* access to buffer*/
		if (pthread_mutex_lock(&mutex) != 0) { // lock the mutex and check if it has been locked correctly
            perror("There has been an error executing the mutex lock");// message in case of error
            exit(1);
        }
		while (queue_full(buffer)){ /* when buffer is full*/ 
			if (pthread_cond_wait(&non_full, &mutex) != 0) { // wait until the signal for the condition variable
                perror("Waiting on the condition variable has failed");// in case of error message 
                exit(2);
            }
		}
		if (queue_put(buffer, &data[i])<0){ // trying to save data[i] on the buffer
			perror("there has been an error trying to save data on the buffer");// message in case of error
		}
		
		if (pthread_cond_signal(&non_empty)!=0){ /* buffer is not empty */
			perror("There has been an error when producing the signal for non empty");// message in case of error
			exit(3);
		}
		if (pthread_mutex_unlock(&mutex)!=0){  // unlocks the mutex and checks if it is done correctly
			printf("There has been an error executing the mutex unlock\n"); //message in case of error
			exit(4);
		}
	}
	pthread_exit(0);
}

void *consumers() { //consumers function
	struct element *Actelem;
	struct result *result = malloc(sizeof(struct result)); // allocating memory for the structure result
	result->partial_profit = 0;//sets the partial profit member of the result to 0
	memcpy(result->product_stock, (int[5]){0},sizeof(result->product_stock)); //copy the integers array into the product stock array 
	while(queue_empty(buffer)==0 || producWorking==1){ // while the buffer is not empty or  if the producer is still working
		
		if (pthread_mutex_lock(&mutex) != 0) {  // access to buffer
            perror("There has been an error executing the mutex lock");// message in case of error
            exit(1);
        }
		while (queue_empty(buffer) && producWorking==1 ){ //buffer is empty
			if (pthread_cond_wait(&non_empty, &mutex) != 0) { 
                perror("Waiting on the condition variable has failed");// message in case of error
                exit(2);
            }
		}
		//Now we compute profit and stock
		if(queue_empty(buffer)==0){ // the buffer is not empty 
			int price; //initialize the price
			Actelem = queue_get(buffer); // dequeue an element from the buffer
			if(Actelem==NULL){ // no element is dequeue
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
			count ++; // increasing the counter 
			if (count>=numOperations){ // in case the counter is greater than the numb of operations 
				producWorking=0; // producers stop working
				pthread_cond_broadcast(&non_empty); // creat a conditional broadcast
				pthread_mutex_unlock(&mutex); // unlock the mutex
				break;
			}
			//producing signal non_full
			if (pthread_cond_signal(&non_full)!=0){ 
				perror("There has been an error when producing the signal for non full"); //message in case of error
				exit(4);
			}
			//unlocking mutex
			if (pthread_mutex_unlock(&mutex)!=0){ 
				printf("There has been an error executing the mutex unlock\n");//message in case of error
				exit(5);
			}
		} else{
			pthread_mutex_unlock(&mutex); // unlock the mutex
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

  	int profits = 0; //initialize the profit
	int product_stock [5] = {0}; //intialize the product stock

		// Reading Input arguments
		if (argc !=5){ // check correct number of inputs
			perror("./store manager <file name><num producers><num consumers><buff size>");// message indicating how it should be 
			return -1;
		}

		const char *fileName = argv[1]; // first argument is the file name
		int numP = atoi(argv[2]);// second argument is the number of producers
		int numC = atoi(argv[3]);// third argument is the number of consumers
		int buffsize = atoi(argv[4]); // fourth argument is the buffer size
		// message errors in case of any of the argumnents is an integer smaller or equal to 0
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
		if (!fidin){ // failure when opening
			perror("Error opening the file");
			return 1;
		}
		if(fscanf(fidin,"%d", &numOperations)!=1){// first get number of operations
			perror("Error reading number of operations");
			return -1;
		} 
		
		
		data =(struct element*) malloc(numOperations*sizeof(struct element)); // Rerserve memory for array of structure

		if(data==NULL){ // in case of error allocating the data structure
			perror("Error allocating data structure");
			free(data);
			return -1;
		}

		for(int i=0;i<numOperations;i++){ // loop for the number of operations
			 
			char operation[20];

			if(fscanf(fidin, "%d %s %d", &data[i].product_id, operation, &data[i].units) != 3) { // reading the data from the file
				perror("Error reading the file"); // in case of error
				return 1;
			}
		
			if (strcmp(operation, "PURCHASE") == 0) {  //check if the string is the op purchase
				data[i].op = 0; //setting the data to 0 in purchase case
			} else if (strcmp(operation, "SALE") == 0) {//check if the string is the op sale
				data[i].op = 1; //setting the data to 1 in sales case
			} else {
				perror("Invalid operation");// error case
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

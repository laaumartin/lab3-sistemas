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

typedef struct arguments{
	//fileInfo *arr;
	int first, last;
}arguments;

//Initialize mutex and condition variables
pthread_cond_t non_full; /* can we add more elements? */
pthread_cond_t non_empty; /* can we remove elements? */
pthread_mutex_t mutex;


void producers(void *args){
	arguments *ourargs = (arguments *)args; // Casting the void pointer to arguments pointer
	for(int i=ourargs->first; i < ourargs->last ; i++ ) {
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

void *consumers(void *args) {
    arguments *conArgs = (arguments *)args;
    struct element *elem;
    int profit = 0;
    int product_stock[5] = {0}; // Inicializar el stock de productos

    // (a) Obtener concurrentemente los elementos insertados en la cola
    for (int i = conArgs->first; i < conArgs->last; i++) {
        if (pthread_mutex_lock(&mutex) != 0) { // Acceso al buffer
            perror("Error en la ejecución de pthread_mutex_lock()");
            exit(1);
        }
        while (queue_empty(buffer)) { // Cuando el buffer está vacío, esperar a que se llene
            if (pthread_cond_wait(&non_empty, &mutex) != 0) {
                perror("Error en la ejecución de pthread_cond_wait()");
                exit(4);
            }
        }

        elem = queue_get(buffer); // Extraer el elemento de la cola

        if (elem == NULL) {
            perror("Error extrayendo el elemento de la cola");
        }

        // (b) Procesar la transacción (compra/venta) y calcular el beneficio
        if (elem->op == 1) { // Compra
            product_stock[elem->product_id - 1] += elem->units; // Incrementar el stock
            profit -= elem->units; // Disminuir el beneficio
        } else { // Venta
            // Verificar si hay suficientes productos para vender
            if (product_stock[elem->product_id - 1] >= elem->units) {
                product_stock[elem->product_id - 1] -= elem->units; // Disminuir el stock
                profit += elem->units; // Incrementar el beneficio
            } else {
                // No hay suficientes productos para vender, descartar la transacción
                printf("Advertencia: Stock insuficiente para la transacción: %d %d %d\n",
                       elem->product_id, elem->op, elem->units);
            }
        }

        if (pthread_cond_signal(&non_full) != 0) { // El buffer no está lleno
            perror("Error en la ejecución de pthread_cond_signal()");
            exit(3);
        }

        if (pthread_mutex_unlock(&mutex) != 0) {
            perror("Error en la ejecución de pthread_mutex_unlock()");
            exit(2);
        }
    } // Fin del bucle for

    // (c) Devolver el beneficio calculado y el stock parcial al proceso principal
    int *result = malloc(sizeof(int) * 6); // Asignar memoria para el beneficio y el array de stock
    result[0] = profit; // Almacenar el beneficio en el primer índice
    // Almacenar el stock de productos en los índices restantes
    for (int i = 0; i < 5; i++) {
        result[i + 1] = product_stock[i];
    }
    
    pthread_exit(result);
    return NULL;
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
			pthread_create(&thrProducer[i], NULL, (void *)producers, &argsProducer[i]);
		}
		for (int i=0;i<cons;i++) {
			pthread_create(&thrConsumer[i], NULL, &consumers, NULL);
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

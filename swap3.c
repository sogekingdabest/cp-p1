#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "options.h"

struct buffer {
	int *data;
	int size;
};

struct thread_info {
	pthread_t       thread_id;        // id returned by pthread_create()
	int             thread_num;       // application defined thread #
};

struct args {
	int 		thread_num;       // application defined thread #
	int 	        delay;			  // delay between operations
	int		*iteraciones;
	struct buffer   *buffer;		  // Shared buffer
	pthread_mutex_t *buffer_lmutex; 
    	pthread_mutex_t *iteracion_mutex;
		
};

void *swap(void *ptr)
{
	struct args *args =  ptr;
	int i,j, tmp, parejaBloqueada;
	while(1) {
		
		pthread_mutex_lock(args->iteracion_mutex);
		if(*args->iteraciones <= 0){
		    pthread_mutex_unlock(args->iteracion_mutex);
		    break;
		}
		 
		(*(args->iteraciones))--;             
		pthread_mutex_unlock(args->iteracion_mutex);
	
		do{
		i=rand() % args->buffer->size;
		j=rand() % args->buffer->size;
		} while(i==j);
		
		do{

			pthread_mutex_lock(&(args->buffer_lmutex[i]));

			if(pthread_mutex_trylock(&(args->buffer_lmutex[j]))){

                		pthread_mutex_unlock(&(args->buffer_lmutex[i]));
                		parejaBloqueada = 0;

            		}else{
                
                		parejaBloqueada = 1;

            		}
        	}while(!parejaBloqueada);

		printf("Thread %d swapping positions %d (== %d) and %d (== %d)\n", 
			args->thread_num, i, args->buffer->data[i], j, args->buffer->data[j]);
				
		tmp = args->buffer->data[i];
		if(args->delay) usleep(args->delay); // Force a context switch

		args->buffer->data[i] = args->buffer->data[j];
		if(args->delay) usleep(args->delay);
		
		args->buffer->data[j] = tmp;
		if(args->delay) usleep(args->delay);

		pthread_mutex_unlock(&(args->buffer_lmutex[i]));
		pthread_mutex_unlock(&(args->buffer_lmutex[j]));

	}
	return NULL;
}

void print_buffer(struct buffer buffer) {
	int i;
	
	for (i = 0; i < buffer.size; i++)
		printf("%i ", buffer.data[i]);
	printf("\n");
}

void start_threads(struct options opt)
{
	int i;
	struct thread_info *threads;
	struct args *args;
	struct buffer buffer;
	pthread_mutex_t *lmutex;
	pthread_mutex_t iteracion_mutex;
	
	srand(time(NULL));
	
	if((buffer.data=malloc(opt.buffer_size*sizeof(int)))==NULL) {
		printf("Out of memory\n");
		exit(1);
	}
	buffer.size = opt.buffer_size;	

	printf("creating %d threads\n", opt.num_threads);
	threads = malloc(sizeof(struct thread_info) * opt.num_threads);
	args = malloc(sizeof(struct args) * opt.num_threads);
	lmutex = malloc(opt.buffer_size * sizeof(pthread_mutex_t));

	if (threads == NULL || args==NULL || lmutex==NULL) {
		printf("Not enough memory\n");
		exit(1);
	}

	for(i=0; i<buffer.size; i++){
        	buffer.data[i]=i;
        	if(pthread_mutex_init(&lmutex[i],NULL)){
            		printf("No se pudo inicializar el mutex %d\n", i);
            		exit(1);            
        	}
    	}
	
        if(pthread_mutex_init(&iteracion_mutex,NULL)){
    		printf("No se pudo inicializar el mutex iterations\n");
		exit(1);            
        }
	
	printf("Buffer before: ");
	print_buffer(buffer);

	// Create num_thread threads running swap() 
	for (i = 0; i < opt.num_threads; i++) {
		threads[i].thread_num = i;
		
		args[i].thread_num = i;
		args[i].buffer     = &buffer;
		args[i].delay      = opt.delay;
		args[i].iteraciones = &opt.iterations;
		args[i].buffer_lmutex      = lmutex;
		args[i].iteracion_mutex = &iteracion_mutex;

		if ( 0 != pthread_create(&threads[i].thread_id, NULL,
					 swap, &args[i])) {
			printf("Could not create thread #%d", i);
			exit(1);
		}
	}
	
	// Wait for the threads to finish
	for (i = 0; i < opt.num_threads; i++)
		pthread_join(threads[i].thread_id, NULL);

	// Print the buffer
	printf("Buffer after:  ");
	print_buffer(buffer);
	
	for(i=0; i<buffer.size; i++){
        	pthread_mutex_destroy(&lmutex[i]);
    	}

	pthread_mutex_destroy(&iteracion_mutex);

	free(args);
	free(threads);
	free(buffer.data);
        free(lmutex);

	pthread_exit(NULL);
}

int main (int argc, char **argv)
{
	struct options opt;
	
	// Default values for the options
	opt.num_threads = 10;
	opt.buffer_size = 10;
	opt.iterations  = 100;
	opt.delay       = 10;
	
	read_options(argc, argv, &opt);

	start_threads(opt);

	exit (0);
}

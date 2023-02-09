#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

//#include <linux/delay.h>

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
        struct thread_data* thread_func_args = (struct thread_data *)thread_param;
	
	//wait 
	int wait_to_obtain = thread_func_args -> wait_to_obtain_ms;
	usleep(wait_to_obtain * 1000);
	
	//obtain mutex
	int ret = pthread_mutex_lock(&mtx);
 	if(ret != 0)
 	{
		perror("pthread_mutex_lock");
		printf("pthread_mutex_lock");
		thread_func_args -> thread_complete_success = false;
		thread_param = (void*) thread_func_args;
   	        return thread_param;
	}
	//wait
	int wait_to_release = thread_func_args -> wait_to_release_ms;
 	usleep(wait_to_release * 1000);

	//release mutex 
	ret = pthread_mutex_unlock(&mtx);
	if(ret != 0)
	{
		perror("pthread_mutex_unlock"); 
		printf("pthread_mutex_unlock"); 	
		thread_func_args -> thread_complete_success = false;
		thread_param = (void*) thread_func_args;
   	        return thread_param;
		
	}

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
        thread_func_args -> thread_complete_success = false;
	thread_param = (void*) thread_func_args;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{	
	//Malloc and malloc check	
	struct thread_data *thread_args = (struct thread_data*)malloc(sizeof(struct thread_data)); 
	if(thread_args == NULL)
	{
		perror("Error crating thread argument structure");
		ERROR_LOG("Error crating thread argument structure");
		return false;
	}	
	
	//Copying the arguments
	thread_args -> wait_to_obtain_ms = wait_to_obtain_ms;
	thread_args ->  wait_to_release_ms = wait_to_release_ms;
	thread_args -> mutex = mutex;
	thread_args -> thread_complete_success = false;

	pthread_t p1;
	int s = pthread_create(&p1, NULL, threadfunc,(void *)&thread_args);
	if(s != 0)
	{
		perror("Error Creating a pthread");
		printf("Error Creating a pthread");
		return false;	
	}

	if(!(thread_args -> thread_complete_success))
		return false;

	free(thread_args);
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    return true;
}


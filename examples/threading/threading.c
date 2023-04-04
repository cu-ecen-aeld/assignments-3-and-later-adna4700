#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>


// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
        struct thread_data* thread_func_args = (struct thread_data *)thread_param;
	
	if(thread_func_args -> mutex == NULL)
	{
		perror("MUtex is NULL");
		ERROR_LOG("Mutex is NULL");
		thread_func_args -> thread_complete_success = false;
	
	}

	//wait
	usleep(thread_func_args -> wait_to_obtain_ms * 1000);
	
	//obtain mutex
	int ret = pthread_mutex_lock(thread_func_args->mutex);
 	if(ret != 0)
 	{
		perror("pthread_mutex_lock");
		ERROR_LOG("pthread_mutex_lock");
		thread_func_args -> thread_complete_success = false;
	}
	//wait
	usleep(thread_func_args -> wait_to_release_ms * 1000);
	

	//release mutex 
	ret = pthread_mutex_unlock(thread_func_args->mutex);
	if(ret != 0)
	{
		perror("pthread_mutex_unlock"); 
		ERROR_LOG("pthread_mutex_unlock"); 	
		thread_func_args -> thread_complete_success = false;
		
	}

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
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
	thread_args -> thread_complete_success = true;
	
	//crerating the thread
	int s = pthread_create(thread, NULL, threadfunc,(void *)thread_args);
	if(s != 0)
	{
		perror("Error Creating a pthread");
		ERROR_LOG("Error Creating a pthread");
		return false;	
	}


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


/*
References: https://man7.org/linux/man-pages/man2/getsockname.2.html
https://beej.us/guide/bgnet/html/#a-simple-stream-server
https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/
https://nullraum.net/how-to-create-a-daemon-in-c/
https://stackoverflow.com/questions/14846768/in-c-how-do-i-redirect-stdout-fileno-to-dev-null-using-dup2-and-then-redirect
https://man7.org/linux/man-pages/man7/queue.7.html
https://www.geeksforgeeks.org/handling-multiple-clients-on-server-with-multithreading-using-socket-programming-in-c-cpp/
https://man7.org/linux/man-pages/man3/slist.3.html
https://pubs.opengroup.org/onlinepubs/7908799/xsh/time.h.html
https://linux.die.net/man/3/clock_gettime
https://man7.org/linux/man-pages/man3/strftime.3.html
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <syslog.h> //to use syslog
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <syslog.h>
#include <fcntl.h>
#include <pthread.h>   //for multithreading
#include <sys/queue.h> //for linked list
#include <time.h>
#include <sys/time.h>
#include <poll.h>

#define PORT 9000
#define BACKLOG 5
#define MAX_SIZE 1024
#define FILE_PATH "/var/tmp/aesdsocketdata"
//server socket fd always non negative so initialized to a negative integer
int sock_fd = -1;
//FILE pointer
FILE *fp;
// Daemon Check flag initialized to false
bool is_daemon = false;

void print_buf(char *buffer, int buffer_len);
void graceful_exit();
//global mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // need  mutex to avoid data corruption
bool signal_caught = false;               // initializing to false to indicate there is no SIGTERM or SIGINT caught

// creatinf a structure to be protected from corruption == client data
typedef struct client_data
{
    pthread_t thread_id; // each thread for client will have a unique thread id
    // client fd and address
    int client_fd;
    struct sockaddr_in address;
    // thread completion flag
    bool thread_complete;
    // the mutex for protection
    pthread_mutex_t *mutex;
} client_thread;

// linklist will have above struct as a member
typedef struct linked_list_node
{
    client_thread client_parameters;
    // SLIST_ENTRY(TYPE); Here TYPE == client_thread
    // SLIST_ENTRY() declares a structure that connects the elements in the list.
    SLIST_ENTRY(linked_list_node)
    entry;
} node;

void print_buf(char *buffer, int buffer_len)
{
    for (int i = 0; i < buffer_len; i++)
    {
        printf("%x,", buffer[i]);
    }
    printf("The length of buff: %d\r\n", buffer_len);
}

void signal_handler(int signal)
{
    if ((signal == SIGINT) || (signal == SIGTERM))
    {
        //to indicate that we caught the signal
        signal_caught = true;
        printf("Signal Caught is %d, exiting\n", signal);
        graceful_exit();
        exit(0);
    }
}

void graceful_exit()
{

    // Closing the server socket
    if (sock_fd > -1)
    {
       shutdown(sock_fd, SHUT_RDWR);
        close(sock_fd);
        // Closing both recv and send on the server socket    
    }
    if (fp != NULL)
    {
        fclose(fp);
    }
    remove(FILE_PATH);
    // Closing syslog
    pthread_mutex_destroy(&mutex);
    closelog();
}

int making_daemon()
{
    // Making a process daemon
    // Steps: fork --> terminate parent --> setsid -->  chdir --> redirect files to /dev/null

    // Forking
    int fork_pid = fork();
    if (fork_pid < 0)
    {
        printf("Forking failed");
        syslog(LOG_ERR, "Forking failed. Error: %d\r\n", errno);
        // Should return if tried to run as daemon and fails
        return -1;
    }
    else if (fork_pid > 0)
    {
        // Terminate the parent process as fork was successfull
        exit(0);
    }

    // setsid
    // Makes Child process the session leader
    if (setsid() < 0)
    {
        printf("Setting Session ID failed");
        syslog(LOG_ERR, "Setting Session ID failed Error: %d\r\n", errno);
        // Should return -1
        return -1;
    }

    // chdir Switching to the root directory as it will be a background process now
    if (chdir("/") == -1)
    {
        printf("Changing directory failed");
        syslog(LOG_ERR, "Changing directory ID failed Error: %d\r\n", errno);
        // Should return -1
        return -1;
    }
    // Redirect output to /dev/null
    int devNull = open("/dev/null", O_RDWR);
    if (devNull == -1)
    {
        printf("Error in opening /dev/null");
        syslog(LOG_ERR, "Error in opening /dev/null. Error: %d\r\n", errno);
        // Should return -1
        return -1;
    }

    // redirect stdout and stderror
    int dup2stdout = dup2(devNull, STDOUT_FILENO);
    if (dup2stdout == -1)
    {
        printf("Error in dup2(devNull, STDOUT_FILENO)\n");
        syslog(LOG_ERR, "Error in dup2(devNull, STDOUT_FILENO). Error: %d\r\n", errno);
        // Should return -1
        return -1;
    }
    int dup2stderr = dup2(devNull, STDERR_FILENO);
    if (dup2stderr == -1)
    {
        printf("Error in dup2(devNull, STDERR_FILENO)\n");
        syslog(LOG_ERR, "Error in dup2(devNull, STDERR_FILENO). Error: %d\r\n", errno);
        // Should return -1
        return -1;
    }
    int dup2stdin = dup2(devNull, STDIN_FILENO);
    if (dup2stdin == -1)
    {
        printf("Error in dup2(devNull, STDIN_FILENO)\n");
        syslog(LOG_ERR, "Error in dup2(devNull, STDIN_FILENO). Error: %d\r\n", errno);
        // Should return -1
        return -1;
    }

    return 0;
}

void *thread_routine(void *client_parameters)
{
   // does that read write via file
    int len_tracker, buffer_len, index, mutex_lock;

    client_thread *parameters = (client_thread *)client_parameters;

    // Initial malloc
    char *buffer = (char *)malloc(MAX_SIZE);
    len_tracker = MAX_SIZE;
    if (buffer == NULL)
    {
        printf("Error in Malloc\r\n");
        // Log this info
        syslog(LOG_INFO, "Error in Malloc\r\n");
        // thread_complete true
        parameters->thread_complete = true;
        graceful_exit();
        close(parameters->client_fd);
        pthread_exit(NULL);
    }
    // memset the buffer so it has all zeros
    memset(buffer, 0, MAX_SIZE);

    while (parameters->thread_complete == false)
    {
        // reads from the client, store it in a buffer
        buffer_len = read(parameters->client_fd, buffer + len_tracker - MAX_SIZE, MAX_SIZE);
        // error in reading
        if (buffer_len == -1)
        {
            syslog(LOG_ERR, "Error reading from the socket. Error: %d \r\n", errno);
            parameters->thread_complete = true;
            if(buffer != NULL)
            {
              free(buffer);
            }
            buffer = NULL;
            graceful_exit();
            close(parameters->client_fd);
            pthread_exit(NULL);
        }
        printf("Can read from the client\r\n");
        printf(" read from client buf length is %d\r\n", buffer_len);

        if (buffer_len == 0)
        {
            printf("Disconnected from client\n");
            break;
        }

        // stop with newline character
        for (index = 0; index < buffer_len; index++)
        {
            if (buffer[index + len_tracker - MAX_SIZE] == '\n')
            {
                printf("New line detected\r\n");
                break;
            }
        }

        // if(buffer_len < MAX_SIZE)
        if (index < buffer_len)
        {
            // can write to file directly
            // fwrite(buffer, buffer_len, 1, fp);
            fseek(fp, 0, SEEK_END);
            mutex_lock = pthread_mutex_lock(parameters->mutex);
            if (mutex_lock != 0)
            {
                printf("Error in acquring the mutex\r\n");
                // Log this info
                syslog(LOG_INFO, "Error in acquring the mutex. Error: %d\r\n", errno);
                if(buffer)
                {
                free(buffer);
                }
                buffer = NULL;
                graceful_exit();
                close(parameters->client_fd);
                pthread_exit(NULL);
            }
            printf("The mutex is locked\r\n");

            if (fwrite(buffer, len_tracker - MAX_SIZE + index + 1, 1, fp) == -1)
            {
                printf("Error in writing to the file\r\n");
                // Log this info
                syslog(LOG_INFO, "Error in writing to the file. Error: %d\r\n", errno);
                if(buffer != NULL)
                {
                free(buffer);
                }
                buffer = NULL;
                graceful_exit();
                close(parameters->client_fd);
                pthread_exit(NULL);
            }

                printf("buf from fwrite to a file from buffer: ");
                for (int i = 0; i < (len_tracker - MAX_SIZE + index + 1); i++)
                    printf("%c", buffer[i]);
                 
        }
        else
        {
                printf("Incrementing the buffer\r\n");
                len_tracker += MAX_SIZE;
                buffer = (char *)realloc(buffer, len_tracker);
                if (buffer == NULL)
                {
                printf("Error in Malloc\r\n");
                // Log this info
                syslog(LOG_INFO, "Error in Malloc\r\n");
                // thread_complete true as write is complete now
                parameters->thread_complete = true;
                }
                printf("Incremented the buffer\r\n");
        }
        printf("Before fseek\r\n");
        fseek(fp, 0, SEEK_SET);
        printf("After fseek\r\n");

        mutex_lock = pthread_mutex_unlock(parameters->mutex);
        if (mutex_lock != 0)
        {
            printf("Error in releasing the mutex\r\n");
            // Log this info
            syslog(LOG_INFO, "Error in releasing the mutex at 322 line. Error: %d\r\n", errno);
            if(buffer != NULL)
            {
            free(buffer);
            }
            buffer = NULL;
            parameters->thread_complete = true;
            graceful_exit();
            close(parameters->client_fd);
            pthread_exit(NULL);
        }
        printf("mutex unlocked\r\n");
        //keep it open
        //fclose(fp);
        /************/
       // FILE *file_fp = fopen(FILE_PATH, "w+");
       /*
        if (file_fp == NULL)
        {
            printf("Error opening file\r\n");
            // syslog the errors
            syslog(LOG_ERR, "Error opening the file. Error code:%d\r\n", errno);
            if(buffer)
            {
            free(buffer);
            }
            buffer = NULL;
            parameters->thread_complete = true;
            graceful_exit();
            close(parameters->client_fd);
            pthread_exit(NULL);
        }
        */

        len_tracker = 0;
        if(buffer != NULL)
        {
        free(buffer);
        }
        buffer = NULL;
        int file_bytes = 0;
        char ch;
        fseek(fp, 0, SEEK_SET);
        while (fread(&ch, 1, 1, fp) > 0)
        {
            file_bytes++;
            
        }
        printf(" fbytes is %d \r\n", file_bytes);

        char *write_buffer = (char *)malloc(file_bytes);
        if (write_buffer == NULL)
            {
                printf("Error in Malloc\r\n");
                // Log this info
                syslog(LOG_INFO, "Error in Malloc\r\n");
                // thread_complete true
                parameters->thread_complete = true;
                graceful_exit();
                close(parameters->client_fd);
                pthread_exit(NULL);
            }
            printf("Created a write buffer\r\n");

        fseek(fp, 0, SEEK_SET);
        if (fread(write_buffer, file_bytes, 1, fp) < 0)
        {
            printf("Error in reading file\r\n");
            // Log this info
            syslog(LOG_INFO, "Error in reading file. Error: %d\r\n", errno);
            parameters->thread_complete = true;
            if(write_buffer)
            {
            free(write_buffer);
            }
            write_buffer = NULL;
            graceful_exit();
            close(parameters->client_fd);
            pthread_exit(NULL);
        }
        printf("buf: ");
        for (int i = 0; i < (file_bytes); i++)
            printf("%c", write_buffer[i]);

        if (write(parameters->client_fd, write_buffer, file_bytes) == -1)
        {
            printf("Error in writing to transmit buffer\r\n");
            // Log this info
            syslog(LOG_INFO, "Error in writing to transmit buffer. Error: %d\r\n", errno);
            break;
        }

        if(write_buffer != NULL)
        {
        free(write_buffer);
        }
        write_buffer = NULL;
        parameters->thread_complete = true;
        close(parameters->client_fd);
        pthread_exit(NULL);      
    }
    return NULL;
}

void* timer_routine(void* mutex_t)
{
   //parameters to accesss
   pthread_mutex_t *mutex = NULL;
   mutex = (pthread_mutex_t*)mutex_t; 
   
    //Initializing the timespec strcut to zero
    /*
    struct timespec {
        time_t   tv_sec;        
        long     tv_nsec;      
    };*/
    
    // struct timespec tp = {0,0};
    /*The <time.h> header declares the structure tm, which includes at least the following members:

    int    tm_sec   seconds [0,61]
    int    tm_min   minutes [0,59]
    int    tm_hour  hour [0,23]
    int    tm_mday  day of month [1,31]
    int    tm_mon   month of year [0,11]
    int    tm_year  years since 1900
    int    tm_wday  day of week [0,6] (Sunday = 0)
    int    tm_yday  day of year [0,365]
    int    tm_isdst daylight savings flag*/
    struct tm *time_local;

    //the count will be 10 as it will be ten sec delay
    // int ten_sec = 10;
    time_t the_time;
    
    // while(1)
    // {
    // //get the status ogf the time
    // if(clock_gettime(CLOCK_MONOTONIC, &tp) == -1)
    // {
    //     // printintg and syslogging the errors
        
    //     printf("Error in clock_gettime()\r\n");
    //     syslog (LOG_ERR, "Error in clock_gettime()\r\n");
    //     // graceful_exit();
    //     continue;
    // }
    while(1)
    {
    sleep(10);

    // //generates 10 sec delay
    // while(ten_sec > 0)
    // {
    //     ten_sec--;
    //     tp.tv_sec = tp.tv_sec + 1;
    //     tp.tv_nsec = tp.tv_nsec + 1000000000;
        

    //     if(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tp, NULL) > 0)
    //     {
    //         // printintg and syslogging the errors
    //         printf("Error in clock_nanosleep()\r\n");
    //         perror("clock_nanosleep:");
    //         syslog (LOG_ERR, "Error in clock_nanosleep()\r\n");
    //         graceful_exit();   
    //         break;
    //     }
    // }

    // if(ten_sec <= 0)
    // {
        char timer_buffer[150];
        memset(timer_buffer, 0, sizeof(timer_buffer)); 
        time(&the_time);
        time_local = localtime(&the_time);

        size_t length = strftime(timer_buffer, sizeof(timer_buffer),"timestamp: %Y \t %b \t %d \t %H:%M:%S\r\n",time_local);

        //locking the mutex
        int mutex_lock = pthread_mutex_lock(mutex);
        if(mutex_lock != 0)
        {
            printf("Error locking the mutex on line 510\r\n");
            // syslog the errors
            syslog(LOG_ERR, "Error locking the mutex. Error code:%d\r\n", errno);
           // fclose(fp);......513
           pthread_exit(NULL);
        }

        printf("timer locked te mutex\r\n");
        //write to the file
        fseek(fp, 0, SEEK_END);

        if (fwrite(timer_buffer, length, 1, fp) == -1)
        {
            printf("Error in writing timer to the file\r\n");
            // Log this info
            syslog(LOG_INFO, "Error in writing timer to the file. Error: %d\r\n", errno);
            graceful_exit();
            pthread_exit(NULL);
        }
        
        //unlocking the mutex
        mutex_lock = pthread_mutex_unlock(mutex);
        if(mutex_lock != 0)
        {
            printf("Error unlocking the mutex at 530 line %d\r\n", errno);
            // syslog the errors
            syslog(LOG_ERR, "Error unlocking the mutex. Error code:%d\r\n", errno);
            //fclose(fp);
            pthread_exit(NULL);
        }
        //close(data->client_fd);
    }

    // }
    //data->thread_complete = true;
    // pthread_exit(NULL);
    return NULL;
}


int main(int argc, char *argv[])
{
    openlog("aesdsocket", 0, LOG_USER);

    int cli_fd;
    socklen_t cli_len;

    //Linked list node
    node *threads_node = NULL;
    //address variables fior the client socket
    struct sockaddr_in server, client;

    SLIST_HEAD(listhead, linked_list_node) head;
    SLIST_INIT(&head);

    // Registering error signals
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (argc == 2)
    {
        printf("2 args detected\r\n");

        // if(argv[1] == "-d")
        if (strcmp(argv[1], "-d") == 0)
        {
            is_daemon = true;
            printf("Program running as daemon\r\n");
        }
        else
        {
            printf("Program runnig as a normal program\r\n");
        }
    }

    // opening file /var/tmp/aesdsocketdata OR creating if it doesnot exist
    fp = fopen(FILE_PATH, "w+");
    if (fp == NULL)
    {
        printf("Error opening file\r\n");
        // syslog the errors
        syslog(LOG_ERR, "Error opening the file. Error code:%d\r\n", errno);
        // return -1 on error
        graceful_exit();
        return -1;
    }
     // Creating a socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1)
    {
        printf("Error creating server socket\r\n");
        // syslog the errors
        syslog(LOG_ERR, "Error creating server socket. Error code:%d\r\n", errno);
        // return -1 on error
        graceful_exit();
        return -1;
    }
    else
    {
        printf("Socket created\r\n");
        syslog(LOG_DEBUG, "Server socket created successfully\r\n");
    }

    int opt = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        printf("Error in socket set. Error code:%d\r\n", errno);
        syslog(LOG_ERR, "Error in socket()\r\n");
        graceful_exit();
        return -1;
    }
    else
    {
        printf("Socket options set\r\n");
        syslog(LOG_DEBUG, "Server socket created successfully\r\n");
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(sock_fd, (struct sockaddr *)&server, sizeof(server)))
    {
        printf("Binding failed\r\n");
        // print the error message
        syslog(LOG_ERR, "Bind failed. Error: %d\r\n", errno);
        graceful_exit();
        return (-1);
    }
    else
    {
        printf("Binding done\r\n");
    }

     // Should fork after binding is done
    if (is_daemon == true)
    {
        if (making_daemon() == -1)
        {
            printf("Failed to make a daemon process\r\n");
            // print the error message
            syslog(LOG_ERR, "Failed to make a daemon process. Error: %d\r\n", errno);
            graceful_exit();
            return (-1);
        }
    }

    if (listen(sock_fd, BACKLOG) < 0)
    {
        printf("Error in listening. Error code:%d\r\n", errno);
        syslog(LOG_ERR, "Error listening\r\n");
        graceful_exit();
        return -1;
    }
    else
    {
        printf("Waiting for connections.....\r\n");
    }

    //timer thread created
    pthread_t timer_thread_id;
    pthread_create(&timer_thread_id, NULL, timer_routine, &mutex);

    while (1)
    {
        cli_len = sizeof(client);

        cli_fd = accept(sock_fd, (struct sockaddr *)&client, &cli_len);

        if (cli_fd < 0)
        {
            printf("Error in accept. Error code:%d", errno);
            syslog(LOG_ERR, "Error in accept");
        }
        else
        {
            printf("Connection accepted\n");
            syslog(LOG_ERR, "New Connection accepted");
        }

        // Printing IP address of client
        char address[30]; // To store the IP address converted into string

        // const char *inet_ntop(int af, const void *restrict src,
        const char *ip_addr = inet_ntop(AF_INET, &client.sin_addr, address, sizeof(address));
        int client_port = htons(client.sin_port);
        if (ip_addr == NULL)
        {
            printf("Error in obtaining ip address of the client\r\n");
        }
        else
        {
            printf("Accepted connection from %s %d\r\n", ip_addr, client_port);
            // Log this info
            syslog(LOG_INFO, "Accepted connection from %s %d\r\n", ip_addr, client_port);
        }

        threads_node = (node *)malloc(sizeof(node));
        if(threads_node == NULL)
        {
              printf("Error in obtaining ip address of the client\r\n");
              graceful_exit();
        }
        SLIST_INSERT_HEAD(&head, threads_node, entry);
        threads_node->client_parameters.client_fd = cli_fd;
        threads_node->client_parameters.address = client;
        threads_node->client_parameters.mutex = &mutex;
        threads_node->client_parameters.thread_complete = false;
        
        pthread_create(&(threads_node->client_parameters.thread_id), NULL, thread_routine, (void*)&threads_node->client_parameters);

        SLIST_FOREACH(threads_node, &head, entry)
        {
            if(threads_node->client_parameters.thread_complete == true)
            {
                pthread_join(threads_node->client_parameters.thread_id, NULL);
                //after completion close the connection 
                shutdown(threads_node->client_parameters.client_fd, SHUT_RDWR);
                close(threads_node->client_parameters.client_fd);
                printf("Closed connection from %s %d\r\n", ip_addr, client_port);
                // Log this info
              //  syslog(LOG_INFO, "Closed connection from %s %d\r\n", ip_addr, client_port);    
            }
        }

    }
    pthread_join(timer_thread_id, NULL);
    while(!SLIST_EMPTY(&head))
    {
        threads_node = SLIST_FIRST(&head);
        pthread_cancel(threads_node->client_parameters.thread_id);
        SLIST_REMOVE_HEAD(&head, entry);
       // if(threads_node != NULL)
       // {
        free(threads_node);
       // }
        threads_node = NULL;
    }
    graceful_exit();
    remove(FILE_PATH);

   /*
    int cli_fd, buffer_len;
    int index = 0;
    struct sockaddr_in server, client;
    socklen_t cli_len;

    // Setting up syslog logging
    openlog("aesdsocket", 0, LOG_USER);

    // Registering error signals
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (argc == 2)
    {
        printf("2 args detected\r\n");

        // if(argv[1] == "-d")
        if (strcmp(argv[1], "-d") == 0)
        {
            is_daemon = true;
            printf("Program running as daemon\r\n");
        }
        else
        {
            printf("Program runnig as a normal program\r\n");
        }
    }

    // opening file /var/tmp/aesdsocketdata OR creating if it doesnot exist
    fp = fopen(FILE_PATH, "w+");
    if (fp == NULL)
    {
        printf("Error opening file\r\n");
        // syslog the errors
        syslog(LOG_ERR, "Error opening the file. Error code:%d\r\n", errno);
        // return -1 on error
        graceful_exit();
        return -1;
    }

    // Creating a socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1)
    {
        printf("Error creating server socket\r\n");
        // syslog the errors
        syslog(LOG_ERR, "Error creating server socket. Error code:%d\r\n", errno);
        // return -1 on error
        graceful_exit();
        return -1;
    }
    else
    {
        printf("Socket created\r\n");
        syslog(LOG_DEBUG, "Server socket created successfully\r\n");
    }

    int opt = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        printf("Error in socket set. Error code:%d\r\n", errno);
        syslog(LOG_ERR, "Error in socket()\r\n");
        graceful_exit();
        return -1;
    }
    else
    {
        printf("Socket options set\r\n");
        syslog(LOG_DEBUG, "Server socket created successfully\r\n");
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(sock_fd, (struct sockaddr *)&server, sizeof(server)))
    {
        printf("Binding failed\r\n");
        // print the error message
        syslog(LOG_ERR, "Bind failed. Error: %d\r\n", errno);
        graceful_exit();
        return (-1);
    }
    else
    {
        printf("Binding done\r\n");
    }

    // Should fork after binding is done
    if (is_daemon == true)
    {
        if (making_daemon() == -1)
        {
            printf("Failed to make a daemon process\r\n");
            // print the error message
            syslog(LOG_ERR, "Failed to make a daemon process. Error: %d\r\n", errno);
            graceful_exit();
            return (-1);
        }
    }

    if (listen(sock_fd, BACKLOG) < 0)
    {
        printf("Error in listening. Error code:%d\r\n", errno);
        syslog(LOG_ERR, "Error listening\r\n");
        graceful_exit();
        return -1;
    }
    else
    {
        printf("Waiting for connections.....\r\n");
    }

    while (1)
    {
        // to track the length of the buffer
        static int len_tracker = 0;

        cli_len = sizeof(client);

        cli_fd = accept(sock_fd, (struct sockaddr *)&client, &cli_len);

        if (cli_fd < 0)
        {
            printf("Error in accept. Error code:%d", errno);
            syslog(LOG_ERR, "Error in accept");
        }
        else
        {
            printf("Connection accepted\n");
            syslog(LOG_ERR, "New Connection accepted");
        }

        // Printing IP address of client
        char address[30]; // To store the IP address converted into string

        // const char *inet_ntop(int af, const void *restrict src,
        //    char *restrict dst, socklen_t size);
        const char *ip_addr = inet_ntop(AF_INET, &client.sin_addr, address, sizeof(address));
        int client_port = htons(client.sin_port);
        if (ip_addr == NULL)
        {
            printf("Error in obtaining ip address of the client\r\n");
        }
        else
        {
            printf("Accepted connection from %s %d\r\n", ip_addr, client_port);
            // Log this info
            syslog(LOG_INFO, "Accepted connection from %s %d\r\n", ip_addr, client_port);
        }

        // Initial malloc
        char *buffer = (char *)malloc(MAX_SIZE);
        len_tracker = MAX_SIZE;
        if (buffer == NULL)
        {
            printf("Error in Malloc\r\n");
            // Log this info
            syslog(LOG_INFO, "Error in Malloc\r\n");
            return -1;
        }

        // memset the buffer so it has all zeros
        memset(buffer, 0, MAX_SIZE);

        // Reception and Transmission
        while (1)
        {

            // reads from the client, store it in a buffer
            buffer_len = read(cli_fd, buffer + len_tracker - MAX_SIZE, MAX_SIZE);

            if (buffer_len == 0)
            {
                printf("Disconnected from client\n");
                break;
            }
            if (buffer_len < 0)
            {
                printf("Error in reading from the client\r\n");
                break;
            }

            // stop with newline character
            for (index = 0; index < buffer_len; index++)
            {
                if (buffer[index + len_tracker - MAX_SIZE] == '\n')
                {
                    break;
                }
            }

            // if(buffer_len < MAX_SIZE)
            if (index < buffer_len)
            {
                printf("2\n");
                // can write to file directly
                // fwrite(buffer, buffer_len, 1, fp);
                if (fwrite(buffer, len_tracker - MAX_SIZE + index + 1, 1, fp) == -1)
                {
                    printf("Error in writing to the file\r\n");
                    // Log this info
                    syslog(LOG_INFO, "Error in writing to the file. Error: %d\r\n", errno);
                    free(buffer);
                    buffer = NULL;
                    graceful_exit();
                    return -1;
                }
                break;
            }
            else
            {
                printf("3\n");
                len_tracker += MAX_SIZE;
                buffer = (char *)realloc(buffer, len_tracker);
            }
        }
        printf("1\n");
        len_tracker = 0;
        free(buffer);
        buffer = NULL;
        int file_bytes = 0;
        char ch;
        fseek(fp, 0, SEEK_SET);
        while (fread(&ch, 1, 1, fp) > 0)
        {
            file_bytes++;
        }
        char *write_buffer = (char *)malloc(file_bytes);
        if (write_buffer == NULL)
            return -1;

        fseek(fp, 0, SEEK_SET);
        if (fread(write_buffer, file_bytes, 1, fp) < 0)
        {
            printf("Error in reading file\r\n");
            // Log this info
            syslog(LOG_INFO, "Error in reading file. Error: %d\r\n", errno);
            free(write_buffer);
            write_buffer = NULL;
            graceful_exit();
            return -1;
        }
        printf("buf: ");
        for (int i = 0; i < (file_bytes); i++)
            printf("%c", write_buffer[i]);

        if (write(cli_fd, write_buffer, file_bytes) == -1)
        {
            printf("Error in writing to transmit buffer\r\n");
            // Log this info
            syslog(LOG_INFO, "Error in writing to transmit buffer. Error: %d\r\n", errno);
            free(write_buffer);
            write_buffer = NULL;
            graceful_exit();
            return -1;
        }

        free(write_buffer);
        write_buffer = NULL;
        close(cli_fd);
        printf("Closed connection from %s %d\r\n", ip_addr, client_port);
        // Log this info
        syslog(LOG_INFO, "Closed connection from %s %d\r\n", ip_addr, client_port);
    }
    graceful_exit();
    */
    return 0;
}

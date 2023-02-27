/*
References: https://man7.org/linux/man-pages/man2/getsockname.2.html
https://beej.us/guide/bgnet/html/#a-simple-stream-server
https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/
https://nullraum.net/how-to-create-a-daemon-in-c/
https://stackoverflow.com/questions/14846768/in-c-how-do-i-redirect-stdout-fileno-to-dev-null-using-dup2-and-then-redirect
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
#include <syslog.h>  //to use syslog
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <syslog.h>
#include <fcntl.h>

#define PORT  9000
#define BACKLOG  5
#define MAX_SIZE 1024
#define FILE_PATH "/var/tmp/aesdsocketdata"


int sock_fd;
 FILE *fp;
//Daemon Check flag initialized to false
bool is_daemon = false;

void print_buf(char* buffer, int buffer_len);
void graceful_exit();

void print_buf(char* buffer, int buffer_len)
{
    for(int i=0; i< buffer_len; i++)
    {
        printf("%x,", buffer[i]);
    }
    printf("The length of buff: %d\r\n", buffer_len);
}

void signal_handler(int signal)
{
    if((signal == SIGINT) || (signal == SIGTERM))
    {
        printf("Signal Caught is %d, exiting\n",signal);
        close(sock_fd);
        exit(0);
    }

}

void graceful_exit()
{
    
    //Closing the server socket
    if(sock_fd)
    {
    	close(sock_fd);
	//Closing both recv and send on the server socket
    	shutdown(sock_fd, SHUT_RDWR);
    }
    if(fp !=NULL)    
	fclose(fp);
    remove(FILE_PATH);	
    //Closing syslog
    closelog();
}

int making_daemon()
{
    //Making a process daemon 
    //Steps: fork --> terminate parent --> setsid -->  chdir --> redirect files to /dev/null 

    //Forking
    int fork_pid = fork();
    if(fork_pid < 0)
    {
        printf("Forking failed");
        syslog(LOG_ERR,"Forking failed. Error: %d\r\n", errno);
        //Should return if tried to run as daemon and fails
        return -1;
    }
    else if(fork_pid > 0)
    {
        //Terminate the parent process as fork was successfull
        exit(0);
    }

    //setsid 
    //Makes Child process the session leader
    if(setsid() < 0)
    {
        printf("Setting Session ID failed");
        syslog(LOG_ERR,"Setting Session ID failed Error: %d\r\n", errno);
        //Should return -1
        return -1;
    }

    //chdir Switching to the root directory as it will be a background process now
    if(chdir("/") == -1)
    {
        printf("Changing directory failed");
        syslog(LOG_ERR,"Changing directory ID failed Error: %d\r\n", errno);
        //Should return -1
	return -1;
    }
   	 //Redirect output to /dev/null
    int devNull = open("/dev/null", O_RDWR);
    if(devNull == -1)
    {
	 printf("Error in opening /dev/null");
        syslog(LOG_ERR,"Error in opening /dev/null. Error: %d\r\n", errno);
        //Should return -1
	return -1;       
    }

    	//redirect stdout and stderror
    int dup2stdout = dup2(devNull, STDOUT_FILENO);
    if(dup2stdout == -1)
    {
        printf("Error in dup2(devNull, STDOUT_FILENO)\n");
        syslog(LOG_ERR,"Error in dup2(devNull, STDOUT_FILENO). Error: %d\r\n", errno);
        //Should return -1
	return -1; 
    }
    int dup2stderr = dup2(devNull, STDERR_FILENO);
    if(dup2stderr == -1)
    {
        printf("Error in dup2(devNull, STDERR_FILENO)\n");
        syslog(LOG_ERR,"Error in dup2(devNull, STDERR_FILENO). Error: %d\r\n", errno);
        //Should return -1
	return -1;
    }
    int dup2stdin = dup2(devNull, STDIN_FILENO);
    if(dup2stdin == -1)
    {
        printf("Error in dup2(devNull, STDIN_FILENO)\n");
        syslog(LOG_ERR,"Error in dup2(devNull, STDIN_FILENO). Error: %d\r\n", errno);
        //Should return -1
	return -1;
    }
    
    return 0;
}


int main(int argc, char *argv[])
{
    int cli_fd, buffer_len;
    int index = 0;
    struct sockaddr_in server, client;
    socklen_t cli_len;

    //Setting up syslog logging
	openlog("aesdsocket",0,LOG_USER);

    //Registering error signals
    signal(SIGINT, signal_handler);
    signal(SIGTERM,signal_handler);

    if(argc == 2)
    {
        printf("2 args detected\r\n");
        
       // if(argv[1] == "-d")
       if(strcmp(argv[1], "-d") == 0)
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
    if(fp == NULL)
    {
        printf("Error opening file\r\n");
        //syslog the errors
        syslog(LOG_ERR,"Error opening the file. Error code:%d\r\n", errno);
        //return -1 on error
	graceful_exit();
        return -1;
    }

    //Creating a socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd == -1)
    {
        printf("Error creating server socket\r\n");
        //syslog the errors
        syslog(LOG_ERR,"Error creating server socket. Error code:%d\r\n", errno);
        //return -1 on error
        graceful_exit();
        return -1;
    }
    else
    {
        printf("Socket created\r\n");
        syslog(LOG_DEBUG, "Server socket created successfully\r\n");
    }

    int opt = 1;
    if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        printf("Error in socket set. Error code:%d\r\n",errno);
        syslog(LOG_ERR,"Error in socket()\r\n");
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

    if(bind(sock_fd, (struct sockaddr*)&server, sizeof(server)))
    {
        printf("Binding failed\r\n");
        //print the error message
        syslog(LOG_ERR,"Bind failed. Error: %d\r\n", errno);
        graceful_exit();
        return (-1);
    }
    else
    {
       printf("Binding done\r\n");
    }

    //Should fork after binding is done
    if(is_daemon == true)
    {
        if(making_daemon()  == -1)
        {
            printf("Failed to make a daemon process\r\n");
            //print the error message
            syslog(LOG_ERR,"Failed to make a daemon process. Error: %d\r\n", errno);
    	     graceful_exit();
            return (-1);
        }
    }

     if(listen(sock_fd, BACKLOG) < 0)
        {
           printf("Error in listening. Error code:%d\r\n",errno);
           syslog(LOG_ERR,"Error listening\r\n");
           graceful_exit();
           return -1;
        }
        else
        {
            printf("Waiting for connections.....\r\n");
        }

    while(1) 
    {
        //to track the length of the buffer
	 static int len_tracker = 0;
	    
        cli_len = sizeof(client);

        cli_fd = accept(sock_fd, (struct sockaddr*)&client, &cli_len);

        if (cli_fd < 0)
        {
            printf("Error in accept. Error code:%d",errno);
            syslog(LOG_ERR,"Error in accept");
        }
        else
        {
            printf("Connection accepted\n");
            syslog(LOG_ERR, "New Connection accepted");
        }

        //Printing IP address of client
        char address[30];   //To store the IP address converted into string
        
        //const char *inet_ntop(int af, const void *restrict src,
                          //   char *restrict dst, socklen_t size);
        const char*ip_addr = inet_ntop(AF_INET, &client.sin_addr, address, sizeof(address));
        int client_port = htons(client.sin_port);
        if(ip_addr == NULL)
        {
            printf("Error in obtaining ip address of the client\r\n");
        }
        else
        {
            printf("Accepted connection from %s %d\r\n", ip_addr,client_port);
            //Log this info
            syslog(LOG_INFO,"Accepted connection from %s %d\r\n", ip_addr,client_port);
        }

	//Initial malloc
	    char *buffer = (char*) malloc(MAX_SIZE);
	    len_tracker = MAX_SIZE;
	    if(buffer == NULL)
	    {
           	printf("Error in Malloc\r\n");
            	//Log this info
            	syslog(LOG_INFO,"Error in Malloc\r\n");
	    	return -1;
	    }

	    //memset the buffer so it has all zeros
	     memset(buffer,0,MAX_SIZE);

        //Reception and Transmission
        while(1)
        {
	    
	    //reads from the client, store it in a buffer
            buffer_len = read(cli_fd, buffer + len_tracker - MAX_SIZE, MAX_SIZE);
		
            if(buffer_len == 0)
            {
                printf("Disconnected from client\n");
                break;
            }
	    if(buffer_len < 0)
	    {
	    	printf("Error in reading from the client\r\n");
		break;
	    }
	
            //stop with newline character
            for(index = 0; index < buffer_len; index++)
            {
                if (buffer[index + len_tracker - MAX_SIZE] == '\n')
                {
		    break;
                }
            }
            

	   //if(buffer_len < MAX_SIZE)
	    if(index < buffer_len)
	   {	  
	   	printf("2\n"); 	
		//can write to file directly
		//fwrite(buffer, buffer_len, 1, fp);
		if(fwrite(buffer, len_tracker - MAX_SIZE + index + 1, 1, fp) == -1)
		 {
           		printf("Error in writing to the file\r\n");
           		//Log this info
           		syslog(LOG_INFO,"Error in writing to the file. Error: %d\r\n", errno);
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
           	buffer = (char*)realloc(buffer,len_tracker);
		  
	   }
	}
	printf("1\n");
	len_tracker = 0;
	free(buffer);
	buffer = NULL;
	int file_bytes = 0;
        char ch;
        fseek(fp, 0, SEEK_SET);
        while(fread(&ch, 1, 1, fp) > 0)
        {
            file_bytes++;
        }
        char *write_buffer = (char*) malloc (file_bytes);
	if(write_buffer == NULL)
	    return -1;

        fseek(fp, 0, SEEK_SET);
        if(fread(write_buffer, file_bytes, 1, fp) < 0)
        {
	    printf("Error in reading file\r\n");
            //Log this info
            syslog(LOG_INFO,"Error in reading file. Error: %d\r\n", errno);
            free(write_buffer);
	     write_buffer = NULL;   
            graceful_exit();
	    return -1;
	}
        printf("buf: ");
        for (int i=0; i<(file_bytes);i++)
            printf("%c",write_buffer[i]);

        if(write(cli_fd, write_buffer, file_bytes) == -1)
        {
            printf("Error in writing to transmit buffer\r\n");
            //Log this info
            syslog(LOG_INFO,"Error in writing to transmit buffer. Error: %d\r\n", errno);
    	    free(write_buffer);
	    write_buffer = NULL;   
    	    graceful_exit();
            return -1;
        }
      
	free(write_buffer);
	write_buffer = NULL;        
	close(cli_fd);
	printf("Closed connection from %s %d\r\n", ip_addr,client_port);
        //Log this info
        syslog(LOG_INFO,"Closed connection from %s %d\r\n", ip_addr,client_port);
    }
    graceful_exit();
    return 0;
}



//  Resources:https://www.tutorialspoint.com/cprogramming/c_command_line_arguments.htm
//	  Coursera videos
//	  https://www.cprogramming.com/tutorial/c/lesson14.html
//	  Linux System Programming, Second Edition
//	  https://stackoverflow.com/questions/8485333/syslog-command-in-c-code	 
//	  https://www.youtube.com/watch?v=wa2AfzyOff0 


//Include required headers
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h> 

int main( int argc, char *argv[] )
{
	//Setting up syslog logging
	openlog(NULL,0,LOG_USER);	

	//Number of arguments check
	if(argc != 3)
	{
		syslog(LOG_ERR, "Invalid number of argguments: %d. Two inputs are needed", argc);
		syslog(LOG_ERR, " First: The full path to the file. ");
		syslog(LOG_ERR, " Second: Text string to write into this file ");
		exit (1);
	}
	
	 //Check if the string is a valid string
	 if(*argv[2] == '\0')
	 {
	    syslog(LOG_ERR, "Error: String is empty");
	    exit (1); 
	 }
	 
	 //Creating or Opening the existing file
	 int fd;
	 fd = creat(argv[1], 777);
	 
	 //Exit if file cannot be created or opened
	  if ( fd == -1 )
         {
            syslog( LOG_ERR, "Could not open/create file: %s", strerror(errno));
	     exit (1);
         }
         else
         {
         	//Write the string to the file passed as argument
         	ssize_t write_to_file = write(fd, argv[2], strlen(argv[2]));
		//Log if issue writing to the file
	 	if(write_to_file == -1)
	 	{
	 		syslog(LOG_ERR, "Error: %s", strerror(errno));
	 	}
	 	//Print into LOG_DEBUG for successfull write to the file
         	else 
         	{
            		syslog(LOG_DEBUG, "Writing %s to file %s", argv[2], argv[1]);

      		}
      		//Close the file
    	  	close(fd);
    	 }
    	//Log if issue closing the file
	if(close(fd) == -1)
	{
	   	syslog(LOG_ERR, "Error closing the file: %s", strerror(errno));	  
	}

  //CLosing the syslog log
  closelog();
  return 0;
}


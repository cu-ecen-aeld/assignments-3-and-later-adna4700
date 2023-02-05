#include "systemcalls.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <syslog.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{   
    //setting up system log
  

    int return_from_system = system(cmd);

    
    	if((return_from_system != 0) || (cmd == NULL))
		return false;
	else 
    //NO error occurs; system() returns success 
   
    		return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

     pid_t return_from_fork, result_from_wait;
     int w_status;
    openlog(NULL, 0, LOG_USER);
	
    return_from_fork = fork();
    if(return_from_fork == -1) 
    	{
		//Child process could not be created 
		syslog( LOG_ERR, "Error: %s", strerror(errno));
		return false;
		     		
    	} 
    else if(return_from_fork == 0)
    	{
		//Child process created 
		syslog(LOG_ERR,"Child process created");
		//call execv() 	
		execv(command[0],command);
		//returns only on failure		
		exit(-1);
    	}
     else
     {
    	//call wait() form parent
    	
    	result_from_wait = waitpid(return_from_fork,&w_status, 0);
    	if(result_from_wait == -1)
	{
		//if error occurs 
		syslog( LOG_ERR, "Error: %s", strerror(errno));
		return false;
	}
	if(WIFEXITED(w_status))
	{
		if(w_status)
			return false;
		else
			return true;
	}
    }

    va_end(args);
    
    closelog();
    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

    pid_t return_from_fork, result_from_wait;
    int w_status;
    
    int fd = open("redirected.txt", O_WRONLY|O_TRUNC|O_CREAT, 0644);
    if(fd == -1)
    {
    	perror("Error creating a file");
	return false;
    }
    
    return_from_fork = fork();
	
    if(return_from_fork == -1) 
    	{
		//Child process could not be created 
		perror("fork"); 
		return false;
		     		
    	} 
    else if(return_from_fork == 0)
    	{
		//Child process created 
		if (dup2(fd, 1) < 0) 
		{ 
			perror("dup2"); 
			return false; 
		}
		close(fd);
		//call execv() 	
		execv(command[0],command);
		perror("execvp"); 
		exit(-1);	
			
    	}
     else
   	  {
   	  	close(fd);
    		result_from_wait = waitpid(return_from_fork,&w_status, 0);
    		if(result_from_wait == -1)
		{
			//if error occurs 
			syslog( LOG_ERR, "Error: %s", strerror(errno));
			return false;
		}
		if(WIFEXITED(w_status))
		{
			if(w_status)
				return false;
			else
				return true;
		}	
   	 }   
   	 
    va_end(args);	 
    return true;
}

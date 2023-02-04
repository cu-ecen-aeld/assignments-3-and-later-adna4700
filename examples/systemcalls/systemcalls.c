#include "systemcalls.h"

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
    openlog(NULL, 0, LOG_USER);

    int return_from_system = system(cmd);
    
    if(cmd == NULL)
	{
    	if(return_from_system != 0)
		{
			syslog(LOG_ERR, "SHELL available");
			closelog();
			return false;
    		}
	if(return_from_system == 0)
		{
			syslog(LOG_ERR, "SHELL isn't available");
			closelog();
			return false;				
    		}
	}
     if(return_from_system == -1)
     	{
		//Child process could not be created or its status could not be retrived
		syslog( LOG_ERR, "Error: %s", strerror(errno));
		closelog();
		return false;    		
     	}
     if(return_from_system  == 127)
	{
		//Child process created but the child shell terminated
		syslog( LOG_ERR, "Child shell terminated");	
		closelog();
		return false; 	
	}
    
    //NO error occurs; system() returns success 
    closelog();    
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

    openlog();
    pid_t return_from_fork = fork();
    if(return_from_fork == -1) 
    	{
		//Child process could not be created 
		syslog( LOG_ERR, "Error: %s", strerror(errno));
		closelog();
		return false;
		     		
    	} 
    if(return_from_fork == 0)
    	{
		//Child process created 
		syslog("Child process created");
		//call execv() 	
		int return_excev = execv(command[0],command);
		//returns only on failure		
		if(return_execv == -1)
			{
				syslog(LOG_ERR, "Error: %s", strerror(errno));
				closelog();
				return false;	
			}
    	}
    //call wait() form parent
    int w_status;
    pid_t result_from_wait = waitpid(pid,&w_status, 0);
    if(result_from_wait == -1)
	{
		//if error occurs 
		syslog( LOG_ERR, "Error: %s", strerror(errno));
		closelog();
		return false;
	}

    //if no error check for status
    if(WIFEXITED(w_status))
	{
		syslog( LOG_ERR, "Child terminated normally: %s", WEXITSTATUS(w_status));
		if(WEXITSTATUS(w_status != 0))
			{
				return false;
			}
	}
    else if(WIFSIGNALED(w_status))
    	{
    		syslog( LOG_ERR, "Child terminated by a signal: %s", WTERMSIG(w_status));
		if(WCOREDUMP)
		{
			syslog( LOG_ERR, "Child produced a core dump");
		}
    	}
    else if(WTFSTOPPED(w_status))
    	{
    		syslog( LOG_ERR, "The number of signal which caused the child to stop",  WSTOPSIG());
    	}

	if(WTFCONTINUED(w_status))
		{
			syslog( LOG_ERR, "Child continued");
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

    int fd = open("redirected.txt", O_WRONLY|O_TRUNC|O_CREAT, 0644);
    if (fd < 0) { perror("open"); abort(); }

    pid_t return_from_fork = fork();

    if(return_from_fork == -1) 
    	{
		//Child process could not be created 
		error("fork"); abort();
		return false;
		     		
    	} 
    if(return_from_fork == 0)
    	{
		//Child process created 
		if (dup2(fd, 1) < 0) { perror("dup2"); abort(); }
		//call execv() 	
		int return_excev = execv(command[0],command);
		//returns only on failure		
		if(return_execv == -1)
			{
				perror("execvp"); abort();
				close(fd);
				return false;	
			}
    	}
    //call wait() form parent
    int w_status;
    pid_t result_from_wait = waitpid(pid,&w_status, 0);
    if(result_from_wait == -1)
	{
		//if error occurs 
		perror( "Error: %d", errno);
		close(fd);
		return false;
	}

    //if no error check for status
    if(WIFEXITED(w_status))
	{
		perror("Child terminated normally: %s", WEXITSTATUS(w_status));
		close(fd);
		if(WEXITSTATUS(w_status != 0))
			{
				return false;
			}
	}
    else if(WIFSIGNALED(w_status))
    	{
    		perror( "Child terminated by a signal: %s", WTERMSIG(w_status));
		if(WCOREDUMP)
		{
			perror("Child produced a core dump");
		}
		close(fd);
    	}
    else if(WTFSTOPPED(w_status))
    	{
    		perror("The number of signal which caused the child to stop",  WSTOPSIG());
		close(fd);
    	}

	if(WTFCONTINUED(w_status))
		{
			perror("Child continued");
			close(fd);
		}



    va_end(args);

    return true;
}

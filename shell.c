#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include<stdbool.h>


//array to store command history
char *command_history[6];
int indx=0;
int count=0;

//array to store pids of process 
int process_history[2000];
int total_no_of_process=0;

//function to find minimum of two elements
int min(int a,int b)
{
	if(a>b)
	{
		return b;
	}
	return a;
}

//function to handle ctrl+C
//adding signal command
void signal_handling(int sig_no) 
{
    signal(sig_no,SIG_IGN);
    exit(0);
}

//helper function for command history
void print_command_history(char* command)
{
	command_history[indx]=malloc(256);
	strcpy(command_history[indx],command);
	indx=(indx+1)%6;
	count++;
}

//function for printing command history
void print_cmd_history()
{
	indx=(indx+5)%6;
	for(int i=1;i<min(count,6);i++)
	{
		indx=(indx+5)%6;
		printf("%s",command_history[indx]);
		printf("\n");
	}
}

//function which print outs current status of the process the Pids of the process
int print_ps_history()
{
    for (int i=0;i<total_no_of_process;i++)
	{
		//if kill returns 0 then process is running else process is stopped
		if (kill(process_history[i],0) == 0)
		{
			printf("%d %s\n", process_history[i],"RUNNING");
		}
		else
		{
			printf("%d %s\n", process_history[i],"STOPPED");
		}
	}
    return 0;
}

//function which checks whether '=' occurs in the string for the background process
int check_equal(char* s)
{
	for(int i=0;i<strlen(s);i++)
	{
		if(s[i]=='=')
		{
			return 1;
		}
	}
	return 0;
}

//function which checks whether '$' occurs in the string for the background process
int check_dollar(char* s)
{
	for(int i=0;i<strlen(s);i++)
	{
		if(s[i]=='$')
		{
			return 1;
		}
	}
	return 0;
}

//function which uses getenv command in case of ennvironment variable
void environment_variable(char** s)
{
	for(int i=0;i<1024;i++)
	{
		if(s[i]==NULL)
		{
			break;
		}
		if(check_dollar(s[i])==1)
		{
			// if command contains $ the break the command at $ and use getenv to print the value 
			char* token = strtok(s[i], "$");
			char* c = token;
			char* v;
			while (token != NULL)
			{
				v=token;
				token = strtok(NULL, "$");
			}
			s[i]=getenv(v);
		}
		
	}
}


//function for parsing space in the command
void space_parsing(char* s, char** string)
{
    for (int i = 0; i < 50; i++) 
	{
		string[i] = strsep(&s, " ");
        if (string[i] == NULL)
		{
			break;
		}
		if (strlen(string[i]) == 0)
		{
			i--; 
		}
	}
}

// function which checks pipe is present or not
int Piping(char* s, char** a)
{
	for (int i = 0; i < 2; i++) 
	{
		a[i] = strsep(&s, "|");
		if (a[i] == NULL)
		{
			break;
		}
	}
	//if pipe is present it returns 1 else it returns 0
    if (a[1] == NULL)
	{
		return 0; 
	}
	else 
	{
		return 1;
	}
}

//function for piping
void Piped(char** string, char** string_pipe)
{
    int arg[2]; 
	//piping the arguments
    pid_t process1, process2;
    if (pipe(arg) < 0)
	{
        return;
    }
    process1 = fork();
    if (process1 == 0)
	{
		//child process before piping
        close(arg[0]);
        dup2(arg[1], STDOUT_FILENO);
        close(arg[1]);
		//checking if first command in piping command is valid or not
		//printing command history
		if(strcmp(string[0],"cmd_history")==0)
		{
			print_cmd_history();
			exit(0);
		}
		//printing ps_history
		else if(strcmp(string[0],"ps_history")==0)
		{
			print_ps_history();
			exit(0);
		}
        else if (execvp(string[0], string) < 0)
		{
            printf("Command 1 could not be executed");
			printf("\n");
			exit(0);
        }
    } 
	else
	{
		//parent process pids storing
		process_history[total_no_of_process]=process1;
		total_no_of_process++;
		//forking the given process
        process2 = fork();
        if (process2 == 0)
		{
            close(arg[1]);
            dup2(arg[0], STDIN_FILENO);
            close(arg[0]);
			//checking if second command in piping command is valid or not
			//printing command history
			if(strcmp(string_pipe[0],"cmd_history")==0)
			{
				print_cmd_history();
				exit(0);
			}
			//printing ps_history
			else if(strcmp(string_pipe[0],"ps_history")==0)
			{
				print_ps_history();
				exit(0);
			}
            else if (execvp(string_pipe[0],string_pipe) < 0)
			{
                printf("Command 2 could not be executed");
				printf("\n");
				exit(0);
            }
        } 
		else 
		{
			process_history[total_no_of_process]=process2;
			total_no_of_process++;
			//parent is exceuting and waiting for its 2 children
			close(arg[0]);
			close(arg[1]);
            wait(NULL);
            wait(NULL);
		}
    }
}


int Process(char* s, char** string, char** string_P)
{
    char* a[2];
	int count_pipes = 0;
    count_pipes=Piping(s,a);
    if(count_pipes) 
	{
		space_parsing(a[0], string);
		environment_variable(string);
		space_parsing(a[1],string_P);
		environment_variable(string_P);
    } 
	else
	{
       space_parsing(s,string);
	   environment_variable(string);
	}
	return 1 + count_pipes;
}

    

int main()
{
	signal(SIGINT, signal_handling);
	char *Argument[50];
	char command[1024];
	char* Argument_piped[50];
	int var=0;
    while (true)
	{
		//printing current working directory
		char current_working_directory[1024];
	    getcwd(current_working_directory, sizeof(current_working_directory));
     	printf("%s",current_working_directory);
		printf("~$ ");
		
		
		if(!fgets(command,1024, stdin))
		{  
            break;                                
        }
	    for(int i=0; i<strlen(command); i++)
		{    
            if(command[i]=='\n')
			{      
                command[i]='\0';
            }
        }
		
		//in case '=' is present in the command then breaking the string at = and setting the variable value
		if(check_equal(command)==1)
		{
			//breaking string at '='
			char* var = strtok(command, "=");
			char* c = var;
			char* v;
			while (var!= NULL)
			{
				v=var;
				var = strtok(NULL, "=");
			}
			//setting the variable
			setenv(c,v,1);
			continue;
		}
		print_command_history(command);
		var = Process(command,Argument,Argument_piped);
		
		
		//if there is no pipe
		if (var == 1)
		{
			pid_t processid = fork();
			//flag variable stores whether the process is background or not
			//if command is running in background then flag is set to 1 else 0
			int flag=0;
			if(Argument[0][0]=='&')
			{
				flag=1;
				*Argument[0]++;
			}
			int tmp;
			if (processid == 0) 
			{
				//printing command history
				if(strcmp(Argument[0],"cmd_history")==0)
				{
					print_cmd_history();
					continue;
				}
				//printing ps_history
				else if(strcmp(Argument[0],"ps_history")==0)
				{
					print_ps_history();
					continue;
				}
				else if (execvp(Argument[0], Argument) < 0) 
				{
					printf("Command could not be executed");
					exit(0);
				}
				printf("\n");
			} 
			else 
			{
				process_history[total_no_of_process]=processid;
		        total_no_of_process++;
				if(flag==0)
				{
					wait(NULL);
				}
				else
				{
					waitpid(processid,&tmp,WNOHANG);
				}
			}
		}
		//is piping command is used
		if (var == 2)
		{
			Piped(Argument,Argument_piped);
		}
	}
	return 0;
}


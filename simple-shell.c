#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAXLEN 1024
#define MAXARGS 256
#define true 1
#define false !true

pid_t pid;
char* currentDirectory;

void init()
{
    // Get the current directory that will be used in different methods
    currentDirectory = (char*) calloc(1024, sizeof(char));
}

void prompt(){
    // We print the prompt in the form "<user>@<host> <cwd> >"
    char hostn[1204] = "";
    gethostname(hostn, sizeof(hostn));
    printf("%s@%s %s > ", getenv("LOGNAME"), hostn, getcwd(currentDirectory, 1024));
}


void cd(char** args)
{
    // If we write no path (only 'cd'), then go to the home directory
    if (args[1] == NULL) chdir(getenv("HOME")); 
    // Else we change the directory to the one specified by the argument, if possible
    else if (chdir(args[1]) == -1) printf("%s: no such directory\n", args[1]);
}

void redirection()
{
    return;
}

void piper(char** args)
{
    int end = true;
    int desEven[2], desOdd[2];      // file descriptors
    int cmd_num = 1;
    char* command[256];

    for(int i = 0; args[i] != NULL; ++i) 
        if(strcmp(args[i],"|") == 0) ++cmd_num;

    for(int i = 0, cmd = 0; end; ++i, ++cmd)
    {
        for(int j = 0; strcmp(args[i], "|") != 0; ++j)
        {
            command[j] = args[i++];
            command[j+1] = NULL;
            if(args[i] == NULL)
            {
                end = false;
                break;
            }
        }

        if (cmd % 2 != 0) pipe(desOdd); // for odd cmd
        else pipe(desEven);             // for even cmd

        pid = fork();
        if(pid < 0) 
        {
            if (cmd != cmd_num - 1){
                if (cmd % 2 != 0) close(desOdd[1]); // for odd cmd
                else close(desEven[1]);             // for even cmd
            }			
            printf("Child could not be created.\n");
            return;
        }    
        if(pid == 0)    // child code
        {   
            // If we are in the first command
            if (cmd == 0)
                dup2(desEven[1], STDOUT_FILENO);
            // If we are in the last command, depending on whether it
            // is placed in an odd or even position, we will replace
            // the standard input for one pipe or another. The standard
            // output will be untouched because we want to see the 
            // output in the terminal
            else if (cmd == cmd_num - 1)
            {
                if (cmd_num % 2 != 0)   // for odd number of commands
                    dup2(desOdd[0], STDIN_FILENO);
                else                    // for even number of commands
                    dup2(desEven[0], STDIN_FILENO);
            // If we are in a command that is in the middle, we will
            // have to use two pipes, one for input and another for
            // output. The position is also important in order to choose
            // which file descriptor corresponds to each input/output
            }
            else
            {
                if (cmd % 2 != 0)       // for odd cmd
                {
                    dup2(desEven[0], STDIN_FILENO); 
                    dup2(desOdd[1], STDOUT_FILENO);
                }
                else                    // for even cmd
                {
                    dup2(desOdd[0], STDIN_FILENO); 
                    dup2(desEven[1], STDOUT_FILENO);					
                } 
            }
            
            if(execvp(command[0], command) == -1) 
            {
                printf("%s: command not found\n", command[0]);  
                fflush(stdout);
                kill(getpid(), SIGTERM);
            }
        }
        // parent code
        if (cmd == 0) close(desEven[1]);
        else if (cmd == cmd_num - 1)
        {
            if (cmd_num % 2 != 0)					
                close(desOdd[0]);
            else				
                close(desEven[0]);
        }
        else
        {
            if (cmd % 2 != 0)
            {					
                close(desEven[0]);
                close(desOdd[1]);
            }
            else
            {					
                close(desOdd[0]);
                close(desEven[1]);
            }
        }

        waitpid(pid, NULL, 0);
    }
}

void launch(char** args)
{
    pid = fork();
    if(pid < 0) 
    {
        printf("Child could not be created.\n");
        return;
    }    
    else if(pid == 0)    // child code
    {   
        if(execvp(args[0], args) == -1) 
        {
            printf("%s: command not found\n",args[0]);  
            fflush(stdout);
            kill(getpid(),SIGTERM);
        }
    }
    else                // parent code
    {
        waitpid(pid, NULL, 0);
    }
}

void execute(char** args)
{
    char* args_aux[256];
    for(int i = 0; args[i] != NULL; ++i)    //detect special characters
    {
        // detect piping
        if (strcmp(args[i], "|") == 0) 
        {
            piper(args);
            return;
        }
        // redirection
        else if (strcmp(args[i], "<") == 0) 
        {
            redirection();
            return;
        }
        else if (strcmp(args[i], ">") == 0) 
        {
            redirection();
            return;
        }
        else if (strcmp(args[i], ">>") == 0) 
        {
            redirection();
            return;
        }
        args_aux[i] = args[i];
    }
    if(strcmp(args[0], "exit") == 0) exit(0);
    else if(strcmp(args[0], "clear") == 0) system("clear");
    else if(strcmp(args[0], "cd") == 0)  cd(args);
    else if(strcmp(args[0], "pwd") == 0) printf("%s\n", getcwd(currentDirectory, 1024));
    else launch(args);
}

int main(int argc, char* argv[])
{
    init();

    char line[MAXLEN];
    char* args[MAXARGS];

    while(true)
    {
        prompt();
        fgets(line, MAXLEN, stdin);     // read line from user
        // printf("line is %s\n", line);
        args[0] = strtok(line, " \n\t");
        // printf("arg is %s\n", args[0]);
        int i = 1;
        while( (args[i] = strtok(NULL, " \n\t")) != NULL) ++i;
        // printargs(args);
        execute(args);
    }

    return 0;
}
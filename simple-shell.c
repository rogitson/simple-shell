#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAXLEN 1024
#define MAXARGS 256

void execute(char** args)
{
    if(strcmp(args[0], "exit") == 0) exit(0);
    pid_t pid;
    pid = fork();
    if(pid < 0) return;
    else if(pid == 0)    // child code
    {   if(execvp(args[0], args) == -1) 
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

void printargs(char** args)
{
    int i = 0;
    while(args[i] != NULL)
    {
        printf("%s\n", args[i++]);
    }
}

void shellPrompt(){
    // We print the prompt in the form "<user>@<host> <cwd> >"
    char hostn[1204] = "";
    gethostname(hostn, sizeof(hostn));
    printf("%s@%s %s > ", getenv("LOGNAME"), hostn, getcwd((char*) calloc(1024, sizeof(char)), 1024));
}

int main(int argc, char* argv[])
{
    char line[MAXLEN];
    char* args[MAXARGS];

    while(1)
    {
        // printf("@> ");
        shellPrompt();
        fgets(line, MAXLEN, stdin);     // read line from user
        // printf("line is %s\n", line);
        args[0] = strtok(line, " \n\t");
        // printf("arg is %s\n", args[0]);
        int i = 1;
        while( (args[i] = strtok(NULL, " \n\t")) != NULL) ++i;
        // printargs(args);
        execute(args);
    }
}
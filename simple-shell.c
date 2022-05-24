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
    if(pid < 0) return printf("Child could not be created.\n");
    else if(pid == 0)    // child code
    {
        if( execvp(args[0], args) == -1)
        {
            printf("%s: command not found\n", args[0]);
            fflush(stdout);
            kill(getpid(), SIGTERM);
        }
    }
    else                // parent code
    {
        waitpid(pid, NULL, 0);
    }
}

int main(int argc, char* argv[])
{
    char line[MAXLEN];
    char* args[MAXARGS];

    while(1)
    {
        printf("@> ");
        fgets(line, MAXLEN, stdin);     // read line from user
        if( (args[0] = strtok(line, " \n\t")) == NULL) continue;
        int i = 1;
        while( (args[i] = strtok(NULL, " \n\t")) != NULL) ++i;
        execute(args);
    }
}
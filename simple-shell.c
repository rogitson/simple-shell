#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
	return;
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

void execute(char** args)
{
    if(strcmp(args[0], "exit") == 0) exit(0);
    else if(strcmp(args[0], "clear") == 0) system("clear");
    else if(strcmp(args[0], "cd") == 0)  cd(args);
    else if(strcmp(args[0], "pwd") == 0) printf("%s\n", getcwd(currentDirectory, 1024));
    else launch(args);
    
}

int main(int argc, char* argv[])
{
    char line[MAXLEN];
    char* args[MAXARGS];

    while(true)
    {
        prompt();
        fgets(line, MAXLEN, stdin);     // read line from user
        if( (args[0] = strtok(line, " \n\t")) == NULL) continue;
        int i = 1;
        while( (args[i] = strtok(NULL, " \n\t")) != NULL) ++i;
        execute(args);
    }
}
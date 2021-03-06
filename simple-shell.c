#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAXLEN 1024
#define MAXARGS 256
#define true 1
#define false !true

void execute(char **args);
void sigHandlerInt(int p);
void sigHandlerChild(int p);

pid_t pid = -10; // initialize pid to pid that is not possible
char *currentDirectory;
struct sigaction act_int;
struct sigaction act_child;
int bProcFlag = false;

void init()
{
    printf("\n\n\t\t-----------------\n\t\t|  Saeed Shell  |\n\t\t-----------------\n\n");
    // Get the current directory that will be used in different methods
    currentDirectory = (char *)calloc(1024, sizeof(char));

    act_child.sa_flags = SA_RESTART;
    act_child.sa_handler = sigHandlerChild;
    act_int.sa_handler = sigHandlerInt;
    sigaction(SIGCHLD, &act_child, 0);
    sigaction(SIGINT, &act_int, 0);
}

// Signal handler for SIGINT
void sigHandlerInt(int p)
{
    // We send a SIGTERM signal to the child process
    if (kill(pid, SIGTERM) == 0)
        printf("\nProcess %d received a SIGINT signal\n", pid);
    else
        printf("\n");
}

void sigHandlerChild(int p)
{
    //WNOHANG is a non blocking call that assures
    //no blocking happens when cleaning child processes
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

void prompt()
{
    // We print the prompt in the form "<user>@<host> <cwd> >"
    char hostn[1204] = "";
    gethostname(hostn, sizeof(hostn));
    printf("%s@%s %s > ", getenv("LOGNAME"), hostn, getcwd(currentDirectory, 1024));
}

void cd(char **args)
{
    // If we write no path (only 'cd'), then go to the home directory
    if (args[1] == NULL)
        chdir(getenv("HOME"));
    // Else we change the directory to the one specified by the argument, if possible
    else if (chdir(args[1]) == -1)
        printf("%s: no such directory\n", args[1]);
}

void redirection(char *args[], char *inputFile, char *outputFile, int option)
{

    int fd; // between 0 and 19, describing the output or input file

    if ((pid = fork()) == -1)
    {
        printf("Child process could not be created\n");
        return;
    }

    if (pid == 0)
    {
        // Option 0: input redirection
        if (option == 0)
        {
            fd = open(inputFile, O_RDONLY, 0600);
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        // Option 1: output redirection
        else if (option == 1)
        {
            fd = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        // Option 2: Append
        else if (option == 2)
        {
            fd = open(outputFile, O_CREAT | O_RDWR | O_APPEND, 0600);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        // Option 3: input and output redirection
        else if (option == 3)
        {
            fd = open(inputFile, O_RDONLY, 0600);
            dup2(fd, STDIN_FILENO);
            close(fd);

            fd = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600);
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        // setenv("parent", getcwd(currentDirectory, 1024), 1);

        if (execvp(args[0], args) == -1)
        {
            printf("%s: Redirection Command Error\n", args[0]);
            fflush(stdout);
            kill(getpid(), SIGTERM);
        }
    }
    waitpid(pid, NULL, 0);
}

void piper(char **args)
{
    int end = true;
    int desEven[2], desOdd[2]; // file descriptors
    int cmd_num = 1;
    char *command[256];

    for (int i = 0; args[i] != NULL; ++i)
        if (strcmp(args[i], "|") == 0)
            ++cmd_num;

    for (int i = 0, cmd = 0; end; ++i, ++cmd)
    {
        for (int j = 0; strcmp(args[i], "|") != 0; ++j)
        {
            command[j] = args[i++];
            command[j + 1] = NULL;
            if (args[i] == NULL)
            {
                end = false;
                break;
            }
        }

        if (cmd % 2 != 0)
            pipe(desOdd); // for odd cmd
        else
            pipe(desEven); // for even cmd

        pid = fork();
        if (pid < 0)
        {
            if (cmd != cmd_num - 1)
            {
                if (cmd % 2 != 0)
                    close(desOdd[1]); // for odd cmd
                else
                    close(desEven[1]); // for even cmd
            }
            printf("Child could not be created.\n");
            return;
        }
        if (pid == 0) // child code
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
                if (cmd_num % 2 != 0) // for odd number of commands
                    dup2(desOdd[0], STDIN_FILENO);
                else // for even number of commands
                    dup2(desEven[0], STDIN_FILENO);
                // If we are in a command that is in the middle, we will
                // have to use two pipes, one for input and another for
                // output. The position is also important in order to choose
                // which file descriptor corresponds to each input/output
            }
            else
            {
                if (cmd % 2 != 0) // for odd cmd
                {
                    dup2(desEven[0], STDIN_FILENO);
                    dup2(desOdd[1], STDOUT_FILENO);
                }
                else // for even cmd
                {
                    dup2(desOdd[0], STDIN_FILENO);
                    dup2(desEven[1], STDOUT_FILENO);
                }
            }

            execute(command);        // execute command in subshell
            kill(getpid(), SIGTERM); // kill current child
        }
        // parent code
        if (cmd == 0)
            close(desEven[1]);
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

void launch(char **args)
{
    pid = fork();
    if (pid < 0)
    {
        printf("Child could not be created.\n");
        return;
    }
    else if (pid == 0) // child code
    {
        // We set the child to ignore SIGINT signals (we want the parent
        // process to handle them with signalHandler_int)
        signal(SIGINT, SIG_IGN);

        if (execvp(args[0], args) == -1)
        {
            printf("%s: command not found\n", args[0]);
            fflush(stdout);
            kill(getpid(), SIGTERM); // kill current child
        }
    }

    if (!bProcFlag)
        waitpid(pid, NULL, 0);
    else
    {
        printf("Background Process Created with ID %d\n", pid);
        bProcFlag = false;
    }
}

void execute(char **args)
{
    char *cmds[256];

    for (int i = 0; args[i] != NULL; ++i) //detect special characters
    {

        //Background Process
        if (strcmp(args[i], "&") == 0)
        {
            bProcFlag = 1;
            cmds[i] = NULL;
            launch(cmds);
            return;
        }

        // Redirection check
        if (strcmp(args[i], "<") == 0 || strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0)
        {
            if (args[i + 1] == NULL)
            {
                printf("Error in Redirection\n");
                return;
            }
        }

        // detect piping
        if (strcmp(args[i], "|") == 0)
        {
            piper(args);
            return;
        }

        // redirection
        else if (strcmp(args[i], "<") == 0)
        {

            if (args[i + 2] == NULL)
            {
                redirection(cmds, args[i + 1], NULL, 0); // input
                return;
            }

            else if (strcmp(args[i + 2], ">") == 0)
            {
                if (args[i + 3] == NULL)
                {
                    printf("Error in Redirection\n");
                    return;
                }
                redirection(cmds, args[i + 1], args[i + 3], 3); // input and output
                return;
            }
        }
        else if (strcmp(args[i], ">") == 0) // output
        {
            redirection(cmds, NULL, args[i + 1], 1);
            return;
        }
        else if (strcmp(args[i], ">>") == 0) // append
        {
            redirection(cmds, NULL, args[i + 1], 2);
            return;
        }
        cmds[i] = args[i];
        cmds[i + 1] = NULL;
    }
    if (strcmp(args[0], "exit") == 0)
        exit(0);
    else if (strcmp(args[0], "cd") == 0)
        cd(args);
    // else if (strcmp(args[0], "clear") == 0)
    //     system("clear");
    // else if (strcmp(args[0], "pwd") == 0)
    //     printf("%s\n", getcwd(currentDirectory, 1024));
    else
        launch(args);
}

int main(int argc, char *argv[])
{
    init();

    char line[MAXLEN];
    char *args[MAXARGS] = {NULL};

    while (true)
    {
        prompt();
        memset(line, '\0', MAXLEN); // empty line buffer
        fgets(line, MAXLEN, stdin); // read line from user
        if ((args[0] = strtok(line, " \n\t")) == NULL)
            continue;
        for (int i = 1; (args[i] = strtok(NULL, " \n\t")) != NULL; ++i)
            ;
        execute(args);
    }

    return 0;
}
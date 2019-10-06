#include <stdio.h> 
#include <stdlib.h> 
#include <sys/types.h> 
#include <sys/wait.h>
#include <unistd.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define MAXLINE 80 /* The maximum length cmd */

#define NORMAL 0
#define CONCURRENT 1
#define REDIRECTION_IN 2
#define REDIRECTION_OUT 3
#define PIPE 4

char file_name[80];

void split(char *par,char *args[]){
    char* tmp;
    tmp = (char*) malloc (strlen(par));
    strcpy(tmp, par);

    args[0] = strtok(tmp, " ");
    int i = 0;
    while(args[i] != NULL){
        i++;
        args[i] = strtok(NULL, " ");
    }
    args[i] = NULL;
    //free(tmp);
}

/*
NORMAL: 
    + child: execute
    + parent: wait
&:
    + child: execute
    + parent: continue
>: 
    + child: create file -> execute -> write to file
    + parent: wait 
<: 
    + child: open file -> read file -> execute
    + parent: wait
|: 
    + parent: wait
    + child: execute 1st -> pipe -> wait
    + child of child: get input from 1st -> execute 2nd
*/

void handle(int mode, char *args[], char *pipe[]) {
    pid_t pid;
    pid = fork();

    if (pid == 0) {
        int fd;
        if (mode == REDIRECTION_IN) {
            fd = open(file_name, O_RDONLY, 0);
            dup2(fd, STDIN_FILENO);
        } 
        else 
        if (mode == REDIRECTION_OUT) {
            fd = creat(file_name, 0644);
            dup2(fd, STDOUT_FILENO);
        }

        close(fd);
        execvp(args[0],args);
    } else {                   
        wait(NULL);
    }
}

/*
convert command => args
!!: -> do nothing, execute previous command
>, <: get file name, remove "<" or ">" and file name
|: remove "|", move 2nd command to 2nd args
*/
int inputProcessing(char *cmd, char *args[], char *(*pipe_args[]))
{
    int mode = NORMAL;
    if (strcmp(cmd, "!!") == 0)
        return mode;   

    split(cmd,args);

    int i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], "<") == 0) {
            mode = REDIRECTION_IN;
            strcpy(file_name, args[i + 1]);
            args[i] = NULL;
            break;
        } 
        else 
        if (strcmp(args[i], ">") == 0) {
            mode = REDIRECTION_OUT;
            strcpy(file_name, args[i + 1]);
            args[i] = NULL;
            break;
        }
        else
        if (strcmp(args[i], "|") == 0)
        {
            mode = PIPE; 
            pipe_args = &(args) + i + 1;
            args[i] = NULL;
            break;
        }
        else 
        if (strcmp(args[i], "&") == 0)
        {
            mode = CONCURRENT;
            args[i] = NULL;
            break;
        }
        
        i++;
    }
    return mode;
}


int main(void)
{
    char cmd[80];
    char *args[MAXLINE/2 + 1]; /* command line arguments */
    char *(*pipe_args[MAXLINE/2 + 1]);
    int should_run = 1; /* flag to determine when to exit program */

    while (should_run) {
        printf("osh>");

        fgets(cmd, MAXLINE, stdin);
        cmd[strlen(cmd) - 1] = '\0';

        int mode = inputProcessing(cmd, args, pipe_args);
        handle(mode, args, *pipe_args);
    }
    return 0;
}
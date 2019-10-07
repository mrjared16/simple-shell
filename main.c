#include <stdio.h> 
#include <stdlib.h> 
#include <sys/types.h> 
#include <sys/wait.h>
#include <unistd.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define MAXLINE 80 /* The maximum length cmd */
#define MAX_FILENAMELENGTH 80

#define ERROR 0
#define SUCCESS 1

#define INIT 0
#define NORMAL 1
#define CONCURRENT 2
#define REDIRECTION_IN 3
#define REDIRECTION_OUT 4
#define PIPE 5
#define EXIT 6

#define IN 1
#define OUT 0


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
    + child: create file -> stdout = file -> execute
    + parent: wait 
<: 
    + child: open file -> stdin = file -> execute
    + parent: wait
|: 
    + parent: wait
    + child: pipe -> fork -> pipe_in = stdout -> execute -> wait
    + child of child: stdin = pipe_out -> execute
*/

void handle(int mode, char *args[], char **pipe_args, char *file_name) {
    pid_t pid1, pid2, pid3;
    pid1 = fork();

    if (pid1 == 0) {
        if (mode == PIPE)
        {
            int fd[2];
            pipe(fd);
            pid2 = fork();

            //2nd process
            if (pid2 == 0)
            {
                dup2(fd[OUT], STDIN_FILENO);
                close(fd[IN]);
                close(fd[OUT]);
                execvp(pipe_args[0], pipe_args);
            }
            else {
                // 1st process
                pid3 = fork();
                if (pid3 == 0)
                {
                    dup2(fd[IN], STDOUT_FILENO);
                    close(fd[OUT]);
                    close(fd[IN]);
                    execvp(args[0], args);
                }
                else {
                    close(fd[IN]);
                    close(fd[OUT]);
                    waitpid(-1, NULL, 0);
                    waitpid(-1, NULL, 0); 
                }
            }
        }
        else {
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
            execvp(args[0], args);
        }
    } else {                   
        if (mode != CONCURRENT)
            waitpid(pid1, NULL, 0);
    }
}

/*
convert command => args
!!: -> do nothing, execute previous command
>, <: get file name, remove "<" or ">" and file name
|: remove "|", move 2nd command to pipe_args
*/
int inputProcessing(char *cmd, int *mode, char *args[], char ***pipe_args, char *file_name, char *history)
{
    if (strcmp(cmd, "") == 0)
    {
        printf("Please input command!\n");
        return ERROR;
    }

    if (strcmp(cmd, "!!") == 0)
    {
        if (*mode == INIT)
        {
            printf("No commands in history.\n");
            return ERROR;
        }
        printf("%s\n", history);
        return SUCCESS;   
    }

    if (strcmp(cmd, "exit") == 0)
    {
        *mode = EXIT;
        return SUCCESS;
    }

    *mode = NORMAL;
    strcpy(history, cmd);
    split(cmd,args);

    int i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], "<") == 0) {
            *mode = REDIRECTION_IN;
            strcpy(file_name, args[i + 1]);
            args[i] = NULL;
            break;
        } 
        else 
        if (strcmp(args[i], ">") == 0) {
            *mode = REDIRECTION_OUT;
            strcpy(file_name, args[i + 1]);
            args[i] = NULL;
            break;
        }
        else
        if (strcmp(args[i], "|") == 0)
        {
            *mode = PIPE; 
            *pipe_args = &args[i + 1];
            args[i] = NULL;
            break;
        }
        else 
        if (strcmp(args[i], "&") == 0)
        {
            *mode = CONCURRENT;
            args[i] = NULL;
            break;
        }
        
        i++;
    }
    return SUCCESS;
}


int main(void)
{
    char cmd[80], history_buffer[80];
    char *args[MAXLINE/2 + 1]; 
    char **pipe_args = NULL;
    char file_name[MAX_FILENAMELENGTH];
    int mode = INIT;
    int run = 1; 

    while (run) {
        printf("osh>");

        fgets(cmd, MAXLINE, stdin);
        cmd[strlen(cmd) - 1] = '\0';

        if (inputProcessing(cmd, &mode, args, &pipe_args, file_name, history_buffer) != ERROR)
        {
            if (mode == EXIT)
            {
                run = 0;
            }
            else
                handle(mode, args, pipe_args, file_name);
        }
    }
    return 0;
}
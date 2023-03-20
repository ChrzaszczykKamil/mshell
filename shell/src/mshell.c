#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "builtins.h"
#include "config.h"
#include "siparse.h"
#include "utils.h"

volatile int numOfChildren = 0; //only fg children
int childrenInFG[1000]; //list of fg processes, that we need to wait for (ends with NULL)

struct procStat{
    int type;
    int status;
    int pid;
} BGProcess[1000];
int lastDead = 0;

sigset_t blockedSIGCHLD;
sigset_t emptySet;
struct sigaction handlerAction;
struct sigaction sigchldHandler;
struct sigaction sigintHandler;

void handler (int);

void printDeadChildren();

void readLine(char *, int);

void executeLine(char *);

int main(int argc, char *argv[]) {
    sigaddset(&blockedSIGCHLD, SIGCHLD);
    handlerAction.sa_handler = handler;
    sigintHandler.sa_handler = SIG_IGN;
    handlerAction.sa_flags = 0;
    sigintHandler.sa_flags = 0;
    sigemptyset(&emptySet);
    sigemptyset(&blockedSIGCHLD);
    sigemptyset(&handlerAction.sa_mask);
    sigemptyset(&sigintHandler.sa_mask);
    sigaction(SIGCHLD, &handlerAction, NULL);
    sigaction(SIGINT, &sigintHandler, &sigchldHandler);

    childrenInFG[0] = -1;


    int isTerminal;

    char buf[MAX_LINE_LENGTH + 1];

    struct stat sb;
    if (fstat(0, &sb) == -1) {
        perror("fstat");
        exit(EXIT_FAILURE);
    }
    isTerminal = S_ISCHR(sb.st_mode);
    int inputLength;

    do{
        if(isTerminal){ 
            printDeadChildren();
            write(1, PROMPT_STR, strlen(PROMPT_STR));
        }

        do {
            inputLength = read(0, buf, MAX_LINE_LENGTH);
        }while(inputLength==-1);
        if(inputLength==0)
            return 0;

        readLine(buf, inputLength);
    } while (1);//splitter breaks if it's not 1
    return 0;
}

void readLine(char *buf, int inputLength) {
    buf[inputLength] = '\0';
    char *prevLine = buf;
    char *nextLine;
    int lineLength;

    while(inputLength > 0){
        nextLine = strchr(prevLine,'\n');
        if(nextLine != NULL){
            nextLine[0] = '\0';
            inputLength--;
        } else {
            //move it back to start
            memmove(buf, prevLine, inputLength);
            prevLine = buf;

            int inputBefore = inputLength;
            inputLength += read(0, prevLine +  inputLength, MAX_LINE_LENGTH - inputLength);
            if (inputLength<=inputBefore)
                exit(0);

            if(strchr(prevLine,'\n') == NULL && inputLength == MAX_LINE_LENGTH){
                write(2, SYNTAX_ERROR_STR, strlen(SYNTAX_ERROR_STR));
                write(2, "\n", 1);

                while(inputLength > 0){
                    //get rid of unwanted things
                    inputLength=0;
                    memmove(buf, prevLine, inputLength);
                    prevLine = buf;

                    inputLength = read(0, prevLine, MAX_LINE_LENGTH);
                    if (inputLength <= 0)
                        exit(0);

                    if (strchr(prevLine, '\n') != NULL) {
                        break;
                    }
                }
                nextLine = strchr(prevLine,'\n');
                nextLine[0] = '\0';
                inputLength--;
                inputLength -= (nextLine - prevLine);
                prevLine = nextLine + 1;
            }
            continue;
        }
        lineLength = nextLine - prevLine;

        if(lineLength > 0 && prevLine[0] != '#'){
            executeLine (prevLine);
        }

        inputLength -= (lineLength);
        prevLine = nextLine + 1;
    }
}

void executeLine(char *buf) {
    pipelineseq *ln = parseline(buf);
    if (ln) {//parse successful

        pipelineseq *ps = ln;
        do {//Execute each pipeline
            sigprocmask(SIG_BLOCK, &blockedSIGCHLD, NULL);

            //Count commands
            int numOfCommands = 0;
            commandseq *cmmndsq = ps->pipeline->commands;
            commandseq *firstcmmndsq = ps->pipeline->commands;
            do {
                numOfCommands++;
                cmmndsq = cmmndsq->prev;
            } while (cmmndsq != firstcmmndsq);
            //printf("%d\n", numOfCommands);
            int currCommand=0;

            int fd[2];
            int fd2[2];
            int FGchild = 0;
            do {
                currCommand++;
                command *pcmd = cmmndsq->com;
                if (!pcmd) {
                    cmmndsq=cmmndsq->next;
                    continue;
                }
                //Create args array
                char *test = pcmd->args->arg;
                //Check whether it's custom or not
                int customCommand = 0;
                for (int i = 0; i < 5; i++) {
                    if (!strcmp(test, builtins_table[i].name)) {
                        int argsCount = 0;
                        argseq *argseq = pcmd->args;
                        do {//count all the arguments
                            argseq = argseq->next;
                            argsCount++;
                        } while (argseq != pcmd->args);
                        char **args = malloc((argsCount + 1) * sizeof(char *));
                        for (int i = 0; i < argsCount; i++) {
                            args[i] = argseq->arg;
                            argseq = argseq->next;
                        }
                        args[argsCount] = NULL;
                        builtins_table[i].fun(args);
                        customCommand = 1;
                    }
                }

                //it's not custom
                if (customCommand) {
                    cmmndsq=cmmndsq->next;
                    continue;
                }
                fd[0]=fd2[0];
                fd[1]=fd2[1];
                if(currCommand<numOfCommands)
                    pipe(fd2);

                int pid = fork();
                if (pid == -1)//CAN'T FORK FOR SOME UNKNOWN REASON
                    printf("can't fork, error occurred\n");
                else if (pid == 0) {//THE CHILD:
                    sigaction(SIGINT, &sigchldHandler, NULL);
                    if(ps -> pipeline -> flags == INBACKGROUND)
                        setsid();
                    sigprocmask(SIG_UNBLOCK, &blockedSIGCHLD, NULL);



                    if(numOfCommands>1) {
                        if(currCommand==1) {
                            dup2(fd2[1], STDOUT_FILENO);
                            close(fd2[0]);
                            close(fd2[1]);
                        }
                        if (currCommand > 1 && currCommand < numOfCommands) {
                            dup2(fd[0], STDIN_FILENO);
                            dup2(fd2[1], STDOUT_FILENO);
                            close(fd[0]);
                            close(fd[1]);
                            close(fd2[0]);
                            close(fd2[1]);
                        }
                        if (currCommand == numOfCommands) {
                            dup2(fd[0], STDIN_FILENO);
                            close(fd[0]);
                            close(fd[1]);
                        }
                    }

                    //arguments
                    int argsCount = 0;
                    argseq *argseq = pcmd->args;
                    do {//count all the arguments
                        argseq = argseq->next;
                        argsCount++;
                    } while (argseq != pcmd->args);
                    char **args = malloc((argsCount + 1) * sizeof(char *));
                    for (int i = 0; i < argsCount; i++) {
                        args[i] = argseq->arg;
                        argseq = argseq->next;
                    }
                    args[argsCount] = NULL;

                    //redirects
                    redirseq *redirs = pcmd->redirs;
                    if (redirs) {
                        do {
                            if (IS_RIN(redirs->r->flags)) {
                                close(0);
                                if (open(redirs->r->filename, O_RDONLY) == -1) {
                                    write(2, redirs->r->filename, strlen(redirs->r->filename));
                                    if(errno == ENOENT)
                                        write(2,": no such file or directory\n",28);
                                    else if(errno == EACCES)
                                        write(2,": permission denied\n",20);
                                    exit(EXEC_FAILURE);
                                }
                            } else {
                                if (IS_ROUT(redirs->r->flags)) {
                                    close(1);
                                    if (open(redirs->r->filename, O_TRUNC | O_WRONLY | O_CREAT, 0644) == -1) {
                                        write(2, redirs->r->filename, strlen(redirs->r->filename));
                                        if(errno == ENOENT)
                                            write(2,": no such file or directory\n",28);
                                        else if(errno == EACCES)
                                            write(2,": permission denied\n",20);
                                        exit(EXEC_FAILURE);
                                    }
                                }
                                if (IS_RAPPEND(redirs->r->flags)) {
                                    close(1);
                                    if (open(redirs->r->filename, O_WRONLY | O_CREAT | O_APPEND,
                                             0644) == -1) {
                                        write(2, redirs->r->filename, strlen(redirs->r->filename));
                                        if(errno == ENOENT)
                                            write(2,": no such file or directory\n",28);
                                        else if(errno == EACCES)
                                            write(2,": permission denied\n",20);
                                        exit(EXEC_FAILURE);
                                    }
                                }
                            }

                            redirs = redirs->next;
                        } while (redirs != pcmd->redirs);
                    }

                    if (execvp(args[0], args) < 0) {//if there was some error
                        //perror(args[0]);
                        //This function would be enough here, but it can't be used
                        //because tests for some reason expect us to use only small letters
                        write(2, args[0], strlen(args[0]));
                        if(errno == ENOENT)
                            write(2,": no such file or directory\n",28);
                        else if(errno == EACCES)
                            write(2,": permission denied\n",20);
                    }

                    free(args);
                    exit(EXEC_FAILURE);
                }//parent
                if(ps -> pipeline -> flags != INBACKGROUND){
                    childrenInFG[FGchild++] = pid;
                    childrenInFG[FGchild] = -1;
                    numOfChildren++;
                }
                if(currCommand>1){
                    close(fd[0]);
                    close(fd[1]);
                }
                cmmndsq=cmmndsq->next;
            }while(cmmndsq!=firstcmmndsq);

            sigprocmask(SIG_UNBLOCK, &blockedSIGCHLD, NULL);

            //something
            while(numOfChildren > 0) {
                sigsuspend(&emptySet);
            }
            ps = ps->next;
        } while (ps != ln);
    } else {//parse error
        write(2, SYNTAX_ERROR_STR, strlen(SYNTAX_ERROR_STR));
        write(2, "\n", 1);
    }
}


void printDeadChildren(){
    if(lastDead > 0){
        sigprocmask(SIG_BLOCK, &blockedSIGCHLD, NULL);
        for(int i = 0; i < lastDead; i++){
            write(2, "Background process ", 19);
            char pidNumber[10];
            sprintf(pidNumber, "%d", BGProcess[i].pid);
            write(2, pidNumber, strlen(pidNumber));
            write(2, " terminated. ", 13);
            if(BGProcess[i].type)
                write(2, "(exited with status ", 20);
            else
                write(2, "(killed by signal ", 18);

            char procesStatus[10];
            sprintf(procesStatus, "%d", BGProcess[i].status);
            write(2, procesStatus, strlen(procesStatus));
            write(2, ")\n", 2);
        }
        lastDead = 0;
        sigprocmask(SIG_UNBLOCK, &blockedSIGCHLD, NULL);
    }
}


void handler(int signum){
    while(1){
        int stat_loc;
        int pid = waitpid(-1, &stat_loc, WNOHANG);

        if(pid <=0)
            return;

        int inBG=0;
        for(int i=0; i<1000; i++){
            if(childrenInFG[i] == pid)
                inBG = 1;
            if(childrenInFG[i]==-1)
                break;
        }
        if(inBG){
            numOfChildren--;
        }
        else {
            if (WIFEXITED(stat_loc)) {
                BGProcess[lastDead].type = 1;
                BGProcess[lastDead].status = WEXITSTATUS(stat_loc);
                BGProcess[lastDead].pid = pid;
                lastDead++;
            } else if (WIFSIGNALED(stat_loc)) {
                BGProcess[lastDead].type = 0;
                BGProcess[lastDead].status = WIFSIGNALED(stat_loc);
                BGProcess[lastDead].pid = pid;
                lastDead++;
            }
        }
    }
}

/*
 * here's what was changed
 * printf was printing on regular output instead of err
 * perror was using bad capitalization
 * my read was treating console and files completely different, since I made some assumptions about line length
 * so now I treat them the same
 * I thought empty lines are supposed to give errors
 * basically a lot of errors, so read was completely rewritten
 *
 * one wrong flag in redirections
 *
 *
 * empty command used to give syntax error
 *
 * redirections were using file descriptions, so count was increased
 * I had one line commented out for some reason, and I did not close the fd correctly
 *
 * added signals
 */
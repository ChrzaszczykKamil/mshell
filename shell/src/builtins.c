#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include "builtins.h"

int lexit(char*[]);
int lecho(char*[]);
int lcd(char*[]);
int lkill(char*[]);
int lls(char*[]);
int undefined(char *[]);

builtin_pair builtins_table[]={
	{"exit",	&lexit},
	{"lcd",		&lcd},
	{"lkill",	&lkill},
	{"lls",		&lls},
    {"lecho",	&lecho},
	{NULL,NULL}
};

int lecho(char * argv[]) {
    int i =1;
    if (argv[i]) {
        write(1, argv[i], strlen(argv[i]));
        i++;
    }
    while  (argv[i]) {
        write(1, argv[i], strlen(argv[i]));
        i++;
    }

    write(1, "\n", 1);
    return 0;
}

int lcd(char * argv[])
{
    int err=0;
    if(argv[2]) {
        write(2, "Builtin lcd error.\n", strlen("Builtin lcd error.\n"));
        return -1;
    }
    if(argv[1])
        err=chdir(argv[1]);
    else
        err=chdir(getenv("HOME"));
    if(err){
        write(2,"Builtin lcd error.\n",strlen("Builtin lcd error.\n"));
        return -1;
    }
    return 0;
}

int lkill(char * argv[])
{
    int pid;
    int sig;

    if(argv[2]) {
        pid = atoi(argv[2]);
        sig = atoi(argv[1])*(-1);
        if(pid==0||sig==0) {
            write(2,"Builtin lkill error.\n",strlen("Builtin lkill error.\n"));
            return - 1;
        }
    }
    else if(argv[1]){
        pid=atoi(argv[1]);
        if(pid==0) {
            write(2,"Builtin lkill error.\n",strlen("Builtin lkill error.\n"));
            return - 1;
        }
        sig = SIGTERM;
    }else{
        write(2,"Builtin lkill error.\n",strlen("Builtin lkill error.\n"));
    }
    kill(pid, sig);
    return 0;
}

int lexit(char * argv[])
{
    exit(0);
}

int lls(char * argv[])
{
    DIR *dir;
    struct dirent *dp;
    char * file_name;
    if(argv[1])
        dir = opendir(argv[1]);
    else
        dir = opendir(".");

    while ((dp=readdir(dir)) != NULL) {
        if ( !strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..") )
        {} else {
            file_name = dp->d_name;
            if(file_name[0]!='.') {
                write(1, file_name, strlen(file_name));
                write(1, "\n", 1);
            }
        }
    }

    closedir(dir);
    return 0;
}

int undefined(char * argv[])
{
	fprintf(stderr, "Commsand %s undefined.\n", argv[0]);
	return BUILTIN_ERROR;
}

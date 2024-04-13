#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <stdlib.h>

#include "builtins.h"

int echo(char*[]);
int undefined(char *[]);
int shell_exit(char *[]);
int lls(char *[]);
int lcd(char *[]);
int lkill(char *[]);



builtin_pair builtins_table[]={
	{"exit",	&shell_exit},
	{"lecho",	&echo},
	{"lcd",		&lcd},
	{"lkill",	&lkill},
	{"lls",		&lls},
	{NULL,NULL}
};

int is_number(char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return 0;
        }
    }
    return 1;
}

int echo( char * argv[]){	
	int i =1;
	if (argv[i]) printf("%s", argv[i++]);
	while  (argv[i])
		printf(" %s", argv[i++]);

	printf("\n");
	fflush(stdout);
	return 0;
}

int undefined(char * argv[]){
	fprintf(stderr, "Command %s undefined.\n", argv[0]);
	return BUILTIN_ERROR;
}

int compare(const void *a, const void *b) {
    return strcasecmp(*(const char **)a, *(const char **)b);
}

int lls(char * argv[]) {
    if(argv[1] != NULL){
        fprintf(stderr, "Builtin lls error.\n");
        return 1;
    }
    DIR *dir;
    struct dirent *ent;
    int i = 0, n = 0;
    char *files[MAX_FILES] = {NULL};

    if ((dir = opendir(".")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] != '.') {
                files[n++] = strdup(ent->d_name);
            }
        }
        closedir(dir);
    } else {
        fprintf(stderr, "Builtin lls error.\n");
        return 1;
    }

    for (i = 0; i < n; i++) {
        printf("%s\n", files[i]);
        free(files[i]);
    }
    return 0;
}

int lcd(char *argv[]) {
    char *path;
    if(argv[1] != NULL && argv[2] != NULL){
        fprintf(stderr, "Builtin lcd error.\n", 22);
        return 1;
    }
    else if (argv[1] == NULL) {
        path = getenv("HOME");
        if(path == NULL){
            fprintf(stderr, "Builtin lcd error.\n", 22);
            return 1;
        }
    } 
    else {
        path = argv[1];
    }
    if (chdir(path) != 0) {
		fprintf(stderr, "Builtin lcd error.\n", 22);
        return 1;
    }
    return 0;
}

int lkill(char **args) {
    int signal_number = SIGTERM;
    int pid;
    if (args[1] && args[1][0] == '-' && args[2]) {
        if (sscanf(args[1], "-%d", &signal_number) != 1) {
            fprintf(stderr, "Builtin lkill error.\n");
            return 1;
        }
        //errory dla atoi
        if(!is_number(args[2])){
            fprintf(stderr, "Builtin lkill error.\n");
            return 1;
        }
        pid = atoi(args[2]);

    } 
    else if(args[1] && args[2] == NULL) {
        pid = atoi(args[1]);
    }
    if (kill(pid, signal_number) != 0) {
        fprintf(stderr, "Builtin lkill error.\n");
        return 1;
    }
    return 0;
    //co jak wiecej argumentow niz 2 i czy arg[2] nie null
}

int shell_exit(char **args) {
    if (args[1] != NULL) {
		fprintf(stderr, "Builtin 'exit' error.\n", 22);
        return 1;
    }
    exit(0);
}
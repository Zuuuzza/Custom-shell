#include "builtins.h"
#include "config.h"
#include "siparse.h"
#include "utils.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>


typedef struct info_in_b{
    pid_t p;
    int stat;
}info_in_b;

int cnt_in_f;
pid_t jobs_in_f[JOBS_SIZE];

void insert_in_f(pid_t p){
    for(int i = 0; i < JOBS_SIZE; i++){
        if(jobs_in_f[i] == 0){
            jobs_in_f[i] = p;
            cnt_in_f++;
            break;
        }
    }
}

void erase_in_f(pid_t p){
    for(int i = 0; i < JOBS_SIZE; i++){
        if(jobs_in_f[i] == p){
            jobs_in_f[i] = jobs_in_f[cnt_in_f - 1];
            cnt_in_f--;
            break;
        }
    }
}

int find_in_f(pid_t p){
    for(int i = 0; i < JOBS_SIZE; i++){
        if(jobs_in_f[i] == p){
            return 1;
        }
    }
    return 0;
}


info_in_b tab_info[JOBS_SIZE];
int cnt_in_b;

void insert_in_b(pid_t p, int stat){
    for(int i = 0; i < JOBS_SIZE; i++){
        if(tab_info[i].p == 0){
            tab_info[i].p = p;
            tab_info[i].stat = stat;
            cnt_in_b++;
            break;
        }
    }
}

void erase_in_b(pid_t p){
    for(int i = 0; i < JOBS_SIZE; i++){
        if(tab_info[i].p == p){
            tab_info[i].p = tab_info[cnt_in_b - 1].p;
            tab_info[i].stat = tab_info[cnt_in_b - 1].stat;
            cnt_in_b--;
            break;
        }
    }
}

void print_in_b(){
    if(cnt_in_b > 0){
        for(int i = 0; i < JOBS_SIZE; i++){
            if(tab_info[i].p != 0){
                if(WIFSIGNALED(tab_info[i].stat)){
                    char err_buf[MAX_LINE_LENGTH + 100];
                    memset(err_buf, 0, 1);
                    strcat(err_buf, "Background process ");
                    char pidBuf[10];
                    memset(pidBuf, 0, 10);
                    sprintf(pidBuf, "%d", tab_info[i].p);
                    strcat(err_buf, pidBuf);
                    strcat(err_buf, " killed\n");
                    int wr_buf = write(STDERR_FILENO, err_buf, strlen(err_buf));
                    err_write(wr_buf);
                }
                else{
                    char err_buf[MAX_LINE_LENGTH + 100];
                    memset(err_buf, 0, 1);
                    strcat(err_buf, "Background process ");
                    char pidBuf[10];
                    memset(pidBuf, 0, 10);
                    sprintf(pidBuf, "%d", tab_info[i].p);
                    strcat(err_buf, pidBuf);
                    strcat(err_buf, " terminated\n");
                    int wr_buf = write(STDERR_FILENO, err_buf, strlen(err_buf));
                    err_write(wr_buf);
                }
            }
        }
    }
}

void sig_handler(){
    int stat;
    pid_t p = waitpid(-1, &stat, WNOHANG);
    while(p > 0){
        if(find_in_f(p)){
            erase_in_f(p);
        }
        else{
            insert_in_b(p, stat);
        }
        p = waitpid(-1, &stat, WNOHANG);
    }
}

int err_write(int wr_buf) {
    if (wr_buf == -1) {
        write(STDERR_FILENO, WRITE_FAILURE_STR, strlen(WRITE_FAILURE_STR));
        exit(FAIL);
    }
}

int err_exe(int exe_buf, char *buf) {
    if (exe_buf < 0) {
        char err_buf[MAX_LINE_LENGTH + 100];
        memset(err_buf, 0, 1);
        strcat(err_buf, buf);  
        char* space = strchr(err_buf, ' ');
        if (space != NULL) {
            int length = space - err_buf;
            char array[MAX_LINE_LENGTH + 100];
            strncpy(array, err_buf, length);
            array[length] = '\0';  
            strncpy(err_buf, array, length + 1);
        }
        if (errno == ENOENT) {
            strcat(err_buf, ": no such file or directory\n");
            int wr_buf = write(STDERR_FILENO, err_buf, strlen(err_buf));
            err_write(wr_buf);
            exit(EXEC_FAILURE);

        } else if (errno == EACCES) {
            strcat(err_buf, ": permission denied\n");
            int wr_buf = write(STDERR_FILENO, err_buf, strlen(err_buf));
            err_write(wr_buf);
            exit(EXEC_FAILURE);

        } else {
            strcat(err_buf, ": exec error\n");
            int wr_buf = write(STDERR_FILENO, err_buf, strlen(err_buf));
            err_write(wr_buf);
            exit(EXEC_FAILURE);
        }
    }
}

void err_syntax() {
    char err_buf[MAX_LINE_LENGTH + 2 * strlen(SYNTAX_ERROR_STR)];
    memset(err_buf, 0, 1);
    strcat(err_buf, SYNTAX_ERROR_STR);
    strcat(err_buf, "\n");
    int w = write(STDERR_FILENO, err_buf, strlen(err_buf));
    err_write(w);
}

void write_propmt(int is_cmd) {
    if(is_cmd){
        print_in_b();
        int wr_buf = write(STDOUT_FILENO, PROMPT_STR, strlen(PROMPT_STR));
        err_write(wr_buf);
    }
}

pipelineseq *parse_line(char *buf) {
    pipelineseq *ln = parseline(buf);
    if (ln == NULL) {
        err_syntax();
    }
    return ln;
}
void get_arguments(command *com, char *arguments[]) {
    argseq *guard = com->args;
    argseq *current = guard;
    int j = 0;
    do {
        arguments[j] = current->arg;
        current = current->next;
        j++;
    } while (current != guard);
    arguments[j] = NULL;
}

void handle_redirections(redirseq *redirections, char *buf) {
    redirseq *guard = redirections;
    if (redirections != NULL) {
        do {
            redir *redirection = redirections->r;
            if (IS_RIN(redirection->flags)) {
                int op_file = open(redirection->filename, O_RDONLY);
                err_exe(op_file, redirection->filename);
                dup2(op_file, STDIN_FILENO);
                close(op_file);
            } else if (IS_ROUT(redirection->flags) ||
                       IS_RAPPEND(redirection->flags)) {
                int flags = O_WRONLY | O_CREAT;
                if (IS_RAPPEND(redirection->flags)) {
                    flags |= O_APPEND;
                } else {
                    flags |= O_TRUNC;
                }
                int op_file = open(redirection->filename, flags, 0644);
                err_exe(op_file, redirection->filename);
                dup2(op_file, STDOUT_FILENO);
                close(op_file);
            }
            redirections = redirections->next;
        } while (redirections != guard);
    }
}

void executeCommand(pipelineseq *ln, char *buf) {
    pipelineseq *guard_p = ln;
    pipelineseq *current_p = guard_p;
    do {
        pipeline *p = current_p->pipeline;
        commandseq *guard_c = p->commands;
        commandseq *current_c = guard_c;
        int file_descriptors1[2] = {0, 0};
        int file_descriptors2[2] = {0, 0};

        int in_b = 0;
        if(p->flags & INBACKGROUND){
            in_b = 1;
        }

        sigset_t mask;
        sigemptyset(&mask); //czysta maska
        sigaddset(&mask, SIGCHLD); //dodajemy sigchld
        sigprocmask(SIG_BLOCK, &mask, NULL); //blokujemy sigchld

        do {

            int next = 0;
            int prev = 0;
            if (current_c != guard_c) {
                prev = 1;
            }
            if (current_c->next != guard_c) {
                next = 1;
            }
            if (current_c->com == NULL) {
                break;
            }
            // arguments
            char *arguments[JOBS_SIZE];
            get_arguments(current_c->com, arguments);

            // builtins
            char *name = builtins_table[0].name;
            int i = 0, changed = 0;
            while (name != NULL) {
                if (strcmp(name, current_c->com->args->arg) == 0) {
                    int (*fun)(char **) = builtins_table[i].fun;
                    int exeB = fun(arguments);
                    fflush(stdout);
                    changed = 1;
                    break;
                }
                i++;
                name = builtins_table[i].name;
            }

            if (file_descriptors1[0] != 0) {
                close(file_descriptors1[0]);
            }
            if (file_descriptors1[1] != 0) {
                close(file_descriptors1[1]);
            }

            file_descriptors1[0] = file_descriptors2[0];
            file_descriptors1[1] = file_descriptors2[1];

            if (next) {
                pipe(file_descriptors2);
            }

           //not in builtin
            if (!changed) {
                pid_t pid = fork();
                
                if (pid < 0) { // fork failed
                    int w = write(STDERR_FILENO, FORK_FAILURE_STR, strlen(FORK_FAILURE_STR));
                    err_write(w);
                    exit(FAIL);
                } else if (!pid) { // proc is child
                    
                    struct sigaction act;
                    act.sa_handler = SIG_DFL;
                    sigaction(SIGINT, &act, NULL);

                    //into other group
                    if(in_b){
                        setsid();
                    }
                    
                    if (prev) {
                        dup2(file_descriptors1[0], STDIN_FILENO);
                        close(file_descriptors1[0]);
                        close(file_descriptors1[1]);
                    }
                    if (next) {
                        dup2(file_descriptors2[1], STDOUT_FILENO);
                        close(file_descriptors2[0]);
                        close(file_descriptors2[1]);
                    }
                    handle_redirections(current_c->com->redirs, buf);
                    int exeB = execvp(current_c->com->args->arg, arguments);
                    err_exe(exeB, buf);
                }
                else{
                    if(!in_b){
                        insert_in_f(pid);
                    }
                }
            }

            current_c = current_c->next;

        } while (current_c != guard_c);


        if (file_descriptors1[0] != 0) {
            close(file_descriptors1[0]);
        }
        if (file_descriptors1[1] != 0) {
            close(file_descriptors1[1]);
        }
        if (file_descriptors2[0] != 0) {
            close(file_descriptors2[0]);
        }
        if (file_descriptors2[1] != 0) {
            close(file_descriptors2[1]);
        }
        
        sigset_t em;
        sigemptyset(&em);
        if(!(in_b)){
            while(cnt_in_f > 0){
                sigsuspend(&em);
            }
        }

        sigprocmask(SIG_UNBLOCK, &mask, NULL); 

        current_p = current_p->next;
    } while (current_p != guard_p);
}

void readFile(char *buf, int is_cmd) {
    char *line_start = buf;
    size_t numOfReadBytes;
    int cont = 0;
    while ((numOfReadBytes = read(STDIN_FILENO, line_start, BUFSIZE - (line_start - buf)) + line_start - buf) > 0) {
        line_start = buf;
        while (1) {
            char *line_end = memchr(line_start, '\n', numOfReadBytes - (line_start - buf));
            if (line_end == NULL) {
                size_t remaining = (buf + numOfReadBytes) - line_start;
                memmove(buf, line_start, remaining);
                line_start = buf + remaining;
                break;
            }
            *line_end = '\0';
            if (line_end - line_start < MAX_LINE_LENGTH &&
                line_end - line_start > 0) {
                if (line_start[0] != '#') {
                    pipelineseq *ln = parse_line(line_start);
                    executeCommand(ln, line_start);
                }
            } else if (!(line_end - line_start)) {} 
            else {
                err_syntax();
            }
            line_start = line_end + 1;
        }
        write_propmt(is_cmd);
    }
}

int main(int argc, char *argv[]) {
    signal(SIGINT, SIG_IGN);

    int is_cmd = isatty(0);
    char buf[BUFSIZE];
    
    struct sigaction act;
    sigset_t mask_main;
    sigemptyset(&mask_main); 
    sigaddset(&mask_main, SIGCHLD); 
    act.sa_handler = sig_handler;
    act.sa_flags = SA_RESTART;
    act.sa_mask = mask_main;
    sigaction(SIGCHLD, &act, NULL);

    write_propmt(is_cmd);

    readFile(buf, is_cmd);
    return 0;
}

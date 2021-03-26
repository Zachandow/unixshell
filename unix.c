
 
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define READ_END 0 
#define WRITE_END 1

int find_pipe(char** label){
    int index = 0;
    while(label[index] != '\0'){
        if(!strncmp(label[index], "|", 1)){
            return index;
        }
        ++index;
    }
    return -1;
}

char** list(char* initial, char* label[]){
    char* var;
    int amount = 0;
    const char delimiters[2] = {' ', '\t'};

    var = strtok(initial, delimiters);
    label[0] = var;

    char** POINT = malloc(2 * sizeof(char*));
    for(int i = 0; i < 2; i++)
    POINT[i] = malloc(BUFSIZ * sizeof(char));
    POINT[0] = "";
    POINT[1] = "";

    while(var != NULL) {
        var = strtok(NULL, delimiters);
        if(var == NULL) 
            break;
        if(!strncmp(var, ">", 1)) {
            var = strtok(NULL, delimiters);
            POINT[0] = "o";
            POINT[1] = var;
            return POINT; 
        } 
        else if(!strncmp(var, "<", 1)) {
            var = strtok(NULL, delimiters);
            POINT[0] = "i";
            POINT[1] = var;
            return POINT;
        }
        else if(!strncmp(var, "|", 1)) {
            POINT[0] = "p";        
        }
        label[++amount] = var;
    }
    return POINT;
}

int find_pipe(char** label);

int main(int argc, const char* argv[]) {
    char initial [BUFSIZ];
    char cache   [BUFSIZ]; 
    int pipe_fd[2];

    memset(initial, 0, BUFSIZ * sizeof(char));
    memset(cache,   0, BUFSIZ * sizeof(char));

    while(1) {
        printf("osh > ");  
        fflush(stdout);   
        fgets(initial, BUFSIZ, stdin);
        initial[strlen(initial) - 1] = '\0'; 
        if(strncmp(initial, "exit()", 4) == 0)
            return 0;
        if(strncmp(initial, "!!", 2)) 
            strcpy(cache, initial);      
        
        bool do_wait = true;
        char* set = strstr(initial, "&");
        if(set != NULL) {
            *set = ' '; 
            do_wait=false;
            }

        pid_t pid = fork();
        if(pid < 0) {   
            fprintf(stderr, "the fork has now failed.\n");
            return -1; 
        }
        if(pid != 0) {  
            if(do_wait) {
                wait(NULL);
            }
        }
        else {          
            char* label[BUFSIZ];   
            int hist = 0; 
            memset(label, 0, BUFSIZ * sizeof(char));
            if(!strncmp(initial, "!!", 2)) 
                hist = 1;
            char** redirect_to = list((hist ? cache : initial), label);
   
            if(hist && cache[0] == '\0') {
                printf("No command has been recently entered.\n");
                exit(0);
            } 

            if(!strncmp(redirect_to[0], "o", 1)) {
                printf("the output is now saved to ./%s\n", redirect_to[1]);
                int file = open(redirect_to[1], O_TRUNC | O_CREAT | O_RDWR);
                dup2(file, STDOUT_FILENO); 

            } else if(!strncmp(redirect_to[0], "i", 1)) {
                printf("reading from file: ./%s\n", redirect_to[1]);
                int file = open(redirect_to[1], O_RDONLY);

                memset(initial, 0, BUFSIZ * sizeof(char));
                read(file, initial,  BUFSIZ * sizeof(char));

                memset(label, 0, BUFSIZ * sizeof(char));
                initial[strlen(initial) - 1]  = '\0';
                list(initial, label);

            } else if(!strncmp(redirect_to[0], "p", 1)) {
                pid_t pid_child;
                int set_right = find_pipe(label);
                label[set_right] = "\0";
                int x = pipe(pipe_fd);

                if(x < 0){
                    fprintf(stderr, "the pipe has failed \n");
                    return 1;
                }

                char* left[BUFSIZ];
                char* right[BUFSIZ];
                memset(left, 0, BUFSIZ*sizeof(char));
                memset(right, 0, BUFSIZ*sizeof(char));

                for(int i = 0; i < set_right; i++){
                    left[i] = label[i];
                }
                for(int i = 0; i < BUFSIZ; i++){
                    int index = i + set_right + 1;
                    if(label[index] == 0) 
                    break;
                    right[i] = label[index];
                }
                
                pid_child = fork();
                if(pid_child < 0){
                    fprintf(stderr, "the fork has failed...\n");
                    return 1;
                }
                if(pid_child != 0) {
                    dup2(pipe_fd[WRITE_END], STDOUT_FILENO);
                    close(pipe_fd[WRITE_END]);
                    execvp(left[0], left);
                    exit(0); 
                } else {
                    dup2(pipe_fd[READ_END], STDIN_FILENO);
                    close(pipe_fd[READ_END]);
                    execvp(right[0], right);
                    exit(0); 
                }
                wait(NULL);
            }
            execvp(label[0], label);
            exit(0);  
        }
    }
    return 0;
}

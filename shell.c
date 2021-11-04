// Modify this file for your assignment
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "guess.h"
//#define MAX_BUFFER_SIZE 80
#define clear() printf("\033[H\033[J")

// Function setup for built-in 
int shell_cd(char **args);
int shell_help(char **args);
int shell_exit(char **args);

void sigint_handler(int sig){
    write(1, "Terminating through signal handler\n", 35);
    exit(0);
}

char *built_in_command[] = {"cd", "help", "exit"};

int (*built_in_comm_func[])(char**) = {&shell_cd, &shell_help, &shell_exit};

//int num_built_ins = 4;
int num_built_ins(){
   return sizeof(built_in_command)/sizeof(char*);
}


int shell_cd(char **args){
    if(args[1] == NULL){
        fprintf(stderr, " "); 
    }else{
        if(chdir(args[1]) != 0){
            perror("shell"); 
        }
    }
    return 1;
}    

int shell_help(char **args){
    int i;
    printf("The following are built ins:\n");
    for (i=0; i<num_built_ins();i++){
        printf(" %s\n", built_in_command[i]);
    }
    return 1;
}

int shell_exit(char **args){
    return 0;
}

int shell_invoke(char **args){
    pid_t pid;
    int status;

    pid = fork();
    if(pid == 0){
         if (execvp(args[0],args)== -1){
         perror("mini-shell>> ");
         }
         exit(EXIT_FAILURE);
    }else if(pid < 0){
         perror("mini-shell>> "); 
    }else{
         do{
             waitpid(pid, &status, WUNTRACED);
         }while(!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}
//int num_build_ins = 4;
int shell_execute(char **args){
    int i;
    if(args[0] == NULL){
        return 1;
    }
    //parse the command to check if it is built_in command
    //if so, return the corresponding implementing function
    for(i=0; i< num_built_ins(); i++){
        if(strcmp(args[0], built_in_command[i]) == 0){
            return (*built_in_comm_func[i])(args);
        }
    }
    // If the args is not build-ins, passing the args into folking.
    return shell_invoke(args);
}

   
char  *shell_read_line(void){
    #ifdef SHELL_USE_STD_GETLINE
     char *line = NULL;
   ssize_t bufsize = 0;
    if(getline(&line, &bufsize, stdin) == -1){
        if(feof(stdin)){
            exit(EXIT_SUCCESS);
        }else{
            perror("shell: getline\n");
            exit(EXIT_SUCCESS);
        }
    }
    return line;
   #else
   #define MAX_BUFFER_SIZE 80
     int bufferSize = MAX_BUFFER_SIZE;
    int i = 0;
    char* buffer = malloc(sizeof(char) * bufferSize);
   int c;
   if(!buffer){
    fprintf(stderr, "mini-shell: allocation error\n");
    exit(EXIT_FAILURE);
   }
   while(1){
     c = getchar();
     if(c == EOF){
       exit(EXIT_SUCCESS);
     }else if(c == '\n'){
        buffer[i] = '\0';
        return buffer;
     }else{
        buffer[i] = c;
     }
     i++;
     if(i >= bufferSize){
       bufferSize += MAX_BUFFER_SIZE;
       buffer = realloc(buffer, bufferSize);
       if(!buffer){
         fprintf(stderr, "mini-shell: allocation error\n ");
         exit(EXIT_FAILURE);
       }

     }
   }
  #endif
}

#define SHELL_TOKEN_BUFSIZE 8
// #define SHELL_TOKEN_DELIM " \t\r\n\a"
char** shell_split_line(char* line){
 char tok_delim[2]  = " ";
    int bufsize = SHELL_TOKEN_BUFSIZE;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token, **tokens_backup;
   
    token = strtok(line, tok_delim);
    int i = 0;
    while(token != NULL){
         tokens[i] = token;
         i++;
         if(i >= bufsize){
          bufsize += SHELL_TOKEN_BUFSIZE;
          tokens_backup = tokens;
          tokens = realloc(tokens, bufsize * sizeof(char*));
            if(!tokens){
              free(tokens_backup);
              fprintf(stderr, "lsh:allocation error\n");
              exit(EXIT_FAILURE);
            }
         }
         token = strtok(NULL, tok_delim);
    }
    tokens[i] = NULL;
    return tokens;
}

void shell_loop(void){
    char *line;
    char **args;
    int status;
  
    do{
         printf("mini-shell> ");
  
         line = shell_read_line();
         args = shell_split_line(line);
       
         status = shell_execute(args);
   
        free(line);
        free(args);
    }while(status);
    piping(args);
}

int main(int argc, char **argv){
   signal(SIGINT, sigint_handler);
  // printf("You can only terminate by pressing Ctrl+C\n");  
   shell_loop();
 
  guess();
  return EXIT_SUCCESS;

}
#define PIPE_READ 0
#define PIPE_WRITE 1
int piping(char **argv){
    int pipe_fd[2];
    int fd;
    pid_t child1, child2;

    printf("Executing \"ls | wc\"; \n");
    printf("Number of files in current dir is (first # is answer):");
    fflush(stdout);

    if (-1 == pipe(pipe_fd)){
        perror("pipe");
    } 

    child1 = fork();
    if(child1 > 0 ){
       child2 = fork();
    }   
    if(child1 == 0){
       if(-1 == fd){
          perror("dup");
       } 
    fd = dup(pipe_fd[PIPE_WRITE]);
    if(-1 == fd){
       perror("dup");
    }
    if(fd !=STDOUT_FILENO){
        fprintf(stderr, "Pipe output not at STDOUT.\n");
    }
    close(pipe_fd[PIPE_READ]);
    close(pipe_fd[PIPE_WRITE]);

    argv[0] = "ls";
    argv[1] = "NULL";
   
    if(-1 == execvp(argv[0], argv)){
       perror("execvp");
    }
 }else if (child2 == 0){
   if (-1== close(STDIN_FILENO)){
      perror("close");
   }
   fd = dup(pipe_fd[PIPE_READ]);
   if(-1 == fd){
      perror("dup");
   }
   if(fd != STDIN_FILENO){
      fprintf(stderr, "Pipe input not at STDIN.\n");
   }
   close(pipe_fd[PIPE_READ]);
   close(pipe_fd[PIPE_WRITE]);
  
   argv[0] = "wc";
   argv[1] = NULL;

   if(-1 == execvp(argv[0],argv)){
      perror("execvp");
   }
 }else{
   int status;
   close(pipe_fd[PIPE_READ]);
   close(pipe_fd[PIPE_WRITE]);

   if(-1 == waitpid(child1, &status, 0)){
      perror("waitpid");
    }
   if(WIFEXITED(status)==0){
    printf("child1 returned w/ error code %d\n", WEXITSTATUS(status));
   }

   if(-1 == waitpid(child2, &status, 0)){
    perror("waitpid");
   }
   if(WIFEXITED(status)==0){
     printf("child2 returned w/ error code %d\n", WEXITSTATUS(status));
   }
}
return 0;
}

   

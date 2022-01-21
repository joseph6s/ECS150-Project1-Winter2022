#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>

#define CMDLINE_MAX 512

struct parse{
        // a struct contained values
        int redir_index;
        int pipe_index;
        int cmd_len;
        char *command;
        char **argument;
        char *redir_arr;
        char **pipe_arr;
};

int sys_call(struct parse parse_rc,char* cmd);
int stat(const char *filename, struct stat *buf);
struct parse cmd_parse(char cmd[]);

int redirection_detect(char* argu) // whether ">" is appeared in the command line
{    
        if(strstr(argu, "&")) { // stderr case
                return 2;
        }
        else if (strstr(argu, ">")) { // base case
                return 1;
        }
        return 0;
}

int pipeline_detect(char* argu) // whether "|" is appeared in the command line
{    
        if(strstr(argu, "|")) {
                return 1;
        }
        return 0;
}

void pipeline(struct parse parse_rc) // pipeline function
{
        // build a struct parse for store all values passed from commandline
        struct parse *parsed_cmd = (struct parse*)malloc(4*sizeof(struct parse));
        for (int i = 0; i < parse_rc.cmd_len ; i++) { // clean the space
                parsed_cmd[i].argument = (char **) malloc(512*sizeof(char*));
                parsed_cmd[i].command = (char *) malloc(CMDLINE_MAX*sizeof(char));
        }
        for (int i = 0; i < parse_rc.cmd_len ; i++) { // rebuild the argument array
                parsed_cmd[i] = cmd_parse(parse_rc.pipe_arr[i]);
        }
        int i =0 ;
        int fd[2];
        pid_t pid;        
        for (; i < parse_rc.cmd_len-1; i++) {
                pipe(fd);
                pid = fork(); 
                if (pid==0) {
                        dup2(fd[1], STDOUT_FILENO);
                        execvp(parsed_cmd[i].command ,parsed_cmd[i].argument);
                        perror("execvp");
                }
                dup2(fd[0], STDIN_FILENO);
                close(fd[1]);                 
        }
        int status;
        waitpid(pid, &status, 0);
        execvp(parsed_cmd[i].command ,parsed_cmd[i].argument);
        perror("execvp");
}

struct parse cmd_parse(char cmd[]) // process value passed into the struct and assign them to correct variable
{
        struct parse parse_input;       
        // make a copy of cmd
        char cmd_copy[CMDLINE_MAX];
        strcpy(cmd_copy, cmd);
        char *token; // used for strtok
        char* redirection_array[17];
        char* argu_array[17];
        char* pipe_array[17];
        char* cmd_rc = (char *) malloc(CMDLINE_MAX*sizeof(char));  
        int i = 0;
        int j = 0;
        int agrument_len = 0;
        parse_input.cmd_len = 0;

        int pipe_index = pipeline_detect(cmd_copy);
        int redir_index = redirection_detect(cmd_copy);
        if (pipe_index) {
                token = strtok(cmd_copy, "|");
                while( token != NULL ) {
                        pipe_array[i] = token;
                        token = strtok(NULL, "|");
                        i++;
                }
                parse_input.cmd_len = i;        
        } else if (redir_index == 1) {
                // resplit the argu_array
                // make an argu_array copy
                token = strtok(cmd_copy, ">");
                while( token != NULL ) {
                        redirection_array[i] = token;
                        token = strtok(NULL, ">");
                        i++;
                }
                
                // delete the space in the filename
                if(strstr(redirection_array[i-1], " ")) {
                        token = NULL;
                        token = strtok(redirection_array[i-1], " ");
                        redirection_array[i-1] = token;
                }

                // empty the token
                token = NULL;
                token = strtok(redirection_array[0], " ");
                while( token != NULL ) {
                        argu_array[j] = token;
                        token = strtok(NULL, " ");
                        j++;
                }
                cmd_rc = argu_array[0];
                argu_array[j] = NULL;
                agrument_len = j;
        }
        else if (redir_index == 2) {
                // resplit the argu_array
                // make an argu_array copy
                token = strtok(cmd_copy, ">&");
                while( token != NULL ) {
                        redirection_array[i] = token;
                        token = strtok(NULL, ">&");
                        i++;
                }
                
                // delete the space in the filename
                if(strstr(redirection_array[i-1], " ")) {
                        token = NULL;
                        token = strtok(redirection_array[i-1], " ");
                        redirection_array[i-1] = token;
                }

                // empty the token
                token = NULL;
                token = strtok(redirection_array[0], " ");
                while( token != NULL ) {
                        argu_array[j] = token;
                        token = strtok(NULL, " ");
                        j++;
                }
                cmd_rc = argu_array[0];
                argu_array[j] = NULL;
                agrument_len = j;
        } else {
                // split command pushed from user
                token = strtok(cmd_copy, " ");
                while( token != NULL ) {
                        argu_array[i] = token;
                        token = strtok(NULL, " ");
                        i++;
                }
                cmd_rc = argu_array[0];
                agrument_len = i; 
                argu_array[i] = NULL; // add a NULL in the end of the argument array => char *args[] = {"date", NULL} 
        }

        parse_input.argument = (char **) malloc(CMDLINE_MAX*sizeof(char*));
        parse_input.command = (char *) malloc(CMDLINE_MAX*sizeof(char));
        parse_input.pipe_arr = (char **) malloc(CMDLINE_MAX*sizeof(char*));
        parse_input.pipe_index = pipe_index;
        parse_input.redir_index = redir_index;
        strcpy(parse_input.command, cmd_rc);
        for (int x = 0; x < parse_input.cmd_len ; x++) {
                parse_input.pipe_arr[x] = (char *) malloc(CMDLINE_MAX*sizeof(char));
                strcpy(parse_input.pipe_arr[x],pipe_array[x]);
        }
        for (int x = 0; x < agrument_len ; x++) {
                parse_input.argument[x] = (char *) malloc(CMDLINE_MAX*sizeof(char));
                strcpy(parse_input.argument[x],argu_array[x]);
        }
        parse_input.redir_arr = redirection_array[i-1];
        return parse_input;
}

int sys_call(struct parse parse_rc,char*cmd) // defined sys_call function, include built-in function, fork + execute + wait
{
        // Build-in function pwd, cd and sls
        if (!strcmp(parse_rc.command, "pwd")) {
                char cwd[PATH_MAX];
                getcwd(cwd,sizeof(cwd));
                fprintf(stdout, "%s\n",cwd);
                fprintf(stderr, "+ completed '%s' [%d]\n",
                        parse_rc.command, 0);
                return 0;
        } 
        if (!strcmp(parse_rc.command, "cd")) {                     
                int rc = chdir((parse_rc.argument)[1]);
                fprintf(stderr, "+ completed '%s' [%d]\n",
                        cmd,rc);
                return 0;      
        }
        if (!strcmp(parse_rc.command, "sls")) {                     
                char cwd[PATH_MAX];
                getcwd(cwd,sizeof(cwd)); // get current working directory
                DIR *streamp;
                struct dirent *dep;
                struct stat s;
                if (!(streamp = opendir(cwd))) {
                        exit(1);
                }
                while((dep = readdir(streamp)) != NULL) {
                        stat(dep->d_name, &s);
                        printf("%s (%ld bytes)\n", dep->d_name,s.st_size);
                }
                if (closedir(streamp)) {
                        exit(1);
                }
                fprintf(stderr, "+ completed '%s' [%d]\n",
                        cmd,0);
                return 0;      
        }

        /* fork + exec + wait */
        int fd;
        pid_t pid;     
        pid = fork();
        if (pid == 0) {
                // Child process end after execution
                if (parse_rc.redir_index == 1) {
                        fd = open(parse_rc.redir_arr, O_WRONLY| O_CREAT,0644);
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                }
                else if (parse_rc.redir_index == 2) {
                        fd = open(parse_rc.redir_arr, O_WRONLY| O_CREAT,0644);
                        dup2(fd, STDERR_FILENO);
                        close(fd);        
                }
                execvp(parse_rc.command, parse_rc.argument);
                perror("execvp");
                exit(1);
        } else if (pid > 0) {
                // Parent process wait for Child print finish statement
                int status;
                waitpid(pid, &status, 0);
                fprintf(stderr, "+ completed '%s' [%d]\n",
                        parse_rc.command, WEXITSTATUS(status));
        } else {
                perror("fork");
                exit(1);
        }
        return 0;
}

int main(void) 
{
        char cmd[CMDLINE_MAX];
        while (1) {
                char *nl;
                //int retval;

                /* Print prompt */
                printf("sshell$ ");
                fflush(stdout);

                /* Get command line */
                fgets(cmd, CMDLINE_MAX, stdin);

                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO)) {
                        printf("%s", cmd);
                        fflush(stdout);
                }

                /* Remove trailing newline from command line */
                nl = strchr(cmd, '\n');
                if (nl) {
                        *nl = '\0';                        
                }

                /* Builtin command exit */
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        fprintf(stderr, "+ completed '%s' [%d]\n",
                                cmd, 0);
                        break;
                }

                struct parse cmd_line = cmd_parse(cmd);
                // move the whole session of fork+wait+exec into helper function sys_call
                if (cmd_line.pipe_index == 1){
                        pid_t pid;     
                        pid = fork();
                        if (pid == 0) {
                                pipeline(cmd_line);
                                /* Child process
                                // end after execution */

                        } else if (pid > 0) {
                                /* Parent process
                                wait for Child
                                print finish statement
                                */
                                int status;
                                waitpid(pid, &status, 0);
                                fprintf(stderr, "+ completed '%s' [%d]\n",
                                cmd, WEXITSTATUS(status));
                        } else {
                                perror("fork");
                                exit(1);
                        }
                } else {
                        sys_call(cmd_line,cmd);
                }
        }
        return EXIT_SUCCESS;
}
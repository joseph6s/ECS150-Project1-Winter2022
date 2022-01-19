#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>

#define CMDLINE_MAX 512

struct parse{
        int redir_index;
        int pipe_index;
        char *command;
        char **argument;
        char *redir_arr;
        char **pipe_arr;
};

int sys_call(char cmd[]);
void cmd_parse(char cmd[], struct parse *parse_input);

int redirection_detect(char* argu) // whether ">" is appeared in the command line
{    
        if(strstr(argu, ">")){
                return 1;
        }
        return 0;
}

int pipeline_detect(char* argu) // whether ">" is appeared in the command line
{    
        if(strstr(argu, "|")){
                return 1;
        }
        return 0;
}

void pipeline(char* process1, char* process2){
        struct parse proc1;
        struct parse proc2;
        cmd_parse(process1,&proc1);
        cmd_parse(process2,&proc2);
        int fd[2];
        pipe(fd);
        if(fork() != 0){
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);  
                execvp(proc1.command,proc1.argument);
        } else {
                close(fd[1]);
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);
                execvp(proc2.command,proc2.argument);
        }
}



void cmd_parse(char cmd[], struct parse *parse_input) {
        
        // make a copy of cmd
        char cmd_copy[CMDLINE_MAX];
        strcpy(cmd_copy, cmd);
        char *token;
        char* redirection_array[17];
        char* argu_array[17]; // need to be released
        char* pipe_array[17];

        // redirction stdout to specfic file

        int redir_index = 0;
        int i = 0;
        int pipe_index = pipeline_detect(cmd);
        if (pipe_index) {
                token = strtok(cmd_copy, "|");
                while( token != NULL ) {
                        pipe_array[i] = token;
                        token = strtok(NULL, "|");
                        i++;
                }        
        } else if (redirection_detect(cmd_copy)){
                // resplit the argu_array
                // make an argu_array copy
                token = strtok(cmd_copy, ">");
                while( token != NULL ) {
                        redirection_array[i] = token;
                        token = strtok(NULL, ">");
                        i++;
                }
                
                // delete the space in the name of file
                if(strstr(redirection_array[i-1], " ")){
                        token = NULL;
                        token = strtok(redirection_array[i-1], " ");
                        redirection_array[i-1] = token;
                }

                // empty the token
                token = NULL;
                token = strtok(redirection_array[0], " ");
                int j = 0;
                while( token != NULL ) {
                        argu_array[j] = token;
                        token = strtok(NULL, " ");
                        j++;
                }
                argu_array[j] = NULL;

                redir_index = 1;
        } else {
                // split command pushed from user
                token = strtok(cmd_copy, " ");
                while( token != NULL ) {
                        argu_array[i] = token;
                        token = strtok(NULL, " ");
                        i++;
                }
                argu_array[i] = NULL; // add a NULL in the end of the argument array => char *args[] = {"date", NULL} 
        }
        
        parse_input->pipe_index = pipe_index;
        parse_input->redir_index = redir_index;
        parse_input->command = cmd_copy;
        parse_input->argument = argu_array;
        parse_input->redir_arr = redirection_array[i-1];
        parse_input->pipe_arr = pipe_array;

}



int sys_call(char cmd[])
{
        int fd;
        struct parse parse_rc;
        cmd_parse(cmd,&parse_rc);
        // Build-in function pwd and cd
        if (!strcmp(parse_rc.command, "pwd")) {
                char cwd[PATH_MAX];
                getcwd(cwd,sizeof(cwd));
                fprintf(stdout, "%s\n",cwd);
                fprintf(stderr, "+ completed '%s' [%d]\n",
                        cmd, 0);
                return 0;
        } 
        if (!strcmp(parse_rc.command, "cd")) {                     
                int rc = chdir((parse_rc.argument)[1]);
                fprintf(stderr, "+ completed '%s' [%d]\n",
                        cmd, rc);
                return 0;      
        }
        char *process1;
        char *process2;
        /* fork + exec + wait */
        pid_t pid;     
        pid = fork();
        if (pid == 0) {

                /* Child process
                // end after execution */

                if (parse_rc.pipe_index == 1) {
                        process1 = parse_rc.pipe_arr[0];
                        process2 = parse_rc.pipe_arr[1];
                        struct parse proc1;
                        struct parse proc2;
                        cmd_parse(process1,&proc1);
                        cmd_parse(process2,&proc2);
                        int fd[2];
                        pipe(fd);
                        if(fork() != pid){
                                close(fd[0]);
                                dup2(fd[1], STDOUT_FILENO);
                                close(fd[1]);  
                                execvp(proc1.command,proc1.argument);
                                int status;
                                waitpid(pid, &status, 0);
                                fprintf(stderr, "+ completed '%s' [%d]\n",
                                        cmd, WEXITSTATUS(status));
                        } else {
                                close(fd[1]);
                                dup2(fd[0], STDIN_FILENO);
                                close(fd[0]);
                                execvp(proc2.command,proc2.argument);
                        }
                } else if (parse_rc.redir_index == 1) {
                        fd = open(parse_rc.redir_arr, O_WRONLY| O_CREAT,0644);
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                        execvp(parse_rc.command, parse_rc.argument);
                } else {
                        execvp(parse_rc.command, parse_rc.argument);
                }

                perror("execvp");
                exit(1);
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
                if (nl)
                        *nl = '\0';

                /* Builtin command */
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        fprintf(stderr, "+ completed '%s' [%d]\n",
                                cmd, 0);
                        break;
                }

                // move the whole session of fork+wait+exec into helper function sys_call
                sys_call(cmd); // require modified
        }

        return EXIT_SUCCESS;
}
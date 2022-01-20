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
        int cmd_len;
        char *command;
        char **argument;
        char *redir_arr;
        char **pipe_arr;
};

int sys_call(struct parse parse_rc,char* cmd);
struct parse cmd_parse(char cmd[]);

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

void pipeline(struct parse parse_rc){
        struct parse *parsed_cmd = (struct parse*)malloc(4*sizeof(struct parse));
        //struct parse parsed_cmd1 = cmd_parse(parse_rc.pipe_arr[0]);
        //printf("loop: %d, cmd: %s, and %s %s\n",12,parsed_cmd1.command,parsed_cmd1.argument[0],parsed_cmd1.argument[1]);
        //printf("it issss %s\n",parse_rc.pipe_arr[0]);
        //printf("count : %d\n",parse_rc.cmd_len);
        for (int i = 0; i < parse_rc.cmd_len ; i++)
        {
                parsed_cmd[i].argument = (char **) malloc(512*sizeof(char*));
                parsed_cmd[i].command = (char *) malloc(CMDLINE_MAX*sizeof(char));
        }
        for (int i = 0; i < parse_rc.cmd_len ; i++)
        {
                parsed_cmd[i] = cmd_parse(parse_rc.pipe_arr[i]);
                //printf("loop: %d, cmd: %s, and %s %s\n",i,parsed_cmd[i].command,parsed_cmd[i].argument[0],parsed_cmd[i].argument[1]);    
        }
        int i =0 ;
        int fd[2];
        pid_t pid;
        //printf("proce: 2, cmd: %s, and%sand%s\n",parsed_cmd[1].command,parsed_cmd[1].argument[0],parsed_cmd[1].argument[1]);           
        for (; i < parse_rc.cmd_len-1  ; i++)
        {
                pipe(fd);
                //printf("11111111\n");
                pid = fork(); 
                if (pid==0) {
                        //printf("22221\n");
                        //printf("proce: 2, cmd:%s, and%sand%s\n",parse_rc.command,parse_rc.argument[0],parse_rc.argument[1]);
                        dup2(fd[1], STDOUT_FILENO);
                        
                        execvp(parsed_cmd[i].command ,parsed_cmd[i].argument);
                        perror("execvp");
                }
                dup2(fd[0], STDIN_FILENO);
                close(fd[1]);                 
        }
        int status;
        waitpid(pid, &status, 0);
        //printf("33333\n");
        //printf("proce: 2, cmd:%s, and%sand%s\n",parsed_cmd[i].command,parsed_cmd[i].argument[0],parsed_cmd[i].argument[1]);
        execvp(parsed_cmd[i].command ,parsed_cmd[i].argument);
        perror("execvp");

}


struct parse cmd_parse(char cmd[]) {
        struct parse parse_input;       
        // make a copy of cmd
        char cmd_copy[CMDLINE_MAX];
        strcpy(cmd_copy, cmd);
        char *token;
        char* redirection_array[17];
        char* argu_array[17]; // need to be released
        char* pipe_array[17];

        // redirction stdout to specfic file
        parse_input.cmd_len = 0;
        int redir_index = 0;
        int i = 0;
        int j = 0;
        int agrument_len = 0;
        char* cmd_rc = (char *) malloc(CMDLINE_MAX*sizeof(char));
        int pipe_index = pipeline_detect(cmd);
        if (pipe_index) {
                token = strtok(cmd_copy, "|");
                while( token != NULL ) {
                        pipe_array[i] = token;
                        token = strtok(NULL, "|");
                        i++;
                }
                parse_input.cmd_len = i;        
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
                
                while( token != NULL ) {
                        argu_array[j] = token;
                        token = strtok(NULL, " ");
                        j++;
                }
                cmd_rc = argu_array[0];
                argu_array[j] = NULL;
                agrument_len = j;
                redir_index = 1;
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
        //printf("=========arr len %d\n",agrument_len);
        //printf("=========cmd len %d\n",parse_input.cmd_len);
        parse_input.pipe_index = pipe_index;
        parse_input.redir_index = redir_index;
        strcpy(parse_input.command, cmd_rc);
        for (int x = 0; x < parse_input.cmd_len ; x++)
        {
                parse_input.pipe_arr[x] = (char *) malloc(CMDLINE_MAX*sizeof(char));
                strcpy(parse_input.pipe_arr[x],pipe_array[x]);
        }
        for (int x = 0; x < agrument_len ; x++)
        {
                //printf("=========arr len\n");
                parse_input.argument[x] = (char *) malloc(CMDLINE_MAX*sizeof(char));
                //printf("=========1111arr len\n");
                strcpy(parse_input.argument[x],argu_array[x]);
        }
        parse_input.redir_arr = redirection_array[i-1];
        //printf("proce: =====, cmd:%s, and%sand%s\n",parse_input.command,parse_input.argument[0],parse_input.argument[1]);
        return parse_input;

}



int sys_call(struct parse parse_rc,char*cmd)
{
        int fd;
        // Build-in function pwd and cd
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
        //char *process1;
        //char *process2;
        /* fork + exec + wait */
        pid_t pid;     
        pid = fork();
        if (pid == 0) {
                /* Child process
                // end after execution */
                        //printf("proce: 2, cmd:%s, and%sand%s\n",parse_rc.command,parse_rc.argument[0],parse_rc.argument[1]);
                        if (parse_rc.redir_index == 1) {
                        fd = open(parse_rc.redir_arr, O_WRONLY| O_CREAT,0644);
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                        }
                        
                        execvp(parse_rc.command, parse_rc.argument);
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
                if (nl)
                        *nl = '\0';

                /* Builtin command */
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        fprintf(stderr, "+ completed '%s' [%d]\n",
                                cmd, 0);
                        break;
                }
                struct parse cmd_line = cmd_parse(cmd);
                //printf("proce: 112121, cmd:%s, and%sand%s\n",cmd_line.command,cmd_line.argument[0],cmd_line.argument[1]);
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
                        
                }
                else{
                        sys_call(cmd_line,cmd);
                }
                        

        }
        return EXIT_SUCCESS;
}
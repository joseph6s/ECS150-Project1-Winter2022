#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>

#define CMDLINE_MAX 512

int redirection_detect(char* argu) // whether ">" is appeared in the command line
{    
        if(strstr(argu, ">")){
                printf("redir\n");
                return 1;
        }
        return 0;
}

int sys_call(char cmd[])
{
        // make a copy of cmd
        char cmd_copy[CMDLINE_MAX];
        strcpy(cmd_copy, cmd);
        char *token;
        char* redirection_array[17];
        char* argu_array[17]; // need to be released
        
        // phase 4
        // echo a>test.txt // dup2 recover problem // do not stdout sshell
        // echo a > test.txt // '\0' problem
        // need a helper function to split the char[] ?
        // release all temp variable, for next command

        // redirction stdout to specfic file
        int fd;
        int redir_index = 0;
        int i = 0;
        if (redirection_detect(cmd_copy)){
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

                // example cmd : echo a b c > test.txt
                // split with >, get an array of ["echo a b c", "test.txt"]
                // take array[0] as new cmd, array[1] as redirection

                /*
                int j = 0;
                printf("r_array: ");
                while(j < i) {
                        printf("%s\n",redirection_array[j]);
                        j++;
                }
                printf("\n");

                // char* new_argu_array = redirection_array[0];
                printf("new_argu_array: %s\n", redirection_array[0]);
                printf("r_array[i]: %s\n", redirection_array[i-1]);
                */

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

                /*
                j = 0;
                printf("argu_array: ");
                while(j < i) {
                        printf("%s\n",argu_array[j]);
                        j++;
                }
                printf("\n"); 
                */

                redir_index = 1;

                // argu_array[i-1] = NULL;
                // argu_array[i-2] = NULL;
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

        // Build-in function pwd and cd
        if (!strcmp(cmd_copy, "pwd")) {
                char cwd[PATH_MAX];
                getcwd(cwd,sizeof(cwd));
                fprintf(stdout, "%s\n",cwd);
                fprintf(stderr, "+ completed '%s' [%d]\n",
                        cmd, 0);
                return 0;
        } 
        if (!strcmp(cmd_copy, "cd")) {                     
                int rc = chdir(argu_array[1]);
                fprintf(stderr, "+ completed '%s' [%d]\n",
                        cmd, rc);
                return 0;      
        }


        /* fork + exec + wait */
        pid_t pid;     
        pid = fork();
        if (pid == 0) {

                /* Child process
                // end after execution */
                if (redir_index == 1){
                        fd = open(redirection_array[i-1], O_WRONLY| O_CREAT,0644);
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                }            
                execvp(cmd_copy, argu_array); 
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
                        break;
                }



                // move the whole session of fork+wait+exec into helper function sys_call
                sys_call(cmd); // require modified
        }

        return EXIT_SUCCESS;
}
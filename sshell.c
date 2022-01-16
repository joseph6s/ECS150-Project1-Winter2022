#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>

#define CMDLINE_MAX 512

int redirection_detect(char** argu) // whether ">" is appeared in the command line
{
        int i = 0;
        while( argu[i] != NULL )
        {
                if(!strcmp(argu[i], ">")){
                        printf("redir\n");
                        return 1;
                }
                i++;
        }
        return 0;
}

int sys_call(char cmd[])
{
        // make a copy of cmd
        char cmd_copy[CMDLINE_MAX];
        strcpy(cmd_copy, cmd);

        /* split command pushed from user to */
        char *token;
        token = strtok(cmd_copy, " ");
        
        char* argu_array[17]; // need to be released
        int i = 0;

        while( token != NULL ) {
                argu_array[i] = token;
                token = strtok(NULL, " ");
                i++;
        }
        argu_array[i] = NULL; // add a NULL in the end of the argument array => char *args[] = {"date", NULL} 
        
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
        
        //redirction stdout to specfic file
        int fd;
        int fd2;
        if (redirection_detect(argu_array)){
                
                fd = open(argu_array[i-1],
                O_WRONLY| O_CREAT,0644);
                dup2(fd, STDOUT_FILENO);
                close(fd);
                argu_array[i-1] = NULL;
                argu_array[i-2] = NULL;
        }                    
        /* fork + exec + wait */
        pid_t pid;     

        pid = fork();
        if (pid == 0) {
                /* Child process
                end after execution */                
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

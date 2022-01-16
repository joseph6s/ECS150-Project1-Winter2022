#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#define CMDLINE_MAX 512

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

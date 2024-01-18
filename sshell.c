#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512

int main(void)
{
        char cmd[CMDLINE_MAX];

        while (1) {
                char *nl;
                // int retval; previous was var of system(), no use

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

                /* Instead of using the function system(), 
                modify the program in order to use the fork+exec+wait method */
                pid_t pid;
                char *argv[] = {cmd, NULL};

                pid = fork();
                if (pid == 0) {
                        /* Child */
                        execvp(cmd, argv);
                        printf("Command not found\n");
                        exit(EXIT_FAILURE);
                } 
                
                else if (pid > 0) {
                        /* Parent */
                        int status;
                        wait(&status);

                        if (WIFEXITED(status)) {
                                int exit_status = WEXITSTATUS(status);
                                fprintf(stderr, "+ completed '%s' [%d]\n", cmd, exit_status);
                        }
                } 
                
                else {
                        perror("fork");
                        exit(EXIT_FAILURE);
                }
        }

        return EXIT_SUCCESS;
}
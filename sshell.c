#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512
#define ARG_MAX 16
// #define TOKEN_MAX 32
#define PATH_MAX 1024

struct command
{
        char *program;
        char *args[ARG_MAX];
        int count;
        char *full_command;
};

void parse(struct command *commands, char *cmd_in)
{
        commands->full_command = strdup(cmd_in);
        commands->count = 0;
        char *token = strtok(cmd_in, " ");
        while (token != NULL && commands->count < ARG_MAX)
        {
                if (commands->count == 0)
                {
                        commands->program = token;
                }
                commands->args[commands->count++] = token;
                token = strtok(NULL, " ");
        }
        commands->args[commands->count] = NULL;
}

int main(void)
{
        char cmd[CMDLINE_MAX];

        while (1)
        {
                char *nl;
                // int retval; previous was var of system(), no use

                /* Print prompt */
                printf("sshell$ ");
                fflush(stdout);

                /* Get command line */
                fgets(cmd, CMDLINE_MAX, stdin);

                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO))
                {
                        printf("%s", cmd);
                        fflush(stdout);
                }

                /* Remove trailing newline from command line */
                nl = strchr(cmd, '\n');
                if (nl)
                        *nl = '\0';

                struct command commands;
                parse(&commands, cmd);
                int status;
                int exit_status;

                /* Builtin command */
                // exit command
                if (!strcmp(cmd, "exit"))
                {
                        fprintf(stderr, "Bye...\n");
                        exit_status = 1;
                }

                // pwd command
                else if (!strcmp(cmd, "pwd"))
                {
                        char cwd[PATH_MAX];
                        if (getcwd(cwd, sizeof(cwd)) != NULL)
                        {
                                printf("%s\n", cwd);
                        }
                        else
                        {
                                status = 1;
                        }
                }

                // cd command
                else if (!strcmp(cmd, "cd"))
                {
                        if (commands.count > 1)
                        {
                                if (chdir(commands.args[1]) != 0)
                                {
                                        status = 1;
                                }
                        }
                }
                else
                {
                        pid_t pid;
                        pid = fork();
                        if (pid == 0)
                        {
                                /* Child */
                                execvp(cmd, commands.args);
                                perror("execvp");
                                exit(EXIT_FAILURE);
                        }
                        else if (pid > 0)
                        {
                                /* Parent */
                                wait(&status);
                        }
                        else
                        {
                                perror("fork");
                                exit(EXIT_FAILURE);
                        }
                }

                // print + completed 'command' [status]
                fprintf(stderr, "+ completed '%s' [%d]\n", commands.full_command, WEXITSTATUS(status));

                if (exit_status == 1)
                {
                        break;
                }
        }

        return EXIT_SUCCESS;
}

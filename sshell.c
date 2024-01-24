#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

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
        char *output_file;
        int append;
};

void parse(struct command *commands, char *cmd_in)
{
        commands->full_command = strdup(cmd_in);
        commands->count = 0;
        commands->output_file = NULL;
        commands->append = 0;

        char *output_redirection = strstr(cmd_in, ">>");
        if (output_redirection != NULL)
        {
                *output_redirection = '\0';
                commands->output_file = output_redirection + 2;
                commands->append = 1;
        }
        else
        {
                output_redirection = strchr(cmd_in, '>');
                if (output_redirection != NULL)
                {
                        *output_redirection = '\0';
                        commands->output_file = output_redirection + 1;
                }
        }

        // check white spaces, increase pointer to first char of next token
        if (commands->output_file != NULL)
        {
                while (*commands->output_file == ' ')
                {
                        commands->output_file++;
                }
        }

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
                        status = 0;
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

                                // redirect if output file is specified
                                if (commands.output_file != NULL)
                                {
                                        int fd;
                                        if (commands.append)
                                        {
                                                fd = open(commands.output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
                                        }
                                        else
                                        {
                                                fd = open(commands.output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                                        }
                                        dup2(fd, STDOUT_FILENO);
                                        close(fd);
                                }

                                execvp(cmd, commands.args);
                                perror("execvp");
                                exit(EXIT_FAILURE);
                        }
                        else if (pid > 0)
                        {
                                /* Parent */
                                wait(&status);
                                // dup2(test, STDOUT_FILENO);
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
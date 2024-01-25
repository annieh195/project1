#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>

#define CMDLINE_MAX 512
#define ARG_MAX 16
// #define TOKEN_MAX 32

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

        // append
        char *output_redirection = strstr(cmd_in, ">>");
        if (output_redirection != NULL)
        {
                *output_redirection = '\0';
                commands->output_file = output_redirection + 2;
                commands->append = 1;
        }
        else
        {
                // truncate
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
                int status = 0;
                int exit_status = 0;

                /* Builtin command */
                // exit command
                if (!strcmp(commands.program, "exit"))
                {
                        fprintf(stderr, "Bye...\n");
                        status = 0;
                        exit_status = 1;
                }

                // pwd command
                else if (!strcmp(commands.program, "pwd"))
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
                else if (!strcmp(commands.program, "cd"))
                {
                        if (commands.count > 1)
                        {
                                if (chdir(commands.args[1]) != 0)
                                {
                                        fprintf(stderr, "Error: cannot cd into directory\n");
                                        status = 1;
                                }
                                else
                                {
                                        status = 0;
                                }
                        }
                }

                // sls command
                // Cite: Lecture 03.syscalls.pdf page 32-33
                else if (!strcmp(commands.program, "sls"))
                {
                        struct stat sb;
                        DIR *dirp;
                        struct dirent *dp;

                        dirp = opendir(".");
                        while ((dp = readdir(dirp)) != NULL)
                        {
                                // check and only print if it is directory using stat macro
                                if (stat(dp->d_name, &sb) == 0 && S_ISREG(sb.st_mode))
                                {
                                        printf("%s (%ld bytes)\n", dp->d_name, sb.st_size);
                                }
                        }
                        closedir(dirp);
                        status = 0;
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

                                execvp(commands.program, commands.args);
                                perror("execvp");
                                fprintf(stderr, "Error: command not found\n");
                                exit(EXIT_FAILURE);
                        }
                        else if (pid > 0)
                        {
                                /* Parent */
                                int child_status;
                                wait(&child_status);
                                if (WIFEXITED(child_status))
                                {
                                        status = WEXITSTATUS(child_status);
                                }
                                // dup2(test, STDOUT_FILENO);
                        }
                        else
                        {
                                perror("fork");
                                exit(EXIT_FAILURE);
                        }
                }

                // print + completed 'command' [status]
                fprintf(stderr, "+ completed '%s' [%d]\n", commands.full_command, status);

                if (exit_status == 1)
                {
                        break;
                }
        }

        return EXIT_SUCCESS;
}
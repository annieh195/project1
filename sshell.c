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
#define PIPE_MAX 3
// #define TOKEN_MAX 32

struct command
{
        char *program;
        char *args[ARG_MAX];
        int count;
        char *full_command;
        char *output_file;
        int append;

        char *programs[PIPE_MAX][ARG_MAX];
};

int parse(struct command *commands, char *cmd_in)
{
        commands->full_command = strdup(cmd_in);
        if (commands->full_command == NULL)
        {
                return -1;
        }

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

        char *pipe_command = strtok(cmd_in, "|");
        int cmd_amt = 0;
        while (pipe_command != NULL && cmd_amt < PIPE_MAX)
        {
                char *command_args = strdup(pipe_command);
                if (command_args == NULL)
                {
                        return -1;
                }

                pipe_command = strtok(NULL, "|");

                char *token = strtok(command_args, " ");
                int cmd_arg_count = 0;

                //  printf("after copying and changing: %s\n", test);

                while (token != NULL)
                {
                        if (cmd_arg_count >= ARG_MAX)
                        {
                                fprintf(stderr, "Error: too many process arguments\n");
                                return -1;
                        }
                        commands->programs[cmd_amt][cmd_arg_count] = strdup(token);
                        // printf("checking test2 tokens: %s\n", test2);
                        if (cmd_arg_count == 0)
                        {
                                commands->program = commands->programs[cmd_amt][0];
                        }

                        // printf("checking array tokens: %s\n", commands->programs[cmd_amt][cmd_arg_count]);
                        // printf("dimensions: row %d column %d\n", cmd_amt, cmd_arg_count);
                        token = strtok(NULL, " ");
                        cmd_arg_count++;
                }
                commands->programs[cmd_amt][cmd_arg_count] = NULL;
                // printf("checking NULL in array: %s\n", commands->programs[cmd_amt][cmd_arg_count]);
                // printf("dimensions: row %d column %d\n", cmd_amt, cmd_arg_count);
                cmd_amt++;
        }
        commands->count = cmd_amt;

        // printf("cmd_in after |: %s\n", cmd_in);
        /*
        char *token = strtok(cmd_in, " ");
        while (token != NULL)
        {
                if (commands->count >= ARG_MAX)
                {
                        fprintf(stderr, "Error: too many process arguments\n");
                        return -1;
                }
                if (commands->count == 0)
                {
                        commands->program = token;
                }
                commands->args[commands->count++] = token;
                // printf("parse token: %s\n", token);
                token = strtok(NULL, " ");
        }
        commands->args[commands->count] = NULL;*/
        return 0;
}

int builtin_pwd()
{
        int status = 0;
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
                printf("%s\n", cwd);
        }
        else
        {
                status = 1;
        }
        return status;
}

int builtin_cd(struct command *commands)
{
        int status = 0;
        if (commands->programs[0][1] != NULL)
        {
                if (chdir(commands->programs[0][1]) != 0)
                {
                        fprintf(stderr, "Error: cannot cd into directory\n");
                        status = 1;
                }
                else
                {
                        status = 0;
                }
        }
        return status;
}

int builtin_sls()
{
        int status = 0;
        struct stat sb;
        DIR *dirp;
        struct dirent *dp;

        dirp = opendir(".");
        if (dirp == NULL)
        {
                fprintf(stderr, "Error: cannot open directory\n");
                status = 1;
                return status;
        }
        while ((dp = readdir(dirp)) != NULL)
        {
                // check and only print if it is directory using stat macro
                if (stat(dp->d_name, &sb) == 0 && S_ISREG(sb.st_mode))
                {
                        printf("%s (%ld bytes)\n", dp->d_name, sb.st_size);
                }
        }
        closedir(dirp);
        return status;
}

void print_complete(struct command *commands, int status)
{
        fprintf(stderr, "+ completed '%s' [%d]\n", commands->full_command, status);
}

int output_redirection(struct command *commands, int cmd_number)
{
        // redirect if output file is specified
        if (commands->output_file != NULL && cmd_number == commands->count - 1)
        {
                int fd;
                if (commands->append)
                {
                        fd = open(commands->output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
                }
                else
                {
                        fd = open(commands->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                }
                if (fd < 0)
                {
                        fprintf(stderr, "Error: cannot open output file\n");
                        return -1;
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
        }
        return 0;
}

void plug_piping(int *cmd_fds, int cmd_number, int commands_count)
{
        // prefer pipe picture in lecture
        /* if its not first command meaning they will be second and third,
        so plug them all to stdin (number 0, blue wire) */
        if (cmd_number != 0)
        {
                dup2(cmd_fds[(cmd_number - 1) * 2], STDIN_FILENO);
        }

        /* if its not last command meaning they will be first and second,
        so plug them all to stdout (number 1, pink wire) */
        if (cmd_number != commands_count - 1)
        {
                dup2(cmd_fds[cmd_number * 2 + 1], STDOUT_FILENO);
        }

        // close all fds
        for (int i = 0; i < 2 * commands_count; i++)
        {
                close(cmd_fds[i]);
        }
}

void pipe_function(struct command *commands)
{
        // check for pipe
        int cmd_fds[2 * PIPE_MAX];
        for (int i = 0; i < commands->count; i++)
        {
                pipe(cmd_fds + i * 2);
        }

        int statuses[PIPE_MAX];

        for (int cmd_number = 0; cmd_number < commands->count; cmd_number++)
        {
                pid_t pid = fork();
                if (pid == 0)
                {
                        output_redirection(commands, cmd_number);
                        plug_piping(cmd_fds, cmd_number, commands->count);
                        if (execvp(commands->programs[cmd_number][0], commands->programs[cmd_number]) < 0)
                        {
                                fprintf(stderr, "Error: command not found\n");
                                exit(EXIT_FAILURE);
                        }
                }
                else if (pid < 0)
                {
                        perror("fork");
                        exit(EXIT_FAILURE);
                }
        }
        /* Parent */
        // closes all pipe fds
        for (int i = 0; i < 2 * commands->count; i++)
        {
                close(cmd_fds[i]);
        }

        // wait for child
        // int child_status;
        for (int i = 0; i < commands->count; i++)
        {
                wait(&statuses[i]);
                // wait(&child_status);
                // if (WIFEXITED(child_status))
                // {
                //         int status = WEXITSTATUS(child_status);
                //         print_complete(commands, status);
                // }
        }
        fprintf(stderr, "+ completed '%s' [", commands->full_command);
        for (int i = 0; i < commands->count; i++)
        {
                if (i > 0)
                        fprintf(stderr, "][");
                fprintf(stderr, "%d", WEXITSTATUS(statuses[i]));
        }
        fprintf(stderr, "]\n");
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
                if (parse(&commands, cmd) < 0)
                {
                        continue; // outloop if error
                }
                int status = 0;
                // int exit_status = 0;

                /* Builtin command */
                // exit command
                if (!strcmp(commands.program, "exit"))
                {
                        fprintf(stderr, "Bye...\n");
                        print_complete(&commands, status);
                        break;
                }

                // pwd command
                else if (!strcmp(commands.program, "pwd"))
                {
                        status = builtin_pwd();
                        print_complete(&commands, status);
                }

                // cd command
                else if (!strcmp(commands.program, "cd"))
                {
                        status = builtin_cd(&commands);
                        print_complete(&commands, status);
                }

                // sls command
                // Cite: Lecture 03.syscalls.pdf page 32-33
                else if (!strcmp(commands.program, "sls"))
                {
                        status = builtin_sls();
                        print_complete(&commands, status);
                }

                else
                {
                        pipe_function(&commands);
                }

                // print + completed 'command' [status]
                // fprintf(stderr, "+ completed '%s' [%d]\n", commands.full_command, WEXITSTATUS(status));

                // if (exit_status == 1)
                // {
                //         break;
                // }
        }

        return EXIT_SUCCESS;
}
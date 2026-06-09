#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_ARGS   64
#define MAX_INPUT 1024

static void parse(char *line, char **args, char **infile, char **outfile) {
    *infile  = NULL;
    *outfile = NULL;

    char *token = strtok(line, " \t\n");
    int i = 0;
    while (token != NULL && i < MAX_ARGS - 1) {
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " \t\n");
            if (token) *infile = token;
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " \t\n");
            if (token) *outfile = token;
        } else {
            args[i++] = token;
        }
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
}

int main(void) {
    char line[MAX_INPUT];
    char *args[MAX_ARGS];
    char *infile, *outfile;

    printf("MiniShell — type 'exit' to quit\n");

    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(line, MAX_INPUT, stdin) == NULL)
            break;

        if (line[0] == '\n')
            continue;

        line[strcspn(line, "\n")] = 0;

        parse(line, args, &infile, &outfile);

        if (args[0] == NULL)
            continue;

        if (strcmp(args[0], "exit") == 0)
            break;

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            continue;
        }

        if (pid == 0) {
            int fd;

            if (infile) {
                fd = open(infile, O_RDONLY);
                if (fd < 0) { perror(infile); exit(1); }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }

            if (outfile) {
                fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) { perror(outfile); exit(1); }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            execvp(args[0], args);
            perror("execvp");
            exit(1);
        }

        int status;
        waitpid(pid, &status, 0);
    }

    return 0;
}

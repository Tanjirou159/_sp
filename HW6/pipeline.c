#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 64

static void split_cmd(char *cmd_str, char **args) {
    int i = 0;
    char *token = strtok(cmd_str, " \t\n");
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s '<cmd1>' '<cmd2>'\n", argv[0]);
        fprintf(stderr, "  pipe stdout of cmd1 into stdin of cmd2\n");
        return 1;
    }

    char cmd1_str[512], cmd2_str[512];
    strncpy(cmd1_str, argv[1], sizeof(cmd1_str) - 1);
    strncpy(cmd2_str, argv[2], sizeof(cmd2_str) - 1);

    char *args1[MAX_ARGS], *args2[MAX_ARGS];
    split_cmd(cmd1_str, args1);
    split_cmd(cmd2_str, args2);

    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("pipe");
        return 1;
    }

    pid_t pid1 = fork();
    if (pid1 == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        execvp(args1[0], args1);
        perror("execvp cmd1");
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);

        execvp(args2[0], args2);
        perror("execvp cmd2");
        exit(1);
    }

    close(pipefd[0]);
    close(pipefd[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    printf("Pipeline completed.\n");
    return 0;
}

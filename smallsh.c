#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <termios.h>

#define MAX_CMD_LENGTH 2048
#define MAX_ARG_LENGTH 512
#define PATH_MAX 256

bool backgroundAllowed = true;

struct command {
    char *argv[MAX_ARG_LENGTH];
    int argc;
    bool is_background;
    char input_file[PATH_MAX];
    char output_file[PATH_MAX];
};

/* Signal handlers should use async-signal-safe functions */
void handleSIGINT(int signo){
    (void) signo;
    const char *message = "\nterminated by signal 2\n";
    write(STDOUT_FILENO, message, strlen(message));
}

void handleSIGTSTP(int signo) {
    (void) signo;
    const char *message;
    if (backgroundAllowed) {
        message = "\nEntering foreground-only mode (& is now ignored)\n";
        backgroundAllowed = false;
    } else {
        message = "\nExiting foreground-only mode\n";
        backgroundAllowed = true;
    }
    write(STDOUT_FILENO, message, strlen(message));
}

void getInput(struct command *cmd);
void execCmd(struct command *cmd, int *exitStatus);
void printExitStatus(int *exitStatus);
void cleanupCmd(struct command *cmd);

/* ---------------------- Main function ---------------------------- */
/*	main() gets the input, checks for blanks or comments, processes
 *  CD, EXIT, and STATUS, and then executes the command if the input
 *  didn't call for any of the other functions.
 * ----------------------------------------------------------------- */

int main(void) {
    struct command cmd;
    int exitStatus = 0;
    bool exitFlag = false;

    struct sigaction SIGINT_action = {0};
    /* Fill out the SIGINT_action struct and Register handle_SIG_IGN as the signal handler */
    SIGINT_action.sa_handler = handleSIGINT;
    /* Block all catchable signals while handle_SIG_IGN is running */
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_action, NULL);

    struct sigaction SIGTSTP_action = {0};
    SIGTSTP_action.sa_handler = handleSIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = 0;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    do {
        printExitStatus(&exitStatus);
        getInput(&cmd);

        /* COMMENT or BLANK */
        if (cmd.argc == 0 || strcmp(cmd.argv[0], "#") == 0) {
            cleanupCmd(&cmd);
            continue;
        }

        /* EXIT */
        if (strcmp(cmd.argv[0], "exit") == 0) {
            exitFlag = true;
        }
        /* CD */
        else if (strcmp(cmd.argv[0], "cd") == 0) {
            // Change to HOME if no argument
            if (cmd.argc == 1) {
                chdir(getenv("HOME"));
            } else {
                if (chdir(cmd.argv[1]) == -1) {
                    perror("cd");
                    exitStatus = 1;
                }
            }
        }
        /* STATUS */
        else if (strcmp(cmd.argv[0], "status") == 0) {
            if (WIFEXITED(exitStatus)) { /* If exited normally */
                int lastForgroundStatus = WEXITSTATUS(exitStatus);
                printf("exit value %d\n", lastForgroundStatus);
            } else { /* If terminated by signal */
                int lastForgroundStatus = WTERMSIG(exitStatus);
                printf("terminated by signal %d\n", lastForgroundStatus);
            }
        }
        /* EXEC COMMAND */
        else {
            execCmd(&cmd, &exitStatus);
        }

        cleanupCmd(&cmd);
    } while (!exitFlag);

    return EXIT_SUCCESS;
}

void getInput(struct command *cmd) {
    char input[MAX_CMD_LENGTH];
    char *token;

    /* Initialize command struct with default values (0 or NULL) */
    memset(cmd, 0, sizeof(struct command));

    printf(": ");
    fflush(stdout);

    if (fgets(input, sizeof(input), stdin) == NULL) { /* Handle EOF */
        cmd->argv[0] = strdup("");
        return;
    }
    input[strcspn(input, "\n")] = '\0'; /* Remove newline character */

    token = strtok(input, " ");
    while (token != NULL && cmd->argc < MAX_ARG_LENGTH - 1) {
        if (strcmp(token, "&") == 0) { /* Background process */
            cmd->is_background = true;
        }
        else if (strcmp(token, "<") == 0) { /* Input redirection */
            token = strtok(NULL, " ");
            if (token) {
                strncpy(cmd->input_file, token, PATH_MAX - 1);
            }
        }
        else if (strcmp(token, ">") == 0) { /* Output redirection */
            token = strtok(NULL, " ");
            if (token) {
                strncpy(cmd->output_file, token, PATH_MAX - 1);
            }
        }
        else {
            if (strstr(token, "$$")) { /* $$ expansion*/
                char expanded[MAX_CMD_LENGTH];
                char *pos = strstr(token, "$$");
                *pos = '\0';
                snprintf(expanded, MAX_CMD_LENGTH, "%s%d%s",
                        token, getpid(), pos + 2);
                cmd->argv[cmd->argc] = strdup(expanded);
            } else {
                cmd->argv[cmd->argc] = strdup(token);
            }
            cmd->argc++;
        }
        token = strtok(NULL, " "); /* Get next token */
    }
    cmd->argv[cmd->argc] = NULL;  /* execvp requires NULL terminated array */
}

void execCmd(struct command *cmd, int *exitStatus) {
    pid_t childPid = fork();
    struct sigaction sa = {0};

    switch (childPid) {
        case -1:
            perror("fork() failed");
            exit(1);
            break;

        case 0:
            sa.sa_handler = SIG_DFL; /* Reset SIGINT to default behavior */
            sigaction(SIGINT, &sa, NULL);
            sa.sa_handler = SIG_IGN; /* Ignore SIGTSTP in child process */
            sigaction(SIGTSTP, &sa, NULL);

            if (cmd->is_background && backgroundAllowed) { /* Handle background process I/O */
                if (strlen(cmd->input_file) == 0) { /* Input redirection for background process */
                    int devNull = open("/dev/null", O_RDONLY);
                    if (devNull == -1) {
                        perror("cannot open /dev/null for input");
                        exit(1);
                    }
                    if (dup2(devNull, STDIN_FILENO) == -1) {
                        perror("input redirection to /dev/null failed");
                        exit(2);
                    }
                    fcntl(devNull, F_SETFD, FD_CLOEXEC); /* Close on successful exec */
                }
                if (strlen(cmd->output_file) == 0) { /* Output redirection for background process */
                    int devNull = open("/dev/null", O_WRONLY);
                    if (devNull == -1) {
                        perror("cannot open /dev/null for output");
                        exit(1);
                    }
                    if (dup2(devNull, STDOUT_FILENO) == -1) {
                        perror("output redirection to /dev/null failed");
                        exit(2);
                    }
                    fcntl(devNull, F_SETFD, FD_CLOEXEC);
                }
            }
            /* Handle explicit input redirection */
            if (strlen(cmd->input_file) > 0) {
                int inFile = open(cmd->input_file, O_RDONLY);
                if (inFile == -1) {
                    perror("cannot open input file");
                    exit(1);
                }
                /* Duplicate fd to perform input redirection (inFile fd to 0 so that it refers to stdin) */
                if (dup2(inFile, STDIN_FILENO) == -1) {
                    perror("input redirection failed");
                    exit(2);
                }
                fcntl(inFile, F_SETFD, FD_CLOEXEC);
            }
            /* Handle explicit output redirection */
            if (strlen(cmd->output_file) > 0) {
                int outFile = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (outFile == -1) {
                    perror("cannot open output file");
                    exit(1);
                }
                /* Duplicate fd to perform output redirection (outFile fd to 1 so that it refers to stdout) */
                if (dup2(outFile, STDOUT_FILENO) == -1) {
                    perror("output redirection failed");
                    exit(2);
                }
                fcntl(outFile, F_SETFD, FD_CLOEXEC);
            }

            /* execvp's first argument "filename" is sought in the list of directories specified in the PATH environment variable.
            *  This is the kind of searching that the shell performs when given a command name. 
            *  The new program inherits its environment from the calling process. 
            */
            if (execvp(cmd->argv[0], cmd->argv) == -1) {
                printf("%s: no such file or directory\n", cmd->argv[0]);
                fflush(stdout);
                exit(1);
            }
            break;

        default: /* Parent process */
            if (backgroundAllowed && cmd->is_background) { /* Handle background process */
                printf("background pid is %d\n", childPid);
                fflush(stdout);
                waitpid(childPid, exitStatus, WNOHANG); /* Non-blocking wait for child process */
            }
            else { /* Handle foreground process */
                waitpid(childPid, exitStatus, 0); /* Wait for child process to finish */
            }
            break;
    }
}

void printExitStatus(int *exitStatus) {
    pid_t childPid;
    /* Check for any background processes (-1 to wait for any child process) */
    while ((childPid = waitpid(-1, exitStatus, WNOHANG)) > 0) {
        printf("background pid %d is done: ", childPid);
        if (WIFEXITED(*exitStatus)) { /* If exited normally */
            printf("exit value %d\n", WEXITSTATUS(*exitStatus));
        } else { /* If terminated by signal */
            printf("terminated by signal %d\n", WTERMSIG(*exitStatus));
        }
        fflush(stdout);
    }
}

void cleanupCmd(struct command *cmd) {
    /* Free memory allocated for each argument */
    for (int i = 0; i < cmd->argc; i++) {
        if (cmd->argv[i]) {
            free(cmd->argv[i]);
            cmd->argv[i] = NULL;
        }
    }
    /* Reset command struct */
    cmd->argc = 0;
    cmd->is_background = false;
    cmd->input_file[0] = '\0';
    cmd->output_file[0] = '\0';
}

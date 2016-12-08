#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include "parser.h"
#include <assert.h>
#include <signal.h>

/*
 * 
 *  Authors: Sergio Rivas Delgado and Sergio Paniego Blanco
 *  Subject: Sistemas Operativos
 *  Issue: Minishell
 * 
 */

int main(void) {
    char buf[1024];
    tline * line;
    int i;
    int file;
    int status;



    printf("%s@minishell:~$ ", getenv("USER"));
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    int counter = 0;
    int * backcommands[40];
    for (i = 0; i < 40; i++) {
        backcommands[i] = (int*) malloc(sizeof (int));
        *backcommands[i] = 0;
    }
    int * backcommandsended[40];
    for (i = 0; i < 40; i++) {
        backcommandsended[i] = (int*) malloc(sizeof (int));
        *backcommandsended[i] = 0;
    }
    char ** commandsname = (char**) malloc(40 * sizeof (char*));
    for (i = 0; i < 40; i++) {
        commandsname[i] = (char*) malloc(1024 * sizeof (char*));
    }
    char ** commandsnameended = (char**) malloc(40 * sizeof (char*));
    for (i = 0; i < 40; i++) {
        commandsnameended[i] = (char*) malloc(1024 * sizeof (char*));
    }

    while (fgets(buf, 1024, stdin)) {
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);

        line = tokenize(buf);
        if (line == NULL) {
            continue;
        }
        if (line->redirect_input != NULL) {
            printf("Input redirection : %s\n", line->redirect_input);

            // File open 
            int file = open(line->redirect_input, O_RDONLY | O_CREAT);
            if (file < 0) return 1;

            // Standard input redirected to file
            dup2(0, 7);
            dup2(file, 0);
        }
        if (line->redirect_output != NULL) {
            printf("Output redirection : %s\n", line->redirect_output);

            // File open 
            file = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC);
            if (file < 0) return 1;

            // Standard output redirected to file
            dup2(1, 8);
            dup2(file, 1);
        }
        if (line->redirect_error != NULL) {
            printf("Error redirection : %s\n", line->redirect_error);

            // File open 
            file = open(line->redirect_error, O_WRONLY | O_CREAT | O_TRUNC);
            if (file < 0) return 1;

            // Standard error output redirected to file
            dup2(1, 8);
            dup2(file, 1);

            dup2(2, 9);
            dup2(file, 2);
        }

        int ** p = (int**) malloc(line->ncommands * sizeof (int*));
        for (i = 0; i < line->ncommands; i++) {
            p[i] = (int*) malloc(2 * sizeof (int));
        }

        int pid[line->ncommands];
        int i;

        // Pipes creation
        for (i = 0; i < line->ncommands; i++) {
            pipe(p[i]);
        }


        // Father creates children and connects pipes
        for (i = 0; i < line->ncommands; i++) {

            char st2[] = "jobs";
            char st3[] = "fg";
            char st4[] = "exit";
            if ((line->ncommands == 1)&&(strcmp(line->commands[0].argv[0], st2) == 0)) {

                counter = 0;
                while (*backcommands[counter] != 0) {
                    counter++;
                }
                counter--;

                for (i = 0; i <= counter; i++) {
                    pid_t finish = waitpid(*backcommands[i], &status, WNOHANG);

                    if (finish == *backcommands[i]) {
                        if (WIFEXITED(status) != 0) {
                            if (WEXITSTATUS(status) != 0) {
                                printf("The command didn't run properly\n");
                            }
                            int a = 0;
                            while (*backcommandsended[a] != 0) {
                                a++;
                            }
                            *backcommandsended[a] = *backcommands[i];
                            strcpy(commandsnameended[a], commandsname[i]);
                            strcpy(commandsname[i], "");
                            *backcommands[i] = 0;
                            for (a = i; a < 39; a++) {
                                *backcommands[a] = *backcommands[a + 1];
                                strcpy(commandsname[a], commandsname[a + 1]);
                            }
                            *backcommands[39] = 0;
                            strcpy(commandsname[39], "");
                            counter--;
                        }
                    }
                }
                int x = 0;
                while (*backcommands[x] != 0) {
                    printf("[%i] Process: %i EXECUTING\t %s \n", x, *backcommands[x], commandsname[x]);
                    x++;
                }
                int a = 0;
                i = 0;
                while (*backcommandsended[a] != 0) {
                    printf("[%i] Proceso: %i DONE\t %s\n", i, *backcommandsended[a], commandsnameended[a]);
                    *backcommandsended[a] = 0;
                    i++;
                    a++;
                }
            } else if ((strcmp(line->commands[0].argv[0], st3) == 0)) {
                int number;
                if (line->commands[0].argv[1] != NULL) {
                    number = atoi(line->commands[0].argv[1]);
                } else {
                    number = 0;
                }

                if (*backcommands[number] != 0) {
                    printf("[%i] Process: %i EXECUTING in foreground\t %s \n", number, *backcommands[number], commandsname[number]);
                    pid_t finish = waitpid(*backcommands[number], &status, 0);
                    if (finish == *backcommands[number]) {
                        if (WIFEXITED(status) != 0) {
                            if (WEXITSTATUS(status) != 0) {
                                printf("The command didn't run properly\n");
                            }
                            printf("The command ran properly\n");
                            int a = 0;
                            while (*backcommandsended[a] != 0) {
                                a++;
                            }
                            *backcommands[number] = 0;
                            *backcommandsended[a] = *backcommands[i];
                            strcpy(commandsnameended[a], commandsname[i]);
                            strcpy(commandsname[i], "");
                            *backcommands[i] = 0;
                            for (a = i; a < 39; a++) {
                                *backcommands[a] = *backcommands[a + 1];
                                strcpy(commandsname[a], commandsname[a + 1]);
                            }

                            *backcommands[39] = 0;
                            strcpy(commandsname[39], "");

                        }
                    }
                }

            } else if ((strcmp(line->commands[0].argv[0], st4) == 0)) {
                kill(0, SIGTERM);
            } else {
                pid[i] = fork();
                if (pid[i] < 0) {
                    fprintf(stderr, "fork() failed.\n%s\n", strerror(errno));
                    exit(1);
                } else if (pid[i] == 0) {
                    // First or only child 
                    signal(SIGINT, SIG_DFL);
                    signal(SIGQUIT, SIG_DFL);
                    if (i == 0) {
                        // Pipe created if there is more than one child
                        if (line->ncommands > 1) {
                            close(p[i][0]);
                            dup2(p[i][1], 1);
                        }
                        // Cd command implementation
                        char st[] = "cd";
                        if ((line->ncommands == 1)&&(strcmp(line->commands[0].argv[0], st) == 0)) {
                            char *dir;
                            char buffer[512];

                            if (line->commands[0].argc > 2) {
                                fprintf(stderr, "Directory: %s used\n", line->commands[0].argv[1]);
                            }

                            if (line->commands[0].argc == 1) {
                                dir = getenv("HOME");
                                if (dir == NULL) {
                                    fprintf(stderr, "$HOME variable doesn't exit\n");
                                }
                            } else {
                                char aux[] = "$HOME";
                                if (strcmp(line->commands[0].argv[1], aux) == 0) {
                                    dir = getenv("HOME");
                                } else {
                                    dir = line->commands[0].argv[1];
                                }

                            }

                            // Check if it is a directory
                            if (chdir(dir) != 0) {
                                fprintf(stderr, "Changing directory fail: %s\n", strerror(errno));
                            }
                            printf("Current directory: %s\n", getcwd(buffer, -1));

                        } else {
                            execvp(line->commands[i].argv[0], line->commands[i].argv);
                            // If it reachs here, execvp failed
                            printf("Fail when command executed: %s\n", strerror(errno));
                            exit(1);
                        }
                    } else if (i == (line->ncommands - 1)) {//Ultimo hijo
                        close(p[i - 1][1]);
                        dup2(p[i - 1][0], 0);
                        execvp(line->commands[i].argv[0], line->commands[i].argv);
                        // If it reachs here, execvp failed
                        printf("Fail when command executed: %s\n", strerror(errno));
                        exit(1);
                    } else {// Intermediate children
                        close(p[i - 1][1]);
                        close(p[i][0]);
                        dup2(p[i - 1][0], 0);
                        dup2(p[i][1], 1);
                        execvp(line->commands[i].argv[0], line->commands[i].argv);
                        // If it reachs here, execvp failed
                        printf("Fail when command executed: %s\n", strerror(errno));
                        exit(1);
                    }
                } else {// Father output pipe close

                    close(p[i][1]);
                }
            }

        }
        if (line->background) {
            if (line->commands->filename != NULL) {
                printf("Command executing in background\n");
                signal(SIGINT, SIG_IGN);
                signal(SIGQUIT, SIG_IGN);
                for (i = 0; i < line->ncommands; i++) {
                    if (i >= line->ncommands - 1) {
                        counter = 0;
                        while (*backcommands[counter] != 0) {
                            counter++;
                        }
                        int k;
                        if (strlen(commandsname[counter]) > 0) {
                            strcpy(commandsname[counter], "");
                        }
                        int x;
                        for (k = 0; k < line->ncommands; k++) {
                            for (x = 0; x < line->commands[k].argc; x++) {

                                strcat(commandsname[counter], line->commands[k].argv[x]);
                                strcat(commandsname[counter], " ");
                            }
                            if (k < line->ncommands - 1) {
                                strcat(commandsname[counter], " | ");
                            } else {
                                strcat(commandsname[counter], " ");
                            }
                        }
                        strcat(commandsname[counter], "&");
                        *backcommands[counter] = pid[0];
                    }

                }
                int y = 0;
                while (*backcommands[y] != 0) {
                    printf("Process [%i]: %i in background execution.\n", y, *backcommands[y]);
                    y++;
                }
            }
        } else {
            // Father does waitpid for each children
            for (i = 0; i < line->ncommands; i++) {
                waitpid(pid[i], &status, 0);
                if (WIFEXITED(status) != 0)
                    if (WEXITSTATUS(status) != 0)
                        printf("The command didn't run properly\n");
            }
        }

        // Free used memory 
        for (i = 0; i < line->ncommands; i++) {
            free(p[i]);
        }
        free(p);

        // Back to initial state in case of redirections
        if (line->redirect_input != NULL) {
            close(file);
            dup2(7, 0);
        }
        if (line->redirect_output != NULL) {
            close(file);
            dup2(8, 1);
        }
        if (line->redirect_error != NULL) {
            close(file);
            dup2(8, 1);
            dup2(9, 2);
        }
        printf("%s@minishell:~$ ", getenv("USER"));
    }
    // Free used memory 
    for (i = 0; i < 40; i++) {
        free(backcommands[i]);
    }
    for (i = 0; i < 40; i++) {
        free(backcommandsended[i]);
    }
    for (i = 0; i < 40; i++) {
        free(commandsname[i]);
    }
    free(commandsname);
    for (i = 0; i < 40; i++) {
        free(commandsnameended[i]);
    }
    free(commandsnameended);
    return 0;
}

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

int main(void) {
    char buf[1024];
    char buf2[1024];
    int bufsize = 1024;
    tline * line;
    int i, j;
    int file;
    int status;

    getlogin_r(buf2, bufsize);
    printf("%s@minishell==> ", buf2);
    //signal(SIGINT, SIG_IGN);
    //signal(SIGQUIT, SIG_IGN);
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
        //signal(SIGINT, SIG_IGN);
        //signal(SIGQUIT, SIG_IGN);

        line = tokenize(buf);
        if (line == NULL) {
            continue;
        }
        if (line->redirect_input != NULL) {
            printf("redirección de entrada: %s\n", line->redirect_input);

            //Abrimos el fichero
            int file = open(line->redirect_input, O_RDONLY | O_CREAT);
            if (file < 0) return 1;

            //Redirigimos la entrada estandar al fichero
            dup2(0, 7);
            dup2(file, 0);
        }
        if (line->redirect_output != NULL) {
            printf("redirección de salida: %s\n", line->redirect_output);

            //Abrimos el fichero
            file = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC);
            if (file < 0) return 1;

            //Redirigimos la salida estandar al fichero
            dup2(1, 8);
            dup2(file, 1);
        }
        if (line->redirect_error != NULL) {
            printf("redirección de error: %s\n", line->redirect_error);

            //Abrimos el fichero
            file = open(line->redirect_error, O_WRONLY | O_CREAT | O_TRUNC);
            if (file < 0) return 1;

            //Redirigimos la salida de error al fichero
            dup2(1, 8);
            dup2(file, 1);

            dup2(2, 9);
            dup2(file, 2);
        }

        for (i = 0; i < line->ncommands; i++) {
            printf("orden %d (%s):\n", i, line->commands[i].filename);
            for (j = 0; j < line->commands[i].argc; j++) {
                printf("  argumento %d: %s\n", j, line->commands[i].argv[j]);
            }
        }

        int ** p = (int**) malloc(line->ncommands * sizeof (int*));
        for (i = 0; i < line->ncommands; i++) {
            p[i] = (int*) malloc(2 * sizeof (int));
        }

        int pid[line->ncommands];
        int i;

        //Creo los pipes
        for (i = 0; i < line->ncommands; i++) {
            pipe(p[i]);
        }


        //El padre crea los hijos y conecta los pipes
        for (i = 0; i < line->ncommands; i++) {
            //signal(SIGINT, SIG_DFL);
            //signal(SIGQUIT, SIG_DFL);
            char st2[] = "jobs";
            if ((line->ncommands == 1)&&(strcmp(line->commands[0].argv[0], st2) == 0)) {

                counter = 0;
                while (*backcommands[counter] != 0) {
                    counter++;
                }
                counter--;

                for (i = 0; i <= counter; i++) {
                    //printf("%i \n",*backcommands[i]);
                    pid_t finish = waitpid(*backcommands[i], &status, WNOHANG);

                    printf("%i  %i  %i %i\n", i, counter, *backcommands[i], finish);
                    if (finish == *backcommands[i]) {
                        if (WIFEXITED(status) != 0) {
                            printf("Proceso %i :ha terminado %i\n", *backcommands[i], finish);
                            if (WEXITSTATUS(status) != 0) {
                                printf("El comando no se ejecutó correctamente\n");
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
                    printf("[%i] Proceso: %i EXECUTING\t %s \n", x, *backcommands[x], commandsname[x]);
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
            } else {
                pid[i] = fork();
                if (pid[i] < 0) {
                    fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
                    exit(1);
                } else if (pid[i] == 0) {//Primer o unico hijo
                    if (i == 0) {
                        //Solo se crea el pipe si hay más de 1 hijo
                        if (line->ncommands > 1) {
                            close(p[i][0]);
                            dup2(p[i][1], 1);
                        }
                        //Implementacion del comando cd
                        char st[] = "cd";
                        if ((line->ncommands == 1)&&(strcmp(line->commands[0].argv[0], st) == 0)) {
                            char *dir;
                            char buffer[512];

                            if (line->commands[0].argc > 2) {
                                fprintf(stderr, "Uso: %s directorio\n", line->commands[0].argv[1]);
                            }

                            if (line->commands[0].argc == 1) {
                                dir = getenv("HOME");
                                if (dir == NULL) {
                                    fprintf(stderr, "No existe la variable $HOME\n");
                                }
                            } else {
                                char aux[] = "$HOME";
                                if (strcmp(line->commands[0].argv[1], aux) == 0) {
                                    dir = getenv("HOME");
                                } else {
                                    dir = line->commands[0].argv[1];
                                }

                            }

                            // Comprobar si es un directorio
                            if (chdir(dir) != 0) {
                                fprintf(stderr, "Error al cambiar de directorio: %s\n", strerror(errno));
                            }
                            printf("El directorio actual es: %s\n", getcwd(buffer, -1));
                            //} else if ((line->ncommands == 1)&&(strcmp(line->commands[0].argv[0], st2) == 0)) {


                        } else {
                            execvp(line->commands[i].argv[0], line->commands[i].argv);
                            //Si llega aquí es que se ha producido un error en el execvp
                            printf("Error al ejecutar el comando: %s\n", strerror(errno));
                            exit(1);
                        }
                    } else if (i == (line->ncommands - 1)) {//Ultimo hijo
                        close(p[i - 1][1]);
                        dup2(p[i - 1][0], 0);
                        execvp(line->commands[i].argv[0], line->commands[i].argv);
                        //Si llega aquí es que se ha producido un error en el execvp
                        printf("Error al ejecutar el comando: %s\n", strerror(errno));
                        exit(1);
                    } else {//Hijos intermedios 
                        close(p[i - 1][1]);
                        close(p[i][0]);
                        dup2(p[i - 1][0], 0);
                        dup2(p[i][1], 1);
                        execvp(line->commands[i].argv[0], line->commands[i].argv);
                        //Si llega aquí es que se ha producido un error en el execvp
                        printf("Error al ejecutar el comando: %s\n", strerror(errno));
                        exit(1);
                    }
                } else {//Se cierra el pipe de salida en el padre

                    close(p[i][1]);
                }
            }

        }
        if (line->background) {
            printf("comando a ejecutarse en background\n");
            //signal(SIGINT, SIG_IGN);
            //signal(SIGQUIT, SIG_IGN);
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
                        for (x = 0; x < line->commands[k].argc; x++){
                            
                            strcat(commandsname[counter], line->commands[k].argv[x]);
                            strcat(commandsname[counter], " ");
                        }
                        if(k < line->ncommands-1){
                            strcat(commandsname[counter], " | ");
                        }else{
                            strcat(commandsname[counter], " ");
                        }
                    }
                    strcat(commandsname[counter], "&");
                    *backcommands[counter] = pid[0];
                }

            }
            int y = 0;
            while (*backcommands[y] != 0) {
                printf("Proceso [%i]: %i en ejecución en background.\n", y, *backcommands[y]);
                y++;
            }
        } else {
            //El padre hace el waitpid por cada uno de los hijos
            for (i = 0; i < line->ncommands; i++) {
                waitpid(pid[i], &status, 0);
                if (WIFEXITED(status) != 0)
                    if (WEXITSTATUS(status) != 0)
                        printf("El comando no se ejecutó correctamente\n");
            }
        }

        //Liberamos la memoria utilizada
        for (i = 0; i < line->ncommands; i++) {
            free(p[i]);
        }
        free(p);

        //Volvemos al estado inicial en el caso de haberse producido redirecciones
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
        getlogin_r(buf2, bufsize);
        printf("%s@minishell==> ", buf2);
    }
    return 0;
}

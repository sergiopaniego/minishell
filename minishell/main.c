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
    tline * line;
    int i, j;
    int file;
    int status;

    printf("minishell ==> ");
    //signal(SIGINT, SIG_IGN);
    //signal(SIGQUIT, SIG_IGN);
    int counter = 0;
    int * backcommands[10];
    for (i = 0; i < 10; i++) {
        backcommands[i] = (int*) malloc(sizeof (int));
    }
    int * backcommandsended[10];
    for (i = 0; i < 10; i++) {
        backcommandsended[i] = (int*) malloc(sizeof (int));
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
                    char st2[] = "jobs";
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
                    } else if ((line->ncommands == 1)&&(strcmp(line->commands[0].argv[0], st2) == 0)) {
                        i = 0;
                        while (*backcommands[i] != 0) {
                            printf("[%i] Proceso: %i EXECUTING\n", i, *backcommands[i]);
                            i++;
                        }
                        int a = 0;
                        while (*backcommandsended[a] != 0) {
                            printf("[%i] Proceso: %i DONE\n", i, *backcommandsended[a]);
                            *backcommandsended[a] = 0;
                            i++;
                            a++;
                        }
                        a = 0;
                        counter = 0;
                        while (*backcommands[counter] != 0) {
                            counter++;
                        }
                        for (i = 0; i < counter; i++) { 
                            waitpid(*backcommands[i], &status, WNOHANG);
                            if (WIFEXITED(status) != 0) {
                                if (WEXITSTATUS(status) != 0) {
                                    printf("El comando no se ejecutó correctamente\n");
                                }
                                printf("Proceso %i :ha terminado\n", *backcommands[i]);
                               
                                //int a = 0;
                                while (*backcommandsended[a] != 0) {
                                    a++;
                                }
                                *backcommandsended[a] = *backcommands[i];
                                *backcommands[i] = 0;
                                for (a = i; a < 9; a++) {
                                    *backcommands[a] = *backcommands[a + 1];
                                }
                                *backcommands[9] = 0;
                            }
                        }
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
        if (line->background) {
            printf("comando a ejecutarse en background\n");
            //signal(SIGINT, SIG_IGN);
            //signal(SIGQUIT, SIG_IGN);
            /*for (i = 0; i < line->ncommands; i++) {
                if (i >= line->ncommands - 1) {*/
                    *backcommands[counter] = pid[0];
                /*}
            }*/

            //Saca la lista de comandos en ejecución y añade el nuevo a la lista
            i = 0;
            while (*backcommands[i] != 0) {
                printf("Proceso [%i]: %i en ejecución en background.\n", i, *backcommands[i]);
                i++;
            }

            /*for (i = 0; i < line->ncommands; i++) {
                waitpid(pid[i], &status, WNOHANG);
                if (WIFEXITED(status) != 0){
                    printf("Entra1\n");
                    if (WEXITSTATUS(status) != 0){
                        printf("El comando no se ejecutó correctamente\n");
                    if(i>=line->ncommands-1){
                        printf("Entra2\n"); 
                        for(i=counter; i<9; i++){
             *backcommands[i]=*backcommands[i+1];                           
                        }
             *backcommands[9]=0;
                    }
                                 
                    
                }
            }*/
          
            for (i = 0; i < counter; i++) {
                waitpid(*backcommands[i], &status, WNOHANG);
                if (WIFEXITED(status) != 0) {
                    printf("Proceso %i :ha terminado \n", *backcommands[i]);
                    if (WEXITSTATUS(status) != 0) {
                        printf("El comando no se ejecutó correctamente\n");
                    }
                    int a = 0;
                    while (*backcommandsended[a] != 0) {
                        a++;
                    }
                    *backcommandsended[a] = *backcommands[i];
                    *backcommands[i] = 0;
                    for (a = i; a < 9; a++) {
                        *backcommands[a] = *backcommands[a + 1];
                    }
                    *backcommands[9] = 0;


                }
            }
            counter = 0;
            while (*backcommands[counter] != 0) {
                counter++;
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

        printf("minishell ==> ");
    }
    return 0;
}

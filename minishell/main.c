#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "parser.h"

int main(void) {
    char buf[1024];
    tline * line;
    int i, j;
    printf("minishell ==> ");
    while (fgets(buf, 1024, stdin)) {

        line = tokenize(buf);
        //pid_t pid;
        int status;
        if (line == NULL) {
            continue;
        }
        if (line->redirect_input != NULL) {
            char* newargv;
            newargv = (char *) malloc(1024 * sizeof (char));
            FILE *fp;
            fp = fopen(line->redirect_input, "r");
            printf("redirección de entrada: %s\n", line->redirect_input);

            while (fgets(buf, 1024, fp) != NULL) {
                strcat(newargv, buf);
            }

            line->commands[0].argc++;
            line->commands[0].argv[1] = newargv;
        }
        if (line->redirect_output != NULL) {
            printf("redirección de salida: %s\n", line->redirect_output);
        }
        if (line->redirect_error != NULL) {
            printf("redirección de error: %s\n", line->redirect_error);
        }
        if (line->background) {
            printf("comando a ejecutarse en background\n");
        }
        for (i = 0; i < line->ncommands; i++) {
            printf("orden %d (%s):\n", i, line->commands[i].filename);
            for (j = 0; j < line->commands[i].argc; j++) {
                printf("  argumento %d: %s\n", j, line->commands[i].argv[j]);
            }
        }

        if (line->ncommands == 1) {
            int pid;
            char st[] = "cd";
            if (strcmp(line->commands[0].argv[0], st) == 0) {
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
                    char aux[]="$HOME";
                    if(strcmp(line->commands[0].argv[1],aux)==0){
                        dir=getenv("HOME");
                    }else{
                        dir = line->commands[0].argv[1];
                    }
                   
                }

                // Comprobar si es un directorio
                if (chdir(dir) != 0) {
                    fprintf(stderr, "Error al cambiar de directorio: %s\n", strerror(errno));
                }
                printf("El directorio actual es: %s\n", getcwd(buffer, -1));
            } else {
                pid = fork();
                if (pid < 0) { /* Error */
                    fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
                    exit(1);
                } else if (pid == 0) { /* Proceso Hijo */
                    execvp(line->commands[0].filename, line->commands[0].argv);

                    //Si llega aquí es que se ha producido un error en el execvp
                    printf("Error al ejecutar el comando: %s\n", strerror(errno));
                    exit(1);

                } else { /* Proceso Padre. 
                - WIFEXITED(estadoHijo) es 0 si el hijo ha terminado de una manera anormal (caida, matado con un kill, etc). 
                Distinto de 0 si ha terminado porque ha hecho una llamada a la función exit()
                - WEXITSTATUS(estadoHijo) devuelve el valor que ha pasado el hijo a la función exit(), siempre y cuando la 
                macro anterior indique que la salida ha sido por una llamada a exit(). */
                    wait(&status);
                    if (WIFEXITED(status) != 0)
                        if (WEXITSTATUS(status) != 0)
                            printf("El comando no se ejecutó correctamente\n");
                }
            }
        } else {

            //Creo el pipe
            int p[2];
            int pid1, pid2;

            pipe(p);

            pid1 = fork();

            if (pid1 < 0) { /* Error */
                fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
                exit(1);
            } else if (pid1 == 0) {//Hijo1
                close(p[0]);
                dup2(p[1], 1); //Entrada del pipe y salida estándar
                //Si hago close de p[1] no se tiene por que cerrar la salida 1
                execvp(line->commands[0].filename, line->commands[0].argv);
                //Si llega aquí es que se ha producido un error en el execvp
                printf("Error al ejecutar el comando: %s\n", strerror(errno));
                exit(1);

            } else { /* Proceso Padre. */
                pid2 = fork();
                if (pid2 == 0) {//Hijo2
                    close(p[1]);
                    dup2(p[0], 0);
                    execvp(line->commands[1].filename, line->commands[1].argv);
                } else {//Padre
                    close(p[0]);
                    close(p[1]);
                    /*- WIFEXITED(estadoHijo) es 0 si el hijo ha terminado de una manera anormal (caida, matado con un kill, etc). 
                    Distinto de 0 si ha terminado porque ha hecho una llamada a la función exit()
                    - WEXITSTATUS(estadoHijo) devuelve el valor que ha pasado el hijo a la función exit(), siempre y cuando la 
                    macro anterior indique que la salida ha sido por una llamada a exit(). */
                    wait(&status);
                    if (WIFEXITED(status) != 0)
                        if (WEXITSTATUS(status) != 0)
                            printf("El comando no se ejecutó correctamente\n");

                }

            }

        }
        printf("minishell ==> ");
    }
    return 0;
}

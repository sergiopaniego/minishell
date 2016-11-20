#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "parser.h"
//#include "libparser_64.a"



int main(void) {
	char buf[1024];
	tline * line;
	int i,j;
        
	printf("minishell ==> ");	
	while (fgets(buf, 1024, stdin)) {
		
		line = tokenize(buf);
		if (line==NULL) {
			continue;
		}
		if (line->redirect_input != NULL) {
			printf("redirección de entrada: %s\n", line->redirect_input);
                        
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
		for (i=0; i<line->ncommands; i++) {
			printf("orden %d (%s):\n", i, line->commands[i].filename);
			for (j=0; j<line->commands[i].argc; j++) {
				printf("  argumento %d: %s\n", j, line->commands[i].argv[j]);
			}
		}
		pid_t  pid;
		int status;			
		
		pid = fork();
		if (pid < 0) { /* Error */
			fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
			exit(1);
		}
		else if (pid == 0) { /* Proceso Hijo */
			execvp(line->commands[0].filename, line->commands[0].argv);
			//Si llega aquí es que se ha producido un error en el execvp
			printf("Error al ejecutar el comando: %s\n", strerror(errno));
			exit(1);
		
		}
		else { /* Proceso Padre. 
	    		- WIFEXITED(estadoHijo) es 0 si el hijo ha terminado de una manera anormal (caida, matado con un kill, etc). 
			Distinto de 0 si ha terminado porque ha hecho una llamada a la función exit()
	    		- WEXITSTATUS(estadoHijo) devuelve el valor que ha pasado el hijo a la función exit(), siempre y cuando la 
			macro anterior indique que la salida ha sido por una llamada a exit(). */
			wait (&status);
			if (WIFEXITED(status) != 0)
				if (WEXITSTATUS(status) != 0)
					printf("El comando no se ejecutó correctamente\n");
			exit(0);
		}
			printf("minishell ==> ");	
		}
	return 0;
}




#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "comun.h"
#include "comun.c"
#include "diccionario.h"
#include "cola.h"
#include <sys/socket.h>
#include <netinet/in.h>

#define TAM  65536 //2¹⁶


int main(int argc, char *argv[]) {
	int s, s_conec, leido;
	unsigned int tam_dir;
	struct sockaddr_in dir, dir_cliente;
	char buf[TAM];
	char *pchar;
	int opcion=1;

	if (argc!=2) {
		fprintf(stderr, "Uso: %s puerto\n", argv[0]);
		return 1;
	}
	if ((s=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("error creando socket");
		return 1;
	}
	/* Para reutilizar puerto inmediatamente */
        if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opcion, sizeof(opcion))<0){
                perror("error en setsockopt");
                return 1;
        }
	dir.sin_addr.s_addr=INADDR_ANY;
	dir.sin_port=htons(atoi(argv[1]));
	dir.sin_family=PF_INET;
	if (bind(s, (struct sockaddr *)&dir, sizeof(dir)) < 0) {
		perror("error en bind");
		close(s);
		return 1;
	}
	if (listen(s, 5) < 0) {
		perror("error en listen");
		close(s);
		return 1;
	}
	while (1) {

		tam_dir=sizeof(dir_cliente);
		if ((s_conec=accept(s, (struct sockaddr *)&dir_cliente, &tam_dir))<0){
			perror("error en accept");
			close(s);
			return 1;
		}
		
        if ( (leido=read(s_conec, buf, sizeof(pchar))) < 0){
			perror("error en read");
			close(s);
			close(s_conec);
			return 1;
        }//read de la operacion

		printf("Se ha seleccionado la operacion: %c \n", buf[0]);		

		switch (buf[0]){
		case 'C':
        	if ( (leido=read(s_conec, buf, TAM)) < 0){
				perror("error en read");
				close(s);
				close(s_conec);
				return 1;
        	}//read

			printf("He creado la cola %s \n", buf);			

			break;
		//FIN DEL CASE C
		default:
			break;
		}// switch



        
        /*
        if (write(1, buf, leido)<0) {
		    perror("error en write");
			close(s);
			close(s_conec);
			return 1;
        }//write
        */        
        
		close(s_conec);
	}
	close(s);
	return 0;
}
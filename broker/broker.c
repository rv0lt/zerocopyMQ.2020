#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "comun.h"
#include "comun.c"
#include "diccionario.h"
#include "cola.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>

#define TAM  65536 //2¹⁶

/*----------------------------------------------------*/
void return_to_client(int fd, char* aux){
	struct iovec iov[1];    
	char *pchar;
	pchar=aux;

    iov[0].iov_base=&pchar;
    iov[0].iov_len=sizeof(pchar);  
    
	writev(fd, iov,1);
}


/*---------------------------------------------------*/
void free_cola(char *c, void *v){
	free(c);
	struct cola *cl =v;
	cola_destroy(cl, NULL);
}

void print_dic(char *c, void *v){
	printf("Nombre de la cola: %s \n", c);
}

void see_dic (struct diccionario *d){
	printf("--------------\n");	
	printf("CONTENIDO DEL DICCIONARIO \n");
	dic_visit(d, print_dic);
	printf("--------------\n");
}
void print_cola(void *v){
	void * msg = v;
	printf("%s", msg);
}
void see_cola (struct cola *c){
	printf("_______________\n");
	printf("CONTENIDO DE LA COLA\n");
	cola_visit(c, print_cola);
	printf("_______________\n");

}

struct cola *get_cola (struct diccionario *d, char* name_cola){
	struct cola *aux;
	int error;
	aux= dic_get(d,name_cola,&error);
	if (error < 0){
		perror("Cola no existente");
		return NULL;
	}
	else
		return aux;
}
/*-----------------------------------------------------*/
int main(int argc, char *argv[]) {
	int s, s_conec, leido;
	unsigned int tam_dir;
	struct sockaddr_in dir, dir_cliente;
	char buf[TAM];
	int size_cola;
	int size_msg;
	char *pchar;
	struct cola *cola;
	int opcion=1;
	struct diccionario *dic;
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
		return_to_client(s_conec,'-1');
		close(s);
		return 1;
	}
	if (listen(s, 5) < 0) {
		perror("error en listen");
		return_to_client(s_conec,'-1');
		close(s);
		return 1;
	}
	dic = dic_create();
	if (dic==NULL){
		perror("Error al crear el diccionario");
		return_to_client(s_conec,'-1');
		close(s);
		return 1;
	}
	while (1) {

		tam_dir=sizeof(dir_cliente);
		if ((s_conec=accept(s, (struct sockaddr *)&dir_cliente, &tam_dir))<0){
			perror("error en accept");
			return_to_client(s_conec,'-1');
			close(s);
			return 1;
		}
		
        if ( (leido=read(s_conec, buf, sizeof(pchar))) < 0){
			perror("error en read");
			return_to_client(s_conec,'-1');
			close(s);
			close(s_conec);
			return 1;
        }//read de la operacion

		printf("Se ha seleccionado la operacion: %c \n", buf[0]);		

		switch (buf[0]){
		case 'C': //Crear una nueva cola
        	if ( (leido=read(s_conec, buf, TAM)) < 0){
				perror("error en read");
				return_to_client(s_conec,'-1');
				close(s);
				close(s_conec);
				return 1;
        	}//read
			//printf("He creado la cola %s \n", buf);			

			char * aux = malloc(leido);
			strcpy(aux,buf);
			cola = cola_create();
			if (cola == NULL){
				perror("Error al crear la estructura cola");
				return_to_client(s_conec,'-1');
				close(s);
				close(s_conec);
				return 1;
			}//if
			if (dic_put(dic, aux, cola) <0){
				perror("Nombre de cola ya existente");
				return_to_client(s_conec,'-1');
				continue;
			}//if
			see_dic(dic);
			break;
		//FIN DEL CASE C
		case 'D': //Destruir una cola (y sus mensajes)
        	if ( (leido=read(s_conec, buf, TAM)) < 0){
				perror("error en read");
				return_to_client(s_conec,'-1');
				close(s);
				close(s_conec);
				return 1;
        	}//read
			if (dic_remove_entry(dic, buf, free_cola) < 0){
				perror("No existe ese nombre de cola \n");
				return_to_client(s_conec, '-1');
				continue;
			}
			see_dic(dic);
			break;
		//FIN DEL CASE D
		case 'P': //Poner un nuevo mensaje a una cola determinada
			if ( (leido=read(s_conec,&size_cola , sizeof(int))) < 0){
				perror("error en read");
				return_to_client(s_conec,'-1');
				close(s);
				close(s_conec);
				return 1;
        	}//read
			//printf("---%d\n", size_cola);
			char * name_cola = malloc(size_cola);
			if ( (leido=read(s_conec, name_cola, size_cola)) < 0){
				perror("error en read");
				return_to_client(s_conec,'-1');
				close(s);
				close(s_conec);
				return 1;
        	}//read
			//printf("He seleccionado la cola %s \n", name_cola);			
			if ( (leido=read(s_conec,&size_msg , sizeof(int))) < 0){
				perror("error en read");
				return_to_client(s_conec,'-1');
				close(s);
				close(s_conec);
				return 1;
        	}//read
			//printf("____ %d\n", size_msg);
			void *msg = malloc(size_msg);
			if ( (leido=read(s_conec, msg, size_msg)) < 0){
				perror("error en read");
				return_to_client(s_conec,'-1');
				close(s);
				close(s_conec);
				return 1;
        	}//read			
			//printf("Contenido \n%s\n", msg);			
			
			cola= get_cola(dic,name_cola);
			if (cola == NULL){
				return_to_client(s_conec,'-1');
				free(name_cola);
				continue;				
			}
			cola_push_back(cola, msg);
			//printf("Contenido de la cola\n");
			see_cola(cola);
			free(name_cola);			
			break;
		//FIN DEL CASE P
		default:
			break;
		}// switch


       
		return_to_client(s_conec, '0');
		close(s_conec);
	}
	close(s);
	return 0;
}


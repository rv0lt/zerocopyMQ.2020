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
#include <stddef.h>

#define TAM  65536 //2¹⁶

int DEBUG = 0; 
int par = 0;
/*----------------------------------------------------*/
void return_to_client(int fd, char* aux){
	struct iovec iov[1];    
	char *pchar;
	pchar= aux;

    iov[0].iov_base= &pchar;
    iov[0].iov_len= sizeof(pchar);  
    
	writev(fd, iov,1);
}

void send_msg_to_client(int fd, void *msg, uint32_t tam,  bool vacio){
	struct iovec iov[2];
	if (vacio){
		int a =0;
		iov[0].iov_base= &a;
		iov[0].iov_len= sizeof(a);		
		writev(fd, iov,1);
	
	}
	if (msg == NULL){
		int a =-1;
		iov[0].iov_base= &a;
		iov[0].iov_len= sizeof(a);
		writev(fd, iov,1);

	}
	else{
		int a = tam;
		iov[0].iov_base=&a;
		iov[0].iov_len=sizeof(a);
		iov[1].iov_base=msg;
		iov[1].iov_len=a;
		writev(fd, iov,2);
	}
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
	if (!par){
		uint32_t tam = v;
		printf("/// Tamaño del msg %d ///\n", tam);
		par = 1;
	}else{
	void * msg = v;
	printf("%s\n", msg);
	par=0;
	}
}
void see_cola (struct cola *c){
	printf("_______________\n");
	printf("CONTENIDO DE LA COLA\n");
	par = 0;
	cola_visit(c, print_cola);
	printf("_______________\n");

}
/*----------------------------------------------------------------------*/
struct cola *get_cola (struct diccionario *d, char* name_cola){
	struct cola *aux;
	int error;
	aux= dic_get(d,name_cola,&error);
	if (error < 0){
		if (DEBUG)
			perror("Cola no existente");
		return NULL;
	}
	else
		return aux;
}
char *get_msg (struct cola *c){
	char * aux;
	int error;
	aux= cola_pop_front(c,&error);
	if (error < 0){
		if(DEBUG)
			perror("Cola vacía");
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
	uint32_t size_msg_;
	char *pchar;
	struct cola *cola;
	int opcion=1;
	struct diccionario *dic;
	void *msg;
	char *name_cola;

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

		if (DEBUG)
			printf("Se ha seleccionado la operacion: %c \n", buf[0]);		

		switch (buf[0]){
		case 'C': //Crear una nueva cola
			if ( (leido=read(s_conec,&size_cola , sizeof(int))) < 0){
				perror("error en read");
				return_to_client(s_conec,'-1');
				close(s);
				close(s_conec);
				return 1;
        	}//read  

			name_cola = malloc(size_cola);
			if ( (leido=read(s_conec, name_cola, size_cola)) < 0){
				perror("error en read");
				return_to_client(s_conec,'-1');
				close(s);
				close(s_conec);
				return 1;
        	}//read
			if (DEBUG)
				printf("He creado la cola %s de tamano %d \n", name_cola, size_cola-1);
			cola = cola_create();
			if (cola == NULL){
				perror("Error al crear la estructura cola");
				return_to_client(s_conec,'-1');
				close(s);
				close(s_conec);
				return 1;
			}//if
			if (dic_put(dic, name_cola, cola) <0){
				if (DEBUG)
					perror("Nombre de cola ya existente \n");
				return_to_client(s_conec,'-1');
				continue;
			}//if
			if (DEBUG)
				see_dic(dic);
			break;
		//FIN DEL CASE C
		case 'D': //Destruir una cola (y sus mensajes)
			if ( (leido=read(s_conec,&size_cola , sizeof(int))) < 0){
				perror("error en read");
				return_to_client(s_conec,'-1');
				close(s);
				close(s_conec);
				return 1;
        	}//read
			name_cola = malloc(size_cola);
			if ( (leido=read(s_conec, name_cola, size_cola)) < 0){
				perror("error en read");
				return_to_client(s_conec,'-1');
				close(s);
				close(s_conec);
				return 1;
        	}//read
			if(DEBUG)
				printf("He destruido la cola %s de tamaño %d \n", name_cola, size_cola-1);
			if (dic_remove_entry(dic, name_cola, free_cola) < 0){
				if (DEBUG)
					perror("No existe ese nombre de cola \n");
				return_to_client(s_conec, '-1');
				continue;
			}
			if (DEBUG)
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
			name_cola = malloc(size_cola);
			if ( (leido=read(s_conec, name_cola, size_cola)) < 0){
				perror("error en read");
				return_to_client(s_conec,'-1');
				close(s);
				close(s_conec);
				return 1;
        	}//read
			if ( (leido=read(s_conec,&size_msg_ , sizeof(uint32_t))) < 0){
				perror("error en read");
				return_to_client(s_conec,'-1');
				close(s);
				close(s_conec);
				return 1;
        	}//read
			if (DEBUG){
				printf("He seleccionado la cola %s \n", name_cola);			
				printf("Tamaño del mensaje a introducir %d\n", size_msg_);
			}
			msg = malloc(size_msg_);
			if ( (leido=read(s_conec, msg, size_msg_)) < 0){
				perror("error en read");
				return_to_client(s_conec,'-1');
				close(s);
				close(s_conec);
				return 1;
        	}//read						
			cola= get_cola(dic,name_cola);
			if (cola == NULL){
				return_to_client(s_conec,'-1');
				free(name_cola);
				continue;				
			}
			cola_push_back(cola, size_msg_);
			cola_push_back(cola, msg);
			if (DEBUG){
				printf("Contenido de la cola\n");
				see_cola(cola);
			}
			free(name_cola);			
			break;
		//FIN DEL CASE P
		case 'G': //Lectura de mensaje (no bloqueante)
			if ( (leido=read(s_conec,&size_cola , sizeof(int))) < 0){
				perror("error en read");
				return_to_client(s_conec,'-1');
				close(s);
				close(s_conec);
				return 1;
        	}//read
			name_cola = malloc(size_cola);
			if ( (leido=read(s_conec, name_cola, size_cola)) < 0){
				perror("error en read");
				return_to_client(s_conec,'-1');
				close(s);
				close(s_conec);
				return 1;
        	}//read
			if (DEBUG)
				printf("He seleccionado la cola %s  para leer \n", name_cola);	
			cola= get_cola(dic,name_cola);
			if (cola == NULL){
				send_msg_to_client(s_conec,NULL, 0, false);
				free(name_cola);
				close(s_conec);
				continue;
			}
			size_msg_ = get_msg(cola);
			if (size_msg_ == NULL){
				send_msg_to_client(s_conec,NULL, 0, true);
				free(name_cola);
				close(s_conec);

				continue;
			}
			msg = get_msg(cola);	
			if (msg == NULL){
				send_msg_to_client(s_conec,NULL, 0, true);
				free(name_cola);
				close(s_conec);

				continue;
			}
			if (DEBUG){
				printf("Tamaño del mensaje a leer %d\n", size_msg_);
			}
			send_msg_to_client(s_conec, msg, size_msg_, false);	
			//free(name_cola);
			break;
		//FIN DEL CASE G
		default: // Lectura bloqueante de mensaje
			//INCOMPLETO
			if ( (leido=read(s_conec,&size_cola , sizeof(int))) < 0){
				perror("error en read");
				return_to_client(s_conec,'-1');
				close(s);
				close(s_conec);
				return 1;
        	}//read
			name_cola = malloc(size_cola);
			if ( (leido=read(s_conec, name_cola, size_cola)) < 0){
				perror("error en read");
				return_to_client(s_conec,'-1');
				close(s);
				close(s_conec);
				return 1;
        	}//read
			if (DEBUG)
				printf("He seleccionado la cola %s  para leer \n", name_cola);	
			cola= get_cola(dic,name_cola);
			if (cola == NULL){
				send_msg_to_client(s_conec,NULL, 0, false);
				free(name_cola);
				close(s_conec);
				continue;
			}
			msg = get_msg(cola);
			if (msg == NULL){
				printf("No hay mensajes, me quedo bloqueado hasta tener uno\n");
				do{
					msg = get_msg(cola);
				} while (msg == NULL);
			}
			send_msg_to_client(s_conec, msg, 0, false);			
			break;
		}// switch
		return_to_client(s_conec, '0');
		close(s_conec);
	}
	close(s);
	return 0;
}


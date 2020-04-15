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

bool DEBUG = false; 
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
		//El cliente recibe un size_msg=0 que le indica que la cola está vacia
	}
	else if (msg == NULL){
		int a =-1;
		iov[0].iov_base= &a;
		iov[0].iov_len= sizeof(a);
		writev(fd, iov,1);
		//El cliente recibe un size_msg=-1 que le indica que no exisitia esa cola 
	}
	else{
		int a = tam;

		iov[0].iov_base=&a;
		iov[0].iov_len=sizeof(a);
		//writev(fd, iov,1);

		iov[1].iov_base=msg;
		iov[1].iov_len=a;
    	int desp= sizeof(a) + a;

    	/*
		do{
      		desp+= writev(fd, iov,1);
       		iov[0].iov_base= msg + desp; 
			iov[0].iov_len = a - desp;
			if (iov[0].iov_len < 0)
				iov[0].iov_len=0;
   		} while (desp != tam);
		*/
	    int i=true;
    	while (desp > 0) {
        	int bytes_written = writev(fd, iov,2);
        	if (bytes_written <= 0) {
            	// handle errors
        	}
        	desp -= bytes_written;
        	if(i){
            	bytes_written-= sizeof(a);
            	i = false;
        	}
        	iov[1].iov_base += bytes_written;
        	iov[1].iov_len -= bytes_written;
        	if (iov[1].iov_len < 0)
            	iov[1].iov_len=0; 
    	} //while   		   
		//El cliente recibe un size_msg>0 y el mensaje correspondiente
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
//	printf("%s\n", msg);
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
void *get_data_from_cola (struct cola *c){
	void * aux;
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
	struct diccionario *dic_colas; //Diccionario que guarda las colas de mensajes existentes
	struct diccionario *dic_bloqueados; //Diccionario que guarda las colas vacias que han solicitado una lectura bloqueante
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
		//return_to_client(s_conec,'-1');
		close(s);
		return 1;
	}
	if (listen(s, 5) < 0) {
		perror("error en listen");
		//return_to_client(s_conec,'-1');
		close(s);
		return 1;
	}
	dic_colas = dic_create();
	if (dic_colas==NULL){
		perror("Error al crear el diccionario");
		//return_to_client(s_conec,'-1');
		close(s);
		return 1;
	}
	dic_bloqueados = dic_create();
	if (dic_bloqueados==NULL){
		perror("Error al crear el diccionario");
		//return_to_client(s_conec,'-1');
		close(s);
		return 1;
	}
	while (1) {

		tam_dir=sizeof(dir_cliente);
		if ((s_conec=accept(s, (struct sockaddr *)&dir_cliente, &tam_dir))<0){
			perror("error en accept");
			//return_to_client(s_conec,'-1');
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
			if (dic_put(dic_colas, name_cola, cola) <0){
				if (DEBUG)
					perror("Nombre de cola ya existente \n");
				return_to_client(s_conec,'-1');
				continue;
			}//if
			if (DEBUG)
				see_dic(dic_colas);
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
			if (dic_remove_entry(dic_colas, name_cola, free_cola) < 0){
				if (DEBUG)
					perror("No existe ese nombre de cola \n");
				return_to_client(s_conec, '-1');
				continue;
			}
			if (DEBUG)
				see_dic(dic_colas);
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
			/*
			if (DEBUG){
				printf("He seleccionado la cola %s \n", name_cola);			
				printf("Tamaño del mensaje a introducir %d\n", size_msg_);
			}
			*/
			msg = malloc(size_msg_);
			struct iovec test[1];
			test[0].iov_base = msg;
			test[0].iov_len = size_msg_;
			ssize_t desp=size_msg_;
    		/*
			do{
    			desp += readv(s_conec, test, 1);;
       			test[0].iov_base=msg+desp;
				test[0].iov_len= size_msg_ - desp;
				if (test[0].iov_len < 0)
					test[0].iov_len = 0;
    		} while (desp != size_msg_);
			*/
			while (desp>0){
				int bytes_read = readv(s_conec, test,1);
				desp -= bytes_read;
				test[0].iov_base += bytes_read;
				test[0].iov_len -=bytes_read;
				if (test[0].iov_len < 0)
					test[0].iov_len = 0;
			}//while
			
			cola= get_cola(dic_colas,name_cola);
			if (cola == NULL){
				return_to_client(s_conec,'-1');
				free(name_cola);
				continue;				
			}
			cola_push_back(cola, size_msg_);
			cola_push_back(cola, msg);
			/*
			if (DEBUG){
				printf("Contenido de la cola\n");
				see_cola(cola);
			}
			*/
			free(name_cola);			
			break;
		//FIN DEL CASE P
		case 'G': //Lectura de mensaje (no bloqueante)
		default: ;
			if ( (leido=read(s_conec,&size_cola , sizeof(int))) < 0){
				perror("error en read");
				//return_to_client(s_conec,'-1');
				close(s);
				close(s_conec);
				return 1;
        	}//read
			name_cola = malloc(size_cola);
			if ( (leido=read(s_conec, name_cola, size_cola)) < 0){
				perror("error en read");
				//return_to_client(s_conec,'-1');
				close(s);
				close(s_conec);
				return 1;
        	}//read
			/*
			if (DEBUG)
				printf("He seleccionado la cola %s  para leer \n", name_cola);	
			*/
			bool vacio = false;
			size_msg_=NULL;
			msg=NULL;
			cola= get_cola(dic_colas,name_cola);
			/*
			if (cola == NULL){
				send_msg_to_client(s_conec,NULL, 0, false);
				//Le indico al cliente que no existe esta cola (msg =NULL y tam = 0)
				free(name_cola);
				close(s_conec);
				continue;
			}
			*/
			if (cola != NULL){
				//La cola existe
				size_msg_ = get_data_from_cola(cola);
				msg = get_data_from_cola(cola);
				if (size_msg_==NULL && msg == NULL)
					vacio=true;	
					//La cola esta vacia
			}
			/*
			if (DEBUG){
				printf("Tamaño del mensaje a leer %d\n", size_msg_);
			}
*/
			send_msg_to_client(s_conec, msg, size_msg_, vacio);
			
			free(name_cola);	
			//He obtenido la cola que almacena el mensaje
			/*
			size_msg_ = get_data_from_cola(cola);
			if (size_msg_ == NULL){
				send_msg_to_client(s_conec,NULL, 0, true);
				//Le indico al cliente que la cola está vacia (vacio =true)
				free(name_cola);
				close(s_conec);
				continue;
			}
			//He obtenido el tamaño del mensaje
			msg = get_data_from_cola(cola);	
			
			if (msg == NULL){
				send_msg_to_client(s_conec,NULL, 0, true);
				//Le indico al cliente que la cola esta vacia (vacio=true)
				free(name_cola);
				close(s_conec);
				continue;
			}
			
			//He obtenido el mensaje
			*/
			

			

		//	send_msg_to_client(s_conec, msg, size_msg_, false);	
			//Le mando al cliente el mensaje y su tamaño
			//free(name_cola);
			break;
		//FIN DEL CASE G
		//default: // Lectura bloqueante de mensaje
		//	break;
		}// switch
		if (buf[0]!= 'G'){
			return_to_client(s_conec, '0');
		}
		close(s_conec);
	}
	close(s);
	return 0;
}


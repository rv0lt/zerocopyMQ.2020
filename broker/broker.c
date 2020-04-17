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
#include <limits.h>

bool DEBUG = false;
int par = 0;
/*----------------------------------------------------*/
void read_error(int s, int s_conec, char *error)
{
	perror(error);
	return_to_client(s_conec, -1);
	close(s);
	close(s_conec);
}
/*----------------------------------------------------*/
void return_to_client(int fd, int aux)
{
	struct iovec iov[1];

	iov[0].iov_base = &aux;
	iov[0].iov_len = sizeof(aux);

	writev(fd, iov, 1);
}
int send_msg_to_client(int fd, void *msg, uint32_t tam, bool vacio)
{
	struct iovec iov[2];
	if (vacio)
	{
		int a = 0;
		iov[0].iov_base = &a;
		iov[0].iov_len = sizeof(a);
		writev(fd, iov, 1);
		//El cliente recibe un size_msg=0 que le indica que la cola está vacia
		return 2;
	}
	else if (msg == NULL)
	{
		int a = -1;
		iov[0].iov_base = &a;
		iov[0].iov_len = sizeof(a);
		writev(fd, iov, 1);
		//El cliente recibe un size_msg=-1 que le indica que no exisitia esa cola
		return 3;
	}
	else
	{
		char *bf[2];
		if (recv(fd,  bf, sizeof(bf), MSG_PEEK | MSG_DONTWAIT) == 0) {
			// if recv returns zero, that means the connection has been closed:
			close(fd);
			return -2;
		}
		uint32_t a = tam;
		iov[0].iov_base = &a;
		iov[0].iov_len = sizeof(a);
		iov[1].iov_base = msg;
		iov[1].iov_len = a;
		uint32_t desp = sizeof(a) + a;
		bool i = true;
		while (desp > 0)
		{
			ssize_t bytes_written = writev(fd, iov, 2);
			if (bytes_written <= 0)
			{
				perror("writev");
				return -1;
			}
			desp -= bytes_written;
			if (i)
			{
				bytes_written -= sizeof(a);
				i = false;
			}
			iov[1].iov_base += bytes_written;
			iov[1].iov_len -= bytes_written;
			if (iov[1].iov_len < 0)
				iov[1].iov_len = 0;
		} //while
		  //El cliente recibe un size_msg>0 y el mensaje correspondiente
		return 5;
	}
}
/*---------------------------------------------------*/
void destroy_colas_bloqueadas(void *v){
	int desc = v;
	return_to_client(desc, -1);
}
void free_cola(char *c, void *v)
{
	free(c);
	struct cola *cl = v;
	cola_destroy(cl, NULL);
}
void print_dic(char *c, void *v)
{
	printf("Nombre de la cola: %s \n", c);
}
void see_dic(struct diccionario *d)
{
	printf("--------------\n");
	printf("CONTENIDO DEL DICCIONARIO \n");
	dic_visit(d, print_dic);
	printf("--------------\n");
}
void print_cola(void *v)
{
	if (!par)
	{
		uint32_t tam = v;
		printf("/// Tamaño del msg %d ///\n", tam);
		par = 1;
	}
	else
	{
		void *msg = v;
		printf("%s\n", msg);
		par = 0;
	}
}
void print_colaB(void *v)
{
	int des = v;
	printf("///Descriptor Nº %d \n", des);
}
void see_cola(struct cola *c)
{
	printf("_______________\n");
	printf("CONTENIDO DE LA COLA\n");
	par = 0;
	cola_visit(c, print_cola);
	printf("_______________\n");
}
void see_colaB(struct cola *c)
{
	printf("_______________\n");
	printf("CONTENIDO DE LA COLA\n");
	cola_visit(c, print_colaB);
	printf("_______________\n");
}
/*----------------------------------------------------------------------*/
struct cola *get_cola(struct diccionario *d, char *name_cola)
{
	struct cola *aux;
	int error;
	aux = dic_get(d, name_cola, &error);
	if (error < 0)
	{
		if (DEBUG)
			perror("Cola no existente");
		return NULL;
	}
	else
		return aux;
}
void *get_data_from_cola(struct cola *c)
{
	void *aux;
	int error;
	aux = cola_pop_front(c, &error);
	if (error < 0)
	{
		if (DEBUG)
			perror("Cola vacía");
		return NULL;
	}
	else
		return aux;
}
/*-----------------------------------------------------*/
int main(int argc, char *argv[])
{
	int s, s_conec, leido;
	unsigned int tam_dir;
	struct sockaddr_in dir, dir_cliente;
	char buf[8];
	int size_cola;
	uint32_t size_msg_;
	char *pchar;
	struct cola *cola, *colaB;
	int opcion = 1;
	struct diccionario *dic_colas;		//Diccionario que guarda las colas de mensajes existentes
	struct diccionario *dic_bloqueados; //Diccionario que guarda las colas vacias que han solicitado una lectura bloqueante
	void *msg;
	char *name_cola;

	if (argc != 2)
	{
		fprintf(stderr, "Uso: %s puerto\n", argv[0]);
		return 1;
	}
	if ((s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		perror("error creando socket");
		return 1;
	}
	/* Para reutilizar puerto inmediatamente */
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opcion, sizeof(opcion)) < 0)
	{
		perror("error en setsockopt");
		return 1;
	}
	dir.sin_addr.s_addr = INADDR_ANY;
	dir.sin_port = htons(atoi(argv[1]));
	dir.sin_family = PF_INET;
	if (bind(s, (struct sockaddr *)&dir, sizeof(dir)) < 0)
	{
		perror("error en bind");
		//return_to_client(s_conec,'-1');
		close(s);
		return 1;
	}
	if (listen(s, 5) < 0)
	{
		perror("error en listen");
		//return_to_client(s_conec,'-1');
		close(s);
		return 1;
	}
	dic_colas = dic_create();
	if (dic_colas == NULL)
	{
		perror("Error al crear el diccionario");
		//return_to_client(s_conec,'-1');
		close(s);
		return 1;
	}
	dic_bloqueados = dic_create();
	if (dic_bloqueados == NULL)
	{
		perror("Error al crear el diccionario");
		//return_to_client(s_conec,'-1');
		close(s);
		return 1;
	}
	while (1)
	{

		tam_dir = sizeof(dir_cliente);
		if ((s_conec = accept(s, (struct sockaddr *)&dir_cliente, &tam_dir)) < 0)
		{
			perror("error en accept");
			//return_to_client(s_conec,'-1');
			close(s);
			return 1;
		}
		if ((leido = read(s_conec, buf, sizeof(pchar))) < 0)
		{
			read_error(s, s_conec, "Error en read");
			return 1;
		} //read de la operacion
		if (DEBUG)
			printf("Se ha seleccionado la operacion: %c \n", buf[0]);
		switch (buf[0])
		{
		case 'C': //Crear una nueva cola
			if ((leido = read(s_conec, &size_cola, sizeof(int))) < 0)
			{
				read_error(s, s_conec, "Error en read");
				return 1;
			} //read
			name_cola = malloc(size_cola);
			if (recv(s_conec, name_cola, size_cola, MSG_WAITALL) < 0)
			{
				read_error(s, s_conec, "Error en recv");
				return 1;
			}
			if (DEBUG)
				printf("He creado la cola %s de tamano %d \n", name_cola, size_cola - 1);
			cola = cola_create();
			if (cola == NULL)
			{
				read_error(s, s_conec, "Error al crear la estructura cola");
				return 1;
			} //cola_create
			if (dic_put(dic_colas, name_cola, cola) < 0)
			{
				if (DEBUG)
					perror("Nombre de cola ya existente \n");
				return_to_client(s_conec, -1);
				continue;
			} //dic_put
			if (DEBUG)
				see_dic(dic_colas);
			break;
		//FIN DEL CASE C
		case 'D': //Destruir una cola (y sus mensajes)
			if ((leido = read(s_conec, &size_cola, sizeof(int))) < 0)
			{
				read_error(s, s_conec, "Error en read");
				return 1;
			} //read
			name_cola = malloc(size_cola);
			if (recv(s_conec, name_cola, size_cola, MSG_WAITALL) < 0)
			{
				read_error(s, s_conec, "Error en recv");
				return 1;
			} //recv
			if (DEBUG)
				printf("He destruido la cola %s de tamaño %d \n", name_cola, size_cola - 1);
			if (dic_remove_entry(dic_colas, name_cola, free_cola) < 0)
			{
				if (DEBUG)
					perror("No existe ese nombre de cola \n");
				return_to_client(s_conec, -1);
				continue;
			} //dic_remove_entry
			struct cola *colaB = get_cola(dic_bloqueados, name_cola);
			if (colaB != NULL){
			//En este caso la cola tiene lecturas bloqueantes pendientes
				cola_visit(colaB,destroy_colas_bloqueadas);
				dic_remove_entry(dic_bloqueados, name_cola, free_cola);
			}//cola_aux !=NULL			

			break;
		//FIN DEL CASE D
		case 'P': //Poner un nuevo mensaje a una cola determinada
			if ((leido = read(s_conec, &size_cola, sizeof(int))) < 0)
			{
				read_error(s, s_conec, "Error en read");
				return 1;
			} //read
			name_cola = malloc(size_cola);
			if (recv(s_conec, name_cola, size_cola, MSG_WAITALL) < 0)
			{
				read_error(s, s_conec, "Error en recv");
				return 1;
			} //recv
			if ((leido = read(s_conec, &size_msg_, sizeof(uint32_t))) < 0)
			{
				read_error(s, s_conec, "Error en read");
				return 1;
			} //read
			/*
			if (DEBUG){
				printf("He seleccionado la cola %s \n", name_cola);			
				printf("Tamaño del mensaje a introducir %d\n", size_msg_);
			}
			*/
			cola = get_cola(dic_colas, name_cola);
			if (cola == NULL)
			{
				return_to_client(s_conec, -1);
				free(name_cola);
				continue;
			} //get_cola
			msg = malloc(size_msg_);
			if (recv(s_conec, msg, size_msg_, MSG_WAITALL) < 0)
			{
				read_error(s, s_conec, "Error en recv");
				return 1;
			} //recv
			/*
			Antes de guardar el mensaje en el diccionario de colas comprobamos si en dicha cola
			existe una lectura bloqueante
			*/
			struct cola *cola_aux = get_cola(dic_bloqueados, name_cola);
			if (cola_aux != NULL){
				int desc;
				int len = cola_length(cola_aux);
				int res = -1;
				while (desc !=NULL && len > 0 && res < 0){
					desc = get_data_from_cola(cola_aux);
					if (desc != NULL){
						res = send_msg_to_client(desc, msg, size_msg_, false);
						// Si la cola se ha quedado vacia la elimino
						len = cola_length(cola_aux);
						if (len < 0){
							read_error(s, s_conec, "Error en cola_lenght");
							return 1;
						}
						if (len == 0){
							if (dic_remove_entry(dic_bloqueados, name_cola, free_cola) < 0){
							if (DEBUG)
								perror("No deberia entrar nunca \n");
							printf("TEST\n");
							break;
							}//dic_remove_entry						
						}//len ==0
					}//desc != null
				}//while
				if (res > 0){
					close(desc);
					free(name_cola);
					break;
				}
			}//envio a cola bloqueada
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
		case 'G': //Lectura de mensaje
		default:;
			if ((leido = read(s_conec, &size_cola, sizeof(int))) < 0)
			{
				read_error(s, s_conec, "Error en read");
				return 1;
			} //read
			name_cola = malloc(size_cola);
			if (recv(s_conec, name_cola, size_cola, MSG_WAITALL) < 0)
			{
				read_error(s, s_conec, "Error en recv");
			}
			/*
			if (DEBUG)
				printf("He seleccionado la cola %s  para leer \n", name_cola);	
			*/
			bool vacio = false;
			size_msg_ = NULL;
			msg = NULL;
			cola = get_cola(dic_colas, name_cola);
			if (cola != NULL)
			{
				//La cola existe
				size_msg_ = get_data_from_cola(cola);
				msg = get_data_from_cola(cola);
				if (size_msg_ == NULL && msg == NULL)
				{
					//La cola esta vacia
					vacio = true;
					if (buf[0] == 'B')
					{
						struct cola *cola_aux = get_cola(dic_bloqueados, name_cola);
						if (cola_aux == NULL){
							colaB = cola_create();
							if (colaB == NULL){
								read_error(s, s_conec, "Error al crear la estructura cola");
								return 1;
							}//cola_create
							int desc = s_conec;	
							if (dic_put(dic_bloqueados, name_cola, colaB) < 0){
								if (DEBUG)
									perror("No deberia entrar \n");
								return_to_client(s_conec, -1);
								continue;
							}//dic_put	
							cola_push_back(colaB, desc);
							//see_colaB(colaB);
						} //cola nunca ha sido bloqueada
						else if (cola_aux != NULL)
						{
							int desc = s_conec;
							cola_push_back(cola_aux, desc);
							//see_colaB(cola_aux);
						} //cola ya ha sido bloqueada
						//see_dic(dic_bloqueados);
						continue;
						//forzar otra interacción del blucle while(1) sin cerrar el descriptor
					}//if bloqueante vacia
				}//cola vacia
			}
			/*
			if (DEBUG){
				printf("Tamaño del mensaje a leer %d\n", size_msg_);
			}
			*/
			send_msg_to_client(s_conec, msg, size_msg_, vacio);
			free(name_cola);
			//He obtenido la cola que almacena el mensaje
			break;
			//FIN DEL CASE G
		} // switch
		if (buf[0] != 'G' && buf[0] != 'B')
		{
			return_to_client(s_conec, 0);
		}
		close(s_conec);
	}
	close(s);
	return 0;
}

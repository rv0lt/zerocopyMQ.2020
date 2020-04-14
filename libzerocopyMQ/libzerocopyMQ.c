#include <stdint.h>
#include "zerocopyMQ.h"
#include "comun.h"
#include "comun.c"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*
export BROKER_HOST=Rv0lt
export BROKER_PORT=12345
*/

char *pchar;
char buf[TAM];
int leido;
bool DEBUG = true; 
/*-----------------------------------------------*/
int connect_socket(){

    int fd; //descriptor del socket
    struct hostent *host_info;
    struct sockaddr_in dir;
    
    if ( (fd= socket(PF_INET, SOCK_STREAM,0))  <0){
        perror("socket error");
        return -1;
    }
    host_info = gethostbyname(getenv("BROKER_HOST"));
    if (host_info == NULL){
        perror ("error en el nombre del host");
        return -1;
    }

    dir.sin_addr=*(struct in_addr *)host_info->h_addr_list[0];
    dir.sin_family=PF_INET;
    dir.sin_port=htons(atoi(getenv("BROKER_PORT")));



	if (connect(fd, (struct sockaddr *)&dir, sizeof(dir)) < 0){
		perror("error en conect");
        close(fd);
        return -1;
	}
	
    return fd;
}
/*-------------------------------------------------*/
int wait_response(int fd){
    if ( (leido=read(fd, buf, sizeof(pchar))) < 0){
		perror("error en read");
		close(fd);
		return -1;
    }
    close(fd);
    if (buf[0]!= '0')
        return -1;
    else 
        return 0;
}
/*----------------------------------------------------------------------------*/
int createMQ(const char *cola){
    int fd;
    struct iovec iov[3];    
    pchar = 'C';
    int a= strlen(cola)+1;
    if ((fd= connect_socket(&fd))<0){
        return -1;
    }
    printf("%d\n",strlen(cola));
    if (strlen(cola) > TAM){
        perror("Nombre de cola demasiado grande");
        return -1;
    }

    iov[0].iov_base= &pchar;
    iov[0].iov_len= sizeof(pchar) ;
    iov[1].iov_base= &a;
    iov[1].iov_len= sizeof(a);
    iov[2].iov_base= cola;
    iov[2].iov_len= a; 
    writev(fd, iov,3);
    return wait_response(fd);
}

int destroyMQ(const char *cola){
    int fd;
    struct iovec iov[3];
    pchar = 'D';
    int a= strlen(cola)+1;
    if (sizeof(cola) > TAM){
        perror("Nombre de cola demasiado grande");
        return -1;
    }

    if ((fd= connect_socket(&fd))<0){
        return -1;
    }
    
    iov[0].iov_base= &pchar;
    iov[0].iov_len= sizeof(pchar) ;
    iov[1].iov_base= &a;
    iov[1].iov_len= sizeof(a);
    iov[2].iov_base= cola;
    iov[2].iov_len= a;  
    writev(fd, iov,3);

    return wait_response(fd);
}
int put(const char *cola, const void *mensaje, uint32_t tam) {
    int fd;
    struct iovec iov[5];
    pchar = 'P';
    int a= strlen(cola)+1;
    if (tam > TAM_MAX_MESSAGE){
        perror("Tamaño del mensaje demasiado grande");
        return -1;
    }
    if ((fd= connect_socket(&fd))<0){
        return -1;
    }
    iov[0].iov_base= &pchar;
    iov[0].iov_len= sizeof(pchar);
    iov[1].iov_base= &a;
    iov[1].iov_len= sizeof(a);
    iov[2].iov_base= cola;
    iov[2].iov_len= a; 
    iov[3].iov_base= &tam;
    iov[3].iov_len= sizeof(tam);
    iov[4].iov_base= mensaje;
    iov[4].iov_len= tam;
    if (DEBUG){
        printf ("Tamaño del mensaje que estoy guardando: %d \n", tam);
       // print("Contenido del mensaje: \n%s \n", mensaje);
    }
    writev(fd, iov,5);
    return wait_response(fd);
}
int get(const char *cola, void **mensaje, uint32_t *tam, bool blocking) {
    int fd;
    struct iovec iov[3];
    int size_msg;
    int a= strlen(cola)+1;
    if (sizeof(cola) > TAM){
        perror("Nombre de cola demasiado grande");
        return -1;
    }
    if ((fd= connect_socket(&fd))<0){
        return -1;
    }

    if(!blocking)
        pchar = 'G';
    else
        pchar = 'B';
    
    iov[0].iov_base= &pchar;
    iov[0].iov_len= sizeof(pchar) ;    
    iov[1].iov_base= &a;
    iov[1].iov_len= sizeof(a);
    iov[2].iov_base= cola;
    iov[2].iov_len= a;    
    writev(fd, iov,3);
    
	if ( (leido=read(fd,tam, sizeof(uint32_t))) < 0){
		perror("error en read");
		close(fd);
		return 1;
    }//read
	//printf("_-_-_-%d\n", size_msg);
	printf("_-_-_-%d\n", leido);

    //memcpy(tam, &size_msg, leido);
    printf("_-_-_-%d\n", *tam);

    if(*tam == 0){
        if (!blocking)
            return 0;
        else{
        //La cola estaba vacia y la operacion era el get bloqueante
        //TODO dejar bloqueado al proceso y esperar a que le llegue un mensaje
        
        printf("No implementado aun\n");
        return 0;
        }
    }
    else if (*tam == -1)
        return -1;
    else {    
	*mensaje = malloc(*tam);
    if ( (leido=read(fd, *mensaje, *tam)) < 0){
		perror("error en read");
		close(fd);
		return -1;
    }//read
    if (DEBUG)
        printf ("Tamaño del mensaje que estoy leeyendo: %d \n", *tam);
       // print("Contenido del mensaje: \n%s \n", mensaje);    return 1;
    }
}

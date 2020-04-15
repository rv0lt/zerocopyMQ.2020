#include <stdint.h>
#include "zerocopyMQ.h"
#include "comun.h"
#include "comun.c"
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#define _XOPEN_SOURCE 700
#define _POSIX_C_SOURCE 200809L

/*
export BROKER_HOST=Rv0lt
export BROKER_PORT=12345
*/

char *pchar;
char buf[TAM];
int leido;
bool DEBUG = false; 
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
    if (strlen(cola)+1 > TAM){
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
    if (sizeof(cola)+1 > TAM){
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
//    writev(fd, iov,4);

    iov[4].iov_base= mensaje;
    iov[4].iov_len= tam;
    int desp= sizeof(pchar) + sizeof(a) + a + sizeof(tam) + tam;
    /*
    do{
       desp+= writev(fd, iov,1);
       iov[0].iov_base= mensaje+ desp;
       iov[0].iov_len= tam - desp;
       if (iov[0].iov_len < 0)
            iov[0].iov_len=0; 
    } while (desp != tam);
    */
    int i=true;
    while (desp > 0) {
        int bytes_written = writev(fd, iov,5);
        if (bytes_written <= 0) {
            // handle errors
        }
        desp -= bytes_written;
        if(i){
            bytes_written-= sizeof(pchar) + sizeof(a) + a + sizeof(tam);
            i = false;
        }
        iov[4].iov_base += bytes_written;
        iov[4].iov_len -= bytes_written;
        if (iov[0].iov_len < 0)
            iov[0].iov_len=0; 
    } //while   
    if (DEBUG){
        printf ("Tamaño del mensaje que estoy guardando: %d \n", tam);
       // print("Contenido del mensaje: \n%s \n", mensaje);
    }
    return wait_response(fd);
}
int get(const char *cola, void **mensaje, uint32_t *tam, bool blocking) {
    int fd;
    struct iovec iov[3];
//    int size_msg;
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
	//printf("_-_-_-%d\n", leido);

    //memcpy(tam, &size_msg, leido);
    //printf("_-_-_-%d\n", *tam);

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
	struct iovec test[1];
	test[0].iov_base = *mensaje;
	test[0].iov_len = *tam;
	ssize_t desp=*tam;

	while (desp>0){
		int bytes_read = readv(fd, test,1);
		desp -= bytes_read;
		test[0].iov_base += bytes_read;
		test[0].iov_len -=bytes_read;
		if (test[0].iov_len < 0)
			test[0].iov_len = 0;
	}//while
    /*if ( (leido=read(fd, *mensaje, *tam)) < 0){
		perror("error en read");
		close(fd);
		return -1;
    }//read
    */
    if (DEBUG)
        printf ("Tamaño del mensaje que estoy leeyendo: %d \n", *tam);
       // print("Contenido del mensaje: \n%s \n", mensaje);    return 1;
    }
}

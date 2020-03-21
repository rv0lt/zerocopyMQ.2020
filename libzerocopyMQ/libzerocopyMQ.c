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
#define TAM  65536 //2¹⁶
#define TAM_MAX_MESSAGE pow(2,32) //2³² 
/*
export BROKER_HOST=Rv0lt
export BROKER_PORT=12345
*/

char *pchar;
char buf[TAM];
int leido;
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
    if (buf[0]!= '0')
        return -1;
    else 
        return 0;
}
/*----------------------------------------------------------------------------*/
int send_request(const unsigned int op, const char *cola, const void *mensaje, size_t tam, 
                    void **mensaje_2, size_t *tam_2, bool blocking) {
    int fd;
    char *s =0;
    if ((fd = connect_socket(&fd))< 0 ){
        return -1;
    }
    
    }
/*----------------------------------------------------------------------------*/
int createMQ(const char *cola){
    int fd;
    struct iovec iov[2];    
    pchar = 'C';

    if ((fd= connect_socket(&fd))<0){
        return -1;
    }
    if (sizeof(cola) > TAM){
        perror("Nombre de cola demasiado grande");
        return -1;
    }
    iov[0].iov_base= &pchar;
    iov[0].iov_len= sizeof(pchar) ;

   

    iov[1].iov_base=cola;
    iov[1].iov_len=sizeof(cola);  
    writev(fd, iov,2);
    /*--------------------------------*/

    return wait_response(fd);
}

int destroyMQ(const char *cola){
    int fd;
    struct iovec iov[2];
    pchar = 'D';

    if (sizeof(cola) > TAM){
        perror("Nombre de cola demasiado grande");
        return -1;
    }

    if ((fd= connect_socket(&fd))<0){
        return -1;
    }

    
    iov[0].iov_base= &pchar;
    iov[0].iov_len= sizeof(pchar) ;

    iov[1].iov_base=cola;
    iov[1].iov_len=sizeof(cola);  
    writev(fd, iov,2);

    return wait_response(fd);
}
int put(const char *cola, const void *mensaje, uint32_t tam) {
    int fd;
    struct iovec iov[2];
    pchar = 'P';
    struct message data;
    if (tam > TAM_MAX_MESSAGE){
        perror("Tamaño del mensaje demasiado grande");
        return -1;
    }
    if ((fd= connect_socket(&fd))<0){
        return -1;
    }

    /*
    Revisando el fichero test me doy cuenta que mensaje es un puntero a la posicion de memoria donde
    se ubica el mensasje a guardar
    IDEA: serializar los datos para poderlos mandar
    
	https://stackoverflow.com/questions/15707933/how-to-serialize-a-struct-in-c

    http://mgarciaisaia.github.io/tutorial-c/blog/2015/02/27/dream-of-serialization/
    */
    iov[0].iov_base= &pchar;
    iov[0].iov_len= sizeof(pchar) ;

    data.cola=cola;
    data.get_mensaje=mensaje;
    data.get_mes_tam=tam;
    data.put_mensaje=NULL;
    data.put_mes_tam=NULL;
    data.blocking=false;

    //TODO descubrir como serializar hulio

    writev(fd, iov,2);

    return 0;
}
int get(const char *cola, void **mensaje, uint32_t *tam, bool blocking) {
    return 0;
}

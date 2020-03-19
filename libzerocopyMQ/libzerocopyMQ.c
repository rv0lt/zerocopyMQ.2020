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

#define TAM  65536 //2¹⁶

int fd;
struct iovec iov[2];
char *pchar = 'C';
char buf[TAM];

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
    //writev(fd, iov,1);

    printf("-------%d\n", sizeof(cola));
    printf("__________%d\n", strlen(cola));
   
    
  //  iov[0].iov_base= strlen(cola);
  //  iov[0].iov_len= 8;

    iov[1].iov_base=cola;
    iov[1].iov_len=sizeof(cola);  
    writev(fd, iov,2);
    /*data.op='C';
    data.cola=cola;
    data.get_mensaje=NULL;
    data.get_mes_tam=0;
    data.put_mensaje=NULL;
    data.put_mes_tam=0;
    data.blocking=0;

    iov[0].iov_base=&data;
    iov[0].iov_len=sizeof(struct comun);
    writev(fd, iov,1);
    */
    /*
    while ((leido=read(0, buf, TAM))>0) {
                if (write(fd, buf, leido)<0) {
                        perror("error en write");
                        close(fd);
                        return 1;
                }
                if ((leido=read(fd, buf, TAM))<0) {
                        perror("error en read");
                        close(fd);
                        return 1;
                }
		write(1, buf, leido);
        }//while
        */

    
    return 0;
}

int destroyMQ(const char *cola){
    return 0;
}
int put(const char *cola, const void *mensaje, uint32_t tam) {
    return 0;
}
int get(const char *cola, void **mensaje, uint32_t *tam, bool blocking) {
    return 0;
}

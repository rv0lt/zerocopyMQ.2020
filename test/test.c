#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <inttypes.h>
#include <fcntl.h>
#include "zerocopyMQ.h"
int main(int argc, char *argv[]) {
    int n;
    char *op, *cola;

    int fd_msg1, fd_msg ,fd_cola, fd_new;
    struct stat st_msg1, st_msg,st_cola, st_new;
    fd_msg = open("../../../../DON4.txt", O_RDONLY);
    fd_cola = open("colanotanlarga.txt", O_RDONLY);
//  fd_cola = open("colalarga.txt", O_RDONLY);
    fstat(fd_msg, &st_msg);
    fstat(fd_cola, &st_cola);
    void *f_s=mmap(NULL, st_cola.st_size, PROT_READ, MAP_PRIVATE, fd_cola, 0);
    void *f_c=mmap(NULL, st_msg.st_size, PROT_READ, MAP_PRIVATE, fd_msg, 0);    

    printf("---------%"PRId64"\n",st_msg.st_size);
    printf("__________%d\n", st_cola.st_size);

    do {
        printf("\nSeleccione nombre de cola y operación (ctrl-D para terminar)\n\tC:create|D:destroy|P:put\n\tG:get no bloqueante|B:get bloqueante\n");
        op=cola=NULL;
        n=scanf("%ms%ms", &cola, &op);
        if (n==2) {
            switch(op[0]) {
                case 'C':
                    if (createMQ(f_s)<0)
                        printf("error creando la cola %s\n", cola);
                    else
                        printf("cola %s creada correctamente\n", cola);
                    break;
                case 'D':
                    if (destroyMQ(cola)<0)
                        printf("error eliminando la cola %s\n", cola);
                    else
                        printf("cola %s eliminada correctamente\n", cola);
                    break;
                case 'G': 
                case 'B': ;
                    uint32_t tam;
                    void *mensaje;
                    if (get(f_s, &mensaje, &tam, (op[0]=='B')?true:false)<0)
                        printf("error leyendo de la cola %s\n", cola);
                    else {
                        printf("lectura de la cola %s correcta\n", cola);
                        if (tam) {
                            char *fich=NULL;
                            int fd;
                            do {
                                printf("Introduzca el nombre del fichero para almacenar el mensaje: ");
                                n=scanf("%ms", &fich);
                                fd = creat(fich, 0644);
                                free(fich);
		            } while (fd == -1);
                            printf("////////////////%d\n", tam);
                            write(fd, mensaje, tam);
                            close(fd);
                            free(mensaje);
                        }
                        else printf("no hay mensajes\n");
                    }
                    break;
                case 'P': ;
                    char *fich=NULL;
                    int fd;
                    struct stat st;
                    void *f;
                    do {
                        printf("Introduzca el nombre del fichero que contiene el mensaje: ");
                        n=scanf("%ms", &fich);
                        fd = open(fich, O_RDONLY);
                        free(fich);
		    } while (fd == -1);
                    fstat(fd, &st);
                    f=mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
                    close(fd);
                    if (put(f_s, f, st.st_size)<0)
                        printf("error escribiendo en la cola %s\n", cola);
                    else
                        printf("escritura en la cola %s correcta\n", cola);
                    printf("-----------%d\n", st.st_size);
                    munmap(f, st.st_size);
                    break;
                default:
                    printf("operación no válida\n");
            }
            free(cola);
            free(op);
        }
    } while (n!=EOF);
    return 0;
}


/*
 * Incluya en este fichero todas las implementaciones que pueden
 * necesitar compartir el broker y la biblioteca, si es que las hubiera.
 */
#include <stdint.h>
#include "comun.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct comun
{
    char op;
    const char *cola;
    const void *put_mensaje;
    size_t put_mes_tam;
    void **get_mensaje; 
    size_t *get_mes_tam; 
    int blocking;
};

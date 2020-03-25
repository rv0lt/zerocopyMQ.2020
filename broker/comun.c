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

#define TAM  65536 //2¹⁶
#define TAM_MAX_MESSAGE pow(2,32) //2³² 

struct put_struct
{
    const void *put_mensaje;
    size_t put_mes_len;

};

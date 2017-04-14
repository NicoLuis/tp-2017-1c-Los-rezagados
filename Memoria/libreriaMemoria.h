/*
 * libreriaMemoria.h
 *
 *  Created on: 3/4/2017
 *      Author: utnso
 */

#ifndef MEMORIA_LIBRERIAMEMORIA_H_
#define MEMORIA_LIBRERIAMEMORIA_H_

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/queue.h>
#include <commons/log.h>
#include <commons/process.h>
#include <herramientas/sockets.h>


// TIPOS DE MENSAJES
#define CPU_TERMINO 100

//Archivo de Configuracion
int puertoMemoria;
int cantidadDeMarcos;
uint32_t tamanioDeMarcos;
int cantidadaEntradasCache;
int cachePorProceso;
char* algoritmoReemplazo;
int retardoMemoria;

t_list* lista_cpus;


void mostrarArchivoConfig();

void escucharKERNEL(void*);

void escucharCPU(void*);

#endif /* MEMORIA_LIBRERIAMEMORIA_H_ */

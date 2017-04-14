/*
 * libreriaConsola.h
 *
 *  Created on: 9/4/2017
 *      Author: utnso
 */

#ifndef LIBRERIACONSOLA_H_
#define LIBRERIACONSOLA_H_

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



//Archivo de Configuracion
char* ipKernel;
int puertoKernel;

void mostrarArchivoConfig();

int socket_kernel;

void finalizarPrograma(void*);
void leerComando(char* comando);

#endif /* LIBRERIACONSOLA_H_ */

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



//Archivo de Configuracion
int puertoMemoria;
int cantidadDeMarcos;
uint32_t tamanioDeMarcos;
int cantidadaEntradasCache;
int cachePorProceso;
char* algoritmoReemplazo;
int retardoMemoria;
int puertoCPU;
int puertoKernel;
char* ipKernel;

void mostrarArchivoConfig() {

	printf("El puerto de la MEMORIA es: %d\n", puertoMemoria);
	printf("La cantidad de Marcos es: %d\n", cantidadDeMarcos);
	printf("El tamaño de las Marcos es: %d\n", tamanioDeMarcos);
	printf("La cantidad de entradas de Cache habilitadas son: %d\n", cantidadaEntradasCache);
	printf("La cantidad maxima de entradas a la cache por proceso son: %d\n", cachePorProceso);
	printf("El algoritmo de reemplaco de la Cache es: %s\n", algoritmoReemplazo);
	printf("El retardo de la memoria es de: %d\n",retardoMemoria);
	printf("El puerto de la CPU es: %d\n", puertoCPU);
	printf("El puerto del KERNEL es: %d\n", puertoKernel);
	printf("La IP del KERNEL es: %s\n", ipKernel);
}

#endif /* MEMORIA_LIBRERIAMEMORIA_H_ */

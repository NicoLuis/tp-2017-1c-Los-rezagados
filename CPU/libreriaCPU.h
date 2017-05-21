#ifndef LIBRERIACPU_H_
#define LIBRERIACPU_H_

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
#include <herramientas/enum.h>
#include <signal.h>

//Archivo de Configuracion
char* ipKernel;
int puertoKernel;
char* ipMemoria;
int puertoMemoria;

t_log* logCPU;

void mostrarArchivoConfig();
void terminar();
void ultimaEjec();
void ejecutar();

int socket_kernel;
int socket_memoria;

bool ultimaEjecucion;

#endif

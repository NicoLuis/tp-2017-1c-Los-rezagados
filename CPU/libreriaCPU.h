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
int tamanioPagina;

t_log* logCPU;
t_log* logAnsisop;

void mostrarArchivoConfig();
void finalizarCPU();
void ultimaEjec();
void ejecutarInstruccion();
t_posicion traducirSP();

char* proximaInstruccion();
t_posicion escribirMemoria(t_posicion puntero, t_valor_variable valor);

int socket_kernel;
int socket_memoria;

bool ultimaEjecucion;
bool finalizado;

#endif

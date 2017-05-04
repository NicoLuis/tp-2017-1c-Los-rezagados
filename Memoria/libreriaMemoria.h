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
#include <herramientas/enum.h>


// TIPOS DE MENSAJES
#define CPU_TERMINO 100
#define KERNEL_TAMANIOPAGS 300
#define KERNEL_SCRIPT 301

//Archivo de Configuracion
int puertoMemoria;
int cantidadDeMarcos;
uint32_t tamanioDeMarcos;
int cantidadaEntradasCache;
int cachePorProceso;
char* algoritmoReemplazo;
int retardoMemoria;
t_list* lista_cpus;
t_list* listaFrames;
t_list* listaProcesos;
t_list* TLB;
t_log* log_memoria;

//PROCESO
typedef struct {
	uint32_t PID;
	int cantPaginas;
	int cantFramesAsignados;
	t_list* listaPaginas;
} t_proceso;

//FRAME
typedef struct {
	int nroFrame;
	int bit_modif;
	int pid;
} t_frame;

//PAGINA
typedef struct {
	uint8_t nroPag;
	int nroFrame;
	int bit_pres;
	int bit_uso;
} t_pag;


void mostrarArchivoConfig();

void escucharKERNEL(void*);

void escucharCPU(void*);

void* memoria_real;

void* reservarMemoria(int, int);

void lockProcesos();

void unlockProcesos();

void lockFrames();

void unlockFrames();

void lockFramesYProcesos();

void unlockFramesYProcesos();

void crearProcesoYAgregarAListaDeProcesos(uint32_t pid,	uint32_t cantidadDePaginas);

void escribirPaginaEnFS(uint32_t pid, uint8_t nroPag, void* contenido_pagina);

void liberarFramesDeProceso(uint32_t unPid);

void eliminarProcesoDeListaDeProcesos(uint32_t unPid);

t_list* crearEInicializarListaDePaginas(uint32_t cantidadDePaginas);

void ponerBitUsoEn1(uint32_t pid, uint8_t numero_pagina);

int paginaInvalida(uint32_t pid, uint8_t numero_pagina);

t_pag* buscarPaginaEnListaDePaginas(uint32_t pid, uint8_t numero_pagina);

void* obtenerContenido(int frame, int offset, int tamanio_leer);

void enviarContenidoCPU(void* contenido_leido, uint32_t tamanioContenido,int socket_cpu);

t_proceso* buscarProcesoEnListaProcesos(uint32_t pid);

void* pedirPaginaAFS(uint32_t pid, uint8_t numero_pagina);

void cargarPaginaAMemoria(uint32_t pid, uint8_t numero_pagina,void* paginaLeida, int accion);

int hayFramesLibres();

t_frame* buscarFrameLibre(uint32_t pid);

void escribirContenido(int frame, int offset, int tamanio_escribir,	void* contenido);


//Socket FS
int socket_fs;

//Variable Global para LRU
int cantAccesosMemoria;

//Semaforos

pthread_mutex_t mutexListaProcesos;
pthread_mutex_t mutexListaFrames;
pthread_mutex_t mutexTLB;
pthread_mutex_t mutexCantAccesosMemoria;
pthread_mutex_t mutexRetardo;
pthread_mutex_t mutexMemoriaReal;
pthread_mutex_t mutexFS;
pthread_mutex_t mutexDump;

#endif /* MEMORIA_LIBRERIAMEMORIA_H_ */

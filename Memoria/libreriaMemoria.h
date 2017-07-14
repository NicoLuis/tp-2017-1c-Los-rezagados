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
#include <sockets.h>
#include <enum.h>
#include "hexdump.h"

#define PID_RESERVADO 255 //sizeof(t_pid)-1

//Archivo de Configuracion
int puertoMemoria;
int cantidadDeMarcos;
int cantidadEntradasCache;
int cachePorProceso;
char* algoritmoReemplazo;
int retardoMemoria;
int tamanioDeMarcos;
int cantidadFramesEstructurasAdministrativas;
t_list* lista_cpus;
t_list* listaProcesos;
t_list* Cache;
t_log* log_memoria;

//PROCESO
typedef struct {
	t_pid PID;
	int cantFramesAsignados;
	int cantFramesUsados;
	int cantEntradasCache;
} t_proceso;

//FRAME
typedef struct {
	// cant posibles de frames 65536 (2¹⁶)
	// si usara t_num8 serian 256 (2⁸)
	// no uso t_num (2³²) xq se quiere achicar espacio
	t_num16 nroFrame;
	t_num16 nroPag;
	t_pid pid;
} t_frame;

//ENTRADA Cache
typedef struct {
	t_pid pid;
	t_num16 numPag;
	int ultimoAcceso;
	void* contenido;
} t_cache;

typedef struct HeapMetadata {
	t_num size;
	bool isFree;
}t_HeapMetadata;


void mostrarArchivoConfig();

void escucharKERNEL(void*);

void escucharCPU(void*);

void* memoria_real;


/*			LOCKS MUTEX			*/
void lockProcesos();
void unlockProcesos();
void lockFrames();
void unlockFrames();
void lockFramesYProcesos();
void unlockFramesYProcesos();

/*			LISTAPROCESOS			*/
void crearProcesoYAgregarAListaDeProcesos(t_pid pid);
void eliminarProcesoDeListaDeProcesos(t_pid pid);

/*			LECTURA-ESCRITURA			*/
void* obtenerContenido(t_pid pid, t_posicion puntero);
int escribirContenido(t_pid pid, t_posicion puntero, void* contenido);

/*			FRAMES			*/
void inicializarFrames();
int cantFramesLibres();
int buscarFrameLibre(t_pid pid);

/*			FRAMES POR PAGINA DE PROCESO			*/
int funcionHashing(t_pid pid, t_num16 nroPagina);
int paginaInvalida(t_pid pid, t_num16 nroPagina);
int buscarFramePorPidPag(t_pid pid, t_num16 nroPagina);
void liberarPagina(t_pid pid, t_num16 nroPagina);
void liberarFramesDeProceso(t_pid pid);

/*			MISC			*/
void terminarMemoria();







/*			CACHE 			*/

t_list* crearCache();
void liberarCacheDeProceso(t_pid pid);
void vaciarCache();
void eliminarCache(t_list* unaCache);


int estaEnCache(t_pid pid, t_num16 numero_pagina);
t_cache* crearRegistroCache(t_pid pid, t_num16 numPag);
void agregarEntradaCache(t_pid pid, t_num16 numero_pagina);
t_cache* buscarEntradaCache(t_pid pid, t_num16 numero_pagina);

int hayEspacioEnCache();
int Cache_Activada();

void algoritmoLRU(t_pid pid);

void* obtenerContenidoSegunCache(t_pid pid, t_posicion puntero);
void escribirContenidoSegunCache(t_pid pid, t_posicion puntero, void* contenido_escribir);



/*			COMANDOS			*/
void ejecutarComandos();

void dumpTodosLosProcesos();
void dumpEstructurasMemoriaTodosLosProcesos(FILE* archivoDump);
void dumpContenidoMemoriaProceso(t_pid pid, FILE* archivoDump);
void dumpProcesoParticular(t_pid pid);
void dumpCache();
void dumpTablaPaginas();
t_proceso* buscarProcesoEnListaProcesosParaDump(t_pid pid);



void mostrarContenidoTodosLosProcesos();
void mostrarContenidoDeUnProceso(t_pid pid);


//Variable Global para LRU
int cantAccesosMemoria;

//Semaforos

pthread_mutex_t mutexListaProcesos;
pthread_mutex_t mutexListaFrames;
pthread_mutex_t mutexCache;
pthread_mutex_t mutexCantAccesosMemoria;
pthread_mutex_t mutexRetardo;
pthread_mutex_t mutexMemoriaReal;
pthread_mutex_t mutexDump;

#endif /* MEMORIA_LIBRERIAMEMORIA_H_ */

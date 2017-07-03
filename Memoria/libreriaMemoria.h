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
#include "hexdump.h"


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
	t_num8 PID;
	int cantFramesAsignados;
} t_proceso;

//FRAME
typedef struct {
	// cant posibles de frames 65536 (2¹⁶)
	// si usara t_num8 serian 256 (2⁸)
	// no uso t_num (2³²) xq se quiere achicar espacio
	t_num16 nroFrame;
	t_num8 pid;
	t_num16 nroPag;
} t_frame;

//ENTRADA Cache
typedef struct {
	t_num8 pid;
	t_num8 numPag;
	t_num8 numFrame;
	int ultimoAcceso;
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
void crearProcesoYAgregarAListaDeProcesos(t_num8 pid);
void eliminarProcesoDeListaDeProcesos(t_num8 pid);

/*			LECTURA-ESCRITURA			*/
void* obtenerContenido(int frame, int offset, int tamanio_leer);
void escribirContenido(int frame, int offset, int tamanio_escribir,	void* contenido);

/*			FRAMES			*/
void inicializarFrames();
int hayFramesLibres(int cantidad);
int buscarFrameLibre(t_num8 pid);

/*			FRAMES POR PAGINA DE PROCESO			*/
int funcionHashing(t_num8 pid, uint8_t nroPagina);
int paginaInvalida(t_num8 pid, uint8_t nroPagina);
int buscarFramePorPidPag(t_num8 pid, t_num8 nroPagina);
void liberarPagina(t_num8 pid, t_num8 nroPagina);
void liberarFramesDeProceso(t_num8 pid);

/*			MISC			*/
void terminarMemoria();







/*			CACHE (y otros)			*/

t_list* crearCache();

t_cache* buscarEntradaCache(t_num8 pid, uint8_t numero_pagina);

int estaEnCache(t_num8 pid, uint8_t numero_pagina);

t_cache* crearRegistroCache(t_num8 pid, uint8_t numPag, int numFrame);

void eliminarCache(t_list* unaCache);

int hayEspacioEnCache();

void agregarEntradaCache(t_num8 pid, uint8_t numero_pagina, int nroFrame);

void* obtenerContenidoSegunCache(t_num8 pid, uint8_t numero_pagina,uint32_t offset, uint32_t tamanio_leer);

void escribirContenidoSegunCache(t_num8 pid, uint8_t numero_pagina,uint32_t offset, uint32_t tamanio_escritura, void* contenido_escribir);

void vaciarCache();

void borrarEntradasCacheSegunPID(t_num8 pid);

void borrarEntradaCacheSegunFrame(int nroFrame);

int Cache_Activada();

void algoritmoLRU();

void ejecutarComandos();

void dumpEstructurasMemoriaTodosLosProcesos(FILE* archivoDump);

void dumpTodosLosProcesos();

void dumpContenidoMemoriaProceso(t_num8 pid, FILE* archivoDump);

void dumpProcesoParticular(t_num8 pid);

t_proceso* buscarProcesoEnListaProcesosParaDump(t_num8 pid);

void mostrarContenidoTodosLosProcesos();

void mostrarContenidoDeUnProceso(t_num8 pid);


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

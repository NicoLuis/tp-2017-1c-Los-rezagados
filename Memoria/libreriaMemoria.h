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
t_list* listaFrames;
t_list* listaProcesos;
t_list* Cache;
t_log* log_memoria;

//PROCESO
typedef struct {
	t_num8 PID;
	int cantPaginas;
	int cantFramesAsignados;
	t_list* listaPaginas;
} t_proceso;

//FRAME
typedef struct {
	int nroFrame;
	int bit_modif;
	t_num8 pid;
} t_frame;

//PAGINA
typedef struct {
	uint8_t nroPag;
	int nroFrame;
	int bit_pres;
	int bit_uso;
} t_pag;

//ENTRADA Cache
typedef struct {
	t_num8 pid;
	uint8_t numPag;
	int numFrame;
	int ultimoAcceso;
} t_cache;


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

void crearProcesoYAgregarAListaDeProcesos(t_num8 pid,	uint32_t cantidadDePaginas);

void escribirPaginaEnFS(t_num8 pid, uint8_t nroPag, void* contenido_pagina);

void liberarFramesDeProceso(t_num8 unPid);

void eliminarProcesoDeListaDeProcesos(t_num8 unPid);

t_list* crearEInicializarListaDePaginas(uint32_t cantidadDePaginas);

void ponerBitUsoEn1(t_num8 pid, uint8_t numero_pagina);

int paginaInvalida(t_num8 pid, uint8_t numero_pagina);

t_pag* buscarPaginaEnListaDePaginas(t_num8 pid, uint8_t numero_pagina);

void* obtenerContenido(int frame, int offset, int tamanio_leer);

void enviarContenidoCPU(void* contenido_leido, uint32_t tamanioContenido,int socket_cpu);

t_proceso* buscarProcesoEnListaProcesos(t_num8 pid);

void* pedirPaginaAFS(uint32_t pid, uint8_t numero_pagina);

void cargarPaginaAMemoria(t_num8 pid, uint8_t numero_pagina,void* paginaLeida, int accion);

int hayFramesLibres();

t_frame* buscarFrameLibre(t_num8 pid);

void escribirContenido(int frame, int offset, int tamanio_escribir,	void* contenido);

void inicializarFrames();

void terminarProceso();

void ponerBitModificadoEn1(int nroFrame);

t_frame* buscarFrame(int numeroFrame);

int estaEnMemoriaReal(t_num8 pid, uint8_t numero_pagina);

int funcionHashing(t_num8 pid, uint8_t);

//Cache

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

void dumpTablaDePaginasDeProceso(t_proceso* proceso, FILE* archivoDump);

void dumpEstructurasMemoriaTodosLosProcesos(FILE* archivoDump);

void dumpTodosLosProcesos();

void dumpContenidoMemoriaProceso(t_proceso* proceso, FILE* archivoDump);

void dumpProcesoParticular(t_num8 pid);

void flushMemoria();

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

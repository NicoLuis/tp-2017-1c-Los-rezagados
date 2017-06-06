/*
 * libreriaKernel.h
 *
 *  Created on: 3/4/2017
 *      Author: utnso
 */

#ifndef LIBRERIAKERNEL_H_
#define LIBRERIAKERNEL_H_


#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
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
#include <commons/collections/dictionary.h>
#include <commons/log.h>
#include <commons/process.h>
#include <herramientas/sockets.h>
#include <herramientas/enum.h>
#include <parser/metadata_program.h>

#define FINALIZO_CORRECTAMENTE 0
#define RECURSOS_INSUFICIENTES -1
#define ARCHIVO_INEXISTENTE -2
#define LECTURA_SIN_PERMISOS -3
#define ESCRITURA_SIN_PERMISOS -4
#define EXCEPCION_MEMORIA -5
#define DESCONEXION_CONSOLA -6
#define COMANDO_FINALIZAR -7
#define MAS_MEMORIA_TAMANIO_PAG -8
#define CANT_PAGS_INSUFICIENTES -9
#define SIN_DEFINICION -20

typedef struct {
	char* nombre;
	t_valor_variable valor;
}t_VariableCompartida;
t_list* lista_variablesCompartidas;

typedef struct {
	char* nombre;
	sem_t semaforo;
}t_VariableSemaforo;
t_list* lista_variablesSemaforo;

typedef struct {
	int nroPag;
	int pid;
	int espacioLibre;
}t_heap;
t_list* tabla_heap;
// todo: "En el caso que el tamaño disponible en una página no sea suficiente para almacenar el valor requerido, se pedirá una nueva tabla a la Memoria."	?????

typedef struct HeapMetadata {
	t_num size;
	bool isFree;
}t_HeapMetadata;

typedef struct {
	int socket;
	t_num8 pid;
}t_infosocket;

typedef struct {
	int socket;
	t_num pid;
	sem_t sem;
	bool libre;
}t_cpu;


typedef struct {
	int pid;
	int cantRafagas;
	int cantOpPriv;
	int tablaArchivos; // fixme: wtf
	int cantPagsHeap;
	int cantOp_alocar;
	int canrBytes_alocar;
	int cantOp_liberar;
	int canrBytes_liberar;
	int cantSyscalls;
}t_infoProceso;
t_list* infoProcs;


t_list* lista_cpus;
t_list* lista_consolas;
t_list* lista_PCBs;
t_list* lista_PCB_consola;
t_list* lista_PCB_cpu;
t_num8 pid;

//Archivo de Configuracion
int puertoPrograma;
int puertoCPU;
char* ipMemoria;
int puertoMemoria;
char* ipFileSystem;
int puertoFS;
t_num quantum;
t_num quantumSleep;
char* algoritmo;
int gradoMultiprogramacion;
//t_list* identificadoresSemaforos;
//t_list* inicializacionSemaforos;
//t_list* identificadorVariables;
t_num8 stackSize;



t_queue* cola_New;
t_queue* cola_Ready;
t_queue* cola_Exec;
t_queue* cola_Block;
t_queue* cola_Exit;
pthread_mutex_t mutex_New;
pthread_mutex_t mutex_Ready;
pthread_mutex_t mutex_Exec;
pthread_mutex_t mutex_Block;
pthread_mutex_t mutex_Exit;


sem_t sem_gradoMp;
sem_t sem_cantColaReady;
sem_t sem_cantCPUs;




void mostrarArchivoConfig();
void inicializarSemaforosYVariables(char**, char**, char**);

int socket_memoria;
int socket_fs;
fd_set fdsMaestro;

t_log* logKernel;

t_num tamanioPag;

void escucharCPU(int);

int handshake(int socket_cliente, int tipo);
void atender_consola(int socket_consola);


void consolaKernel();
void terminarKernel();

void finalizarPid(t_num8 pid, int exitCode);

#endif /* LIBRERIAKERNEL_H_ */

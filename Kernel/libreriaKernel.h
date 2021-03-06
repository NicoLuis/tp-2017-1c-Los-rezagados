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
#include <sockets.h>
#include <enum.h>
#include <parser/metadata_program.h>

#define SIN_ASIGNAR 50
#define FINALIZO_CORRECTAMENTE 0
#define RECURSOS_INSUFICIENTES 1
#define ARCHIVO_INEXISTENTE 2
#define LECTURA_SIN_PERMISOS 3
#define ESCRITURA_SIN_PERMISOS 4
#define ERROR_EXCEPCION_MEMORIA 5
#define DESCONEXION_CONSOLA 6
#define COMANDO_FINALIZAR 7
#define MAS_MEMORIA_TAMANIO_PAG 8
#define CANT_PAGS_INSUFICIENTES 9
#define ERROR_SCRIPT 11
#define ERROR_STACKOVERFLOW 12
#define SEMAFORO_INEXISTENTE 13
#define VARIABLE_COMPARTIDA_INEXISTENTE 14
#define NO_HAY_BLOQUES_LIBRES 15
#define SIN_DEFINICION 20

//////////////////////// CAPA FS ////////////////////////////

typedef struct{
	t_descriptor_archivo fd;
	t_banderas bandera;
	uint referenciaGlobalTable;
	t_valor_variable cursor;
}t_entrada_proceso;

typedef struct{
	t_pid pid;
	uint fdMax;
	t_list* lista_entradas_de_proceso;

} t_tabla_proceso;

t_list* lista_tabla_de_procesos;
pthread_mutex_t mutex_tablaProcesos;

typedef struct{
	t_descriptor_archivo indiceGlobalTable;
	char* FilePath;
	uint Open;
}t_entrada_GlobalFile;

t_list* lista_tabla_global;
pthread_mutex_t mutex_tablaGlobal;


////////////////////////////////////////////////////////


typedef struct{
	uint indice;
	char Flags[2];
	uint GlobalFD;
}t_ProcessFile;

typedef struct {
	char* nombre;
	t_valor_variable valor;
}t_VariableCompartida;
t_list* lista_variablesCompartidas;
pthread_mutex_t mutex_listaVariables;

typedef struct {
	char* nombre;
	int valorSemaforo;
	t_queue* colaBloqueados;
	pthread_mutex_t mutex_colaBloqueados;
}t_VariableSemaforo;
t_list* lista_variablesSemaforo;
pthread_mutex_t mutex_listaSemaforos;

typedef struct {
	int nroPag;
	t_pid pid;
}t_heap;
t_list* tabla_heap;

typedef struct HeapMetadata {
	t_num size;
	bool isFree;
}t_HeapMetadata;

typedef struct {
	int socket;
	t_pid pidPCB;
}t_infosocket;

typedef struct {
	int socket;
	t_num pidProcesoCPU;
	//sem_t semaforo;
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
}t_infoProceso;
t_list* infoProcs;
pthread_mutex_t mutex_infoProcs;


t_list* lista_cpus;
t_list* lista_consolas;
t_list* lista_PCBs;
t_list* lista_PCB_consola;
t_list* lista_PCB_cpu;

pthread_mutex_t mutex_cpus;
pthread_mutex_t mutex_consolas;
pthread_mutex_t mutex_PCBs;
pthread_mutex_t mutex_PCB_consola;
pthread_mutex_t mutex_PCB_cpu;

void _lockLista_cpus();
void _unlockLista_cpus();
void _lockLista_Consolas();
void _unlockLista_Consolas();
void _lockLista_PCBs();
void _unlockLista_PCBs();
void _lockLista_PCB_consola();
void _unlockLista_PCB_consola();
void _lockLista_PCB_cpu();
void _unlockLista_PCB_cpu();
void _lockLista_infoProc();
void _unlockLista_infoProc();
void _lockMemoria();
void _unlockMemoria();
void _lockFS();
void _unlockFS();

t_pid pid;
int indiceGlobal;

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
t_num16 stackSize;



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

pthread_mutex_t mutex_Solicitud_Memoria;
pthread_mutex_t mutex_Solicitud_FS;

bool flag_logCancer;

void mostrarArchivoConfig();
void inicializarSemaforosYVariables(char**, char**, char**);

int socket_memoria;
int socket_fs;
fd_set fdsMaestro;

t_log* logKernel;

t_num tamanioPag;

void escucharCPU(int);
void _sumarCantOpPriv(t_pid);

int handshake(int socket_cliente, int tipo);
void atender_consola(int socket_consola);

void consolaKernel();
void terminarKernel();

void finalizarPid(t_pid pid);

#endif /* LIBRERIAKERNEL_H_ */

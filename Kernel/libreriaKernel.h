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
#include <commons/log.h>
#include <commons/process.h>
#include <herramientas/sockets.h>
#include <herramientas/enum.h>



typedef struct {	//todo: reemplazar void* por lo q corresponda
	int socketConsola;
	uint32_t pid;
	uint32_t pc;				//Program Counter
	uint32_t cantPagsCodigo;	//Páginas utilizadas por elcodigo AnSISOP
	t_list* indiceCodigo;		//Lista de t_indiceCodigo
	void* indiceEtiquetas;		// no se
	t_list* indiceStack;		//Lista de t_Stack
	uint32_t ec; 				//Exit Code
}t_PCB;

typedef struct {
	uint32_t offset_inicio;
	uint32_t size;
}t_indiceCodigo;

typedef struct {
	char* nombre;
	void* valor;
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
	uint32_t size;
	bool isFree;
}t_HeapMetadata;

typedef struct {
	t_list* args; //listaArgumentos
	t_list* vars; //listaVariables
	uint32_t retPos;
	uint32_t retVar[3];
}t_Stack;

typedef struct {
	char* id;
	uint32_t posicionMemoria[3];
}t_StackMetadata;


t_list* lista_cpus;
t_list* lista_consolas;
t_list* lista_PCBs;
uint32_t pid;

//Archivo de Configuracion
int puertoPrograma;
int puertoCPU;
char* ipMemoria;
int puertoMemoria;
char* ipFileSystem;
int puertoFS;
int quantum;
int quantumSleep;
char* algoritmo;
int gradoMultiprogramacion;
//t_list* identificadoresSemaforos;
//t_list* inicializacionSemaforos;
//t_list* identificadorVariables;
int stackSize;

void mostrarArchivoConfig();
void inicializarSemaforosYVariables(char**, char**, char**);

int socket_memoria;
int socket_fs;
fd_set fdsMaestro;

t_log* logKernel;

uint32_t tamanioPag;

void escucharCPU(int);

int handshake(int socket_cliente, int tipo);
void atender_consola(int socket_consola);

//PCB
int crearPCB(int);
void borrarPCB(int);
void setearExitCode(int, int);



int enviarScriptAMemoria(uint32_t, char*);
void consolaKernel();
void terminarKernel();

#endif /* LIBRERIAKERNEL_H_ */

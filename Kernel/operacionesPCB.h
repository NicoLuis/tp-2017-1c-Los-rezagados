/*
 * operacionesPCB.h
 *
 *  Created on: 15/5/2017
 *      Author: utnso
 */

#ifndef OPERACIONESPCB_H_
#define OPERACIONESPCB_H_

#include "libreriaKernel.h"

typedef struct {
	t_num size;
	void* bloqueSerializado;
}t_indice;

typedef struct {
	int socketConsola;
	t_num pid;
	t_num pc;				//Program Counter
	t_num cantPagsCodigo;	//Páginas utilizadas por elcodigo AnSISOP
	t_indice indiceCodigo;
	t_indice indiceEtiquetas;
	t_list* indiceStack;		//Lista de t_Stack
	t_num ec; 				//Exit Code
}t_PCB;


typedef struct {
	t_list* args; 	//lista de t_StackMetadata
	t_list* vars; 	//lista de t_StackMetadata
	t_num retPos;
	t_num retVar[3];
}t_Stack;

typedef struct {
	char* id;
	t_num posicionMemoria[3];
}t_StackMetadata;

int crearPCB(int);
void llenarIndicesPCB(int, char*);
void setearExitCode(int, int);
int tamanioTotalPCB(t_PCB* pcb);
void *serializarPCB(t_PCB* pcb);
t_PCB *desserealizarPCB(void*);
void liberarPCB(t_PCB* pcb);

bool _sacarDeCola(int pid, t_queue* cola);

#endif /* OPERACIONESPCB_H_ */

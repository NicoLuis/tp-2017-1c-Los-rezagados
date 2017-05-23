/*
 * operacionesPCB.h
 *
 *  Created on: 15/5/2017
 *      Author: utnso
 */

#ifndef OPERACIONESPCB_H_
#define OPERACIONESPCB_H_

#include <herramientas/sockets.h>

typedef struct {
	t_num size;
	void* bloqueSerializado;
}t_indice;

typedef struct {
	t_num8 pid;
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
	t_posicion retVar;
}t_Stack;

typedef struct {
	char id;
	t_posicion posicionMemoria;
}t_StackMetadata;



void *serializarArgVar(t_list* lista, int tamanio);
int tamanioIndiceStack(t_list* indiceStack);
void *serializarIndiceStack(t_list* indiceStack, int tamanio);
int tamanioArgVar(t_list* lista);
int tamanioTotalPCB(t_PCB* pcb);
void *serializarPCB(t_PCB* pcb);
t_PCB *desserealizarPCB(void*);
void liberarPCB(t_PCB* pcb);
void desserializarArgVar(void* buffer, int tamanio, t_list* lista);
void desserializarIndiceStack(void* buffer, t_num size, t_list* indiceStack);

#endif /* OPERACIONESPCB_H_ */

/*
 * operacionesPCB.h
 *
 *  Created on: 15/5/2017
 *      Author: utnso
 */

#ifndef OPERACIONESPCB_H_
#define OPERACIONESPCB_H_

#include "sockets.h"

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
	t_num8 retVar[3];
}t_Stack;

typedef struct {
	char id;
	t_num posicionMemoria;
}t_StackMetadata;

int tamanioArgVar(t_list* lista);
int tamanioTotalPCB(t_PCB* pcb);
void *serializarPCB(t_PCB* pcb);
t_PCB *desserealizarPCB(void*);
void liberarPCB(t_PCB* pcb);

#endif /* OPERACIONESPCB_H_ */

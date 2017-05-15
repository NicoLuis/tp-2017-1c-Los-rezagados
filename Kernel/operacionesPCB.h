/*
 * operacionesPCB.h
 *
 *  Created on: 15/5/2017
 *      Author: utnso
 */

#ifndef OPERACIONESPCB_H_
#define OPERACIONESPCB_H_

#include "libreriaKernel.h"


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
	t_list* args; //listaArgumentos
	t_list* vars; //listaVariables
	uint32_t retPos;
	uint32_t retVar[3];
}t_Stack;

typedef struct {
	char* id;
	uint32_t posicionMemoria[3];
}t_StackMetadata;

int crearPCB(int);
void setearExitCode(int, int);
void *serializarPCB(t_PCB* pcb);

bool _sacarDeCola(int pid, t_queue* cola);

#endif /* OPERACIONESPCB_H_ */

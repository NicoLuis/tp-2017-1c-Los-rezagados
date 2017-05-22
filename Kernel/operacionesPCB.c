/*
 * operacionesPCB.c
 *
 *  Created on: 15/5/2017
 *      Author: utnso
 */

#include "libreriaKernel.h"
#include <herramientas/operacionesPCB.h>
#include "operacionesPCB.h"

int crearPCB(int socketConsola){
	t_PCB* pcb = malloc(sizeof(t_PCB));
	pcb->pid = pid;
	pcb->indiceStack = list_create();
	list_add(lista_PCBs, pcb);
	return pcb->pid;
}


void llenarIndicesPCB(int pidPCB, char* codigo){
	bool _buscarPCB(t_PCB* pcb){
		return pcb->pid == pidPCB;
	}

	t_PCB* pcb = list_find(lista_PCBs, (void*) _buscarPCB);
	t_metadata_program* metadata = metadata_desde_literal(codigo);

	pcb->pc = metadata->instruccion_inicio;

	pcb->indiceEtiquetas.size = metadata->etiquetas_size;
	pcb->indiceEtiquetas.bloqueSerializado = metadata->etiquetas;

	pcb->indiceCodigo.size = metadata->instrucciones_size;
	pcb->indiceCodigo.bloqueSerializado = metadata->instrucciones_serializado;

	t_Stack* entradaStack = malloc(sizeof(t_Stack));
	entradaStack->args = list_create();
	entradaStack->vars = list_create();
	list_add(pcb->indiceStack, entradaStack);

	metadata_destruir(metadata);
}


bool _sacarDeCola(int pid, t_queue* cola){
	// todo: semaforear
	int i = 0, tmppid;
	bool estaba = false;
	for(; i < queue_size(cola) ; i++){
		tmppid = (int) queue_pop(cola);
		if( tmppid != pid )
			queue_push(cola, &tmppid);
		else
			estaba = true;
	}
	return estaba;
}

void finalizarPCB(int pidPCB){

	if(!_sacarDeCola(pidPCB, cola_New))
		if( _sacarDeCola(pidPCB, cola_Ready) ||
				_sacarDeCola(pidPCB, cola_Exec) ||
				_sacarDeCola(pidPCB, cola_Block) )
			sem_post(&sem_gradoMp);

}


void setearExitCode(int pidPCB, int exitCode){
	bool _buscarPCB(t_PCB* pcb){
		return pcb->pid == pidPCB;
	}

	t_PCB* pcb = list_find(lista_PCBs, (void*) _buscarPCB);
	pcb->ec = exitCode;
	queue_push(cola_Exit, &pcb->pid);
}

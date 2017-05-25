/*
 * operacionesPCB.c
 *
 *  Created on: 15/5/2017
 *      Author: utnso
 */

#include "operacionesPCBKernel.h"

int crearPCB(int socketConsola){
	t_PCB* pcb = malloc(sizeof(t_PCB));
	pcb->pid = pid;
	pcb->exitCode = -20;
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

	pcb->indiceEtiquetas.bloqueSerializado = malloc(metadata->etiquetas_size);
	memcpy(&pcb->indiceEtiquetas.size, &metadata->etiquetas_size, sizeof(t_size));
	memcpy(pcb->indiceEtiquetas.bloqueSerializado, metadata->etiquetas, metadata->etiquetas_size);

	pcb->indiceCodigo.bloqueSerializado = malloc(metadata->instrucciones_size * sizeof(t_intructions));
	memcpy(&pcb->indiceCodigo.size, &metadata->instrucciones_size, sizeof(t_size));
	memcpy(pcb->indiceCodigo.bloqueSerializado, metadata->instrucciones_serializado, metadata->instrucciones_size * sizeof(t_intructions));

	log_trace(logKernel, "metadata->instrucciones_size %d", metadata->instrucciones_size);

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
	pcb->exitCode = exitCode;
	queue_push(cola_Exit, &pcb->pid);
	liberarPCB(pcb, true);
}

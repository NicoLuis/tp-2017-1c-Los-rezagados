/*
 * operacionesPCB.c
 *
 *  Created on: 15/5/2017
 *      Author: utnso
 */

#include "operacionesPCBKernel.h"

t_num8 crearPCB(int socketConsola){
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


t_num8 _sacarDeCola(t_num8 pid, t_queue* cola, pthread_mutex_t mutex){
	int i = 0;
	t_num8 tmppid, retorno = -1;
	void* aux;
	pthread_mutex_lock(&mutex);
	if(pid == 0){		//si pid == 0 saco el ultimo
		aux = queue_pop(cola);
		memcpy(&retorno, aux, sizeof(t_num8));
		free(aux);
	}
	else
	for(; i < queue_size(cola) ; i++){
		aux = queue_pop(cola);
		memcpy(&tmppid, aux, sizeof(t_num8));
		if( tmppid != pid )
			queue_push(cola, aux);
		else{
			memcpy(&retorno, aux, sizeof(t_num8));
			free(aux);
		}
	}
	pthread_mutex_unlock(&mutex);
	return retorno;
}

void _ponerEnCola(t_num8 pid, t_queue* cola, pthread_mutex_t mutex){
	void* aux = malloc(sizeof(t_num8));
	memcpy(aux, &pid, sizeof(t_num8));
	pthread_mutex_lock(&mutex);
	queue_push(cola, aux);
	pthread_mutex_unlock(&mutex);
}


bool _estaEnCola(t_num8 pid, t_queue* cola, pthread_mutex_t mutex){
	// todo: semaforear
	int i = 0;
	t_num8 tmppid;
	void* aux;
	bool esta = false;
	pthread_mutex_lock(&mutex);
	for(; i < queue_size(cola) ; i++){
		aux = queue_pop(cola);
		memcpy(&tmppid, aux, sizeof(t_num8));
		if( tmppid == pid )
			esta = true;
		queue_push(cola, aux);
	}
	pthread_mutex_unlock(&mutex);
	return esta;
}

void finalizarPCB(int pidPCB){

	if(!_sacarDeCola(pidPCB, cola_New, mutex_New))
		if( _sacarDeCola(pidPCB, cola_Ready, mutex_Ready) ||
				_sacarDeCola(pidPCB, cola_Exec, mutex_Exec) ||
				_sacarDeCola(pidPCB, cola_Block, mutex_Block) )
			sem_post(&sem_gradoMp);

}


void setearExitCode(int pidPCB, int exitCode){
	bool _buscarPCB(t_PCB* pcb){
		return pcb->pid == pidPCB;
	}

	t_PCB* pcb = list_find(lista_PCBs, (void*) _buscarPCB);
	pcb->exitCode = exitCode;
	_ponerEnCola(pcb->pid, cola_Exit, mutex_Exit);
	liberarPCB(pcb, true);
}

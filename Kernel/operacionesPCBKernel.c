/*
 * operacionesPCB.c
 *
 *  Created on: 15/5/2017
 *      Author: utnso
 */

#include "operacionesPCBKernel.h"

t_pid crearPCB(int socketConsola, char* script){

	t_PCB* pcb = malloc(sizeof(t_PCB));
	pcb->pid = pid;
	pcb->exitCode = SIN_ASIGNAR;
	pcb->cantRafagas = 0;
	pcb->cantPagsCodigo = 0;
	pcb->cantPagsStack = stackSize;
	pcb->cantPagsHeap = 0;
	pcb->indiceStack = list_create();

	llenarIndicesPCB(pcb, script);

	_lockLista_PCBs();
	list_add(lista_PCBs, pcb);
	_unlockLista_PCBs();

	return pcb->pid;
}


void llenarIndicesPCB(t_PCB* pcb, char* codigo){

	t_metadata_program* metadata = metadata_desde_literal(codigo);

	pcb->pc = metadata->instruccion_inicio;

	pcb->indiceEtiquetas.bloqueSerializado = malloc(metadata->etiquetas_size);
	memcpy(&pcb->indiceEtiquetas.size, &metadata->etiquetas_size, sizeof(t_size));
	memcpy(pcb->indiceEtiquetas.bloqueSerializado, metadata->etiquetas, metadata->etiquetas_size);

	pcb->indiceCodigo.bloqueSerializado = malloc(metadata->instrucciones_size * sizeof(t_intructions));
	memcpy(&pcb->indiceCodigo.size, &metadata->instrucciones_size, sizeof(t_size));
	memcpy(pcb->indiceCodigo.bloqueSerializado, metadata->instrucciones_serializado, metadata->instrucciones_size * sizeof(t_intructions));

	t_Stack* entradaStack = malloc(sizeof(t_Stack));
	entradaStack->args = list_create();
	entradaStack->vars = list_create();
	list_add(pcb->indiceStack, entradaStack);

	metadata_destruir(metadata);
}


t_pid _sacarDeCola(t_pid pid, t_queue* cola, pthread_mutex_t mutex){
	int i = 0;
	t_pid tmppid, retorno = -1;
	void* aux;
	pthread_mutex_lock(&mutex);
	if(queue_size(cola) > 0){
		if(pid == 0){		//si pid == 0 saco el ultimo
			aux = queue_pop(cola);
			memcpy(&retorno, aux, sizeof(t_pid));
			free(aux);
		}
		else
		for(; i < queue_size(cola) ; i++){
			aux = queue_pop(cola);
			memcpy(&tmppid, aux, sizeof(t_pid));
			if( tmppid != pid )
				queue_push(cola, aux);
			else{
				memcpy(&retorno, aux, sizeof(t_pid));
				free(aux);
			}
		}
	}
	pthread_mutex_unlock(&mutex);
	return retorno;
}

void _ponerEnCola(t_pid pid, t_queue* cola, pthread_mutex_t mutex){
	void* aux = malloc(sizeof(t_pid));
	memcpy(aux, &pid, sizeof(t_pid));
	pthread_mutex_lock(&mutex);
	queue_push(cola, aux);
	pthread_mutex_unlock(&mutex);
}


bool _estaEnCola(t_pid pid, t_queue* cola, pthread_mutex_t mutex){
	// todo: semaforear
	int i = 0;
	t_pid tmppid;
	void* aux;
	bool esta = false;
	pthread_mutex_lock(&mutex);
	for(; i < queue_size(cola) ; i++){
		aux = queue_pop(cola);
		memcpy(&tmppid, aux, sizeof(t_pid));
		if( tmppid == pid )
			esta = true;
		queue_push(cola, aux);
	}
	pthread_mutex_unlock(&mutex);
	return esta;
}

void finalizarPCB(t_pid pidPCB){

	if(!_sacarDeCola(pidPCB, cola_New, mutex_New))
		if( _sacarDeCola(pidPCB, cola_Ready, mutex_Ready) ||
				_sacarDeCola(pidPCB, cola_Exec, mutex_Exec) ||
				_sacarDeCola(pidPCB, cola_Block, mutex_Block) )
			sem_post(&sem_gradoMp);

}


void setearExitCode(t_pid pidPCB, int exitCode){
	//log_trace(logKernel, "Setteo exit code %d a pid %d", exitCode, pidPCB);
	bool _buscarPCB(t_PCB* pcb){
		return pcb->pid == pidPCB;
	}

	_lockLista_PCBs();
	t_PCB* pcb = list_remove_by_condition(lista_PCBs, (void*) _buscarPCB);
	if(pcb == NULL)
		log_warning(logKernel, "No se encuentra pcb %d", pidPCB);
	else{
		if(pcb->exitCode == SIN_ASIGNAR){
			pcb->exitCode = exitCode;
		}
		list_add(lista_PCBs, pcb);
	}
	_unlockLista_PCBs();
}

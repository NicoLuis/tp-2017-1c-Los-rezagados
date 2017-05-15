/*
 * operacionesPCB.c
 *
 *  Created on: 15/5/2017
 *      Author: utnso
 */

#include "operacionesPCB.h"

int crearPCB(int socketConsola){

	t_PCB* pcb = malloc(sizeof(t_PCB));
	pcb->socketConsola = socketConsola;
	pcb->pid = pid;
	pcb->pc = 0;
	pcb->indiceCodigo = list_create();
	pcb->indiceStack = list_create();
	//todo: ver q pija son los indices

	list_add(lista_PCBs, pcb);
	return pcb->pid;
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


int _tamanioIndiceCodigo(t_list* indiceCodigo){
	return list_size(indiceCodigo) * sizeof(uint32_t) * 2;
}

int _tamanioIndiceEtiquetas(t_list* indiceEtiquetas){
	//	todo: implementar
	return 0;
}

int _tamanioIndiceStack(t_list* indiceStack){
	//	todo: implementar
	return 0;
}

void *_serializarIndiceCodigo(t_list* indiceCodigo, int tamanio){
	int offset = 0, tmpsize, i = 0;
	void *buffer = malloc(tamanio);

	for(; i < list_size(indiceCodigo) ; i++){
		t_indiceCodigo *aux = list_get(indiceCodigo, i);
		memcpy(buffer + offset, &aux->offset_inicio, tmpsize = sizeof(uint32_t));
		offset += tmpsize;
		memcpy(buffer + offset, &aux->size, tmpsize = sizeof(uint32_t));
		offset += tmpsize;
		free(aux);
	}

	return buffer;
}

void *_serializarIndiceEtiquetas(t_list* indiceEtiquetas, int tamanio){
	//int offset = 0, tmpsize;
	void *buffer = malloc(tamanio);

	//	todo: implementar

	return buffer;
}



void *_serializarIndiceStack(t_list* indiceStack, int tamanio){
	int offset = 0, tmpsize, i = 0;
	void *buffer = malloc(tamanio);

	for(; i < list_size(indiceStack) ; i++){
		t_Stack *aux = list_get(indiceStack, i);
		memcpy(buffer + offset, &aux->retPos, tmpsize = sizeof(uint32_t));
		offset += tmpsize;
		memcpy(buffer + offset, &aux->retVar[0], tmpsize = sizeof(uint32_t));
		offset += tmpsize;
		memcpy(buffer + offset, &aux->retVar[1], tmpsize = sizeof(uint32_t));
		offset += tmpsize;
		memcpy(buffer + offset, &aux->retVar[2], tmpsize = sizeof(uint32_t));
		offset += tmpsize;

		// todo: terminnar de serializar listas args y vars

		free(aux);
	}

	return buffer;
}





void *serializarPCB(t_PCB* pcb){
	int offset = 0;
	uint32_t tmpsize;
	void *buffer = malloc(sizeof(t_PCB)), *tmpBuffer;

	memcpy(buffer + offset, &pcb->pid, tmpsize = sizeof(uint32_t));
		offset += tmpsize;
	memcpy(buffer + offset, &pcb->pc, tmpsize = sizeof(uint32_t));
		offset += tmpsize;
	memcpy(buffer + offset, &pcb->cantPagsCodigo, tmpsize = sizeof(uint32_t));
		offset += tmpsize;
	memcpy(buffer + offset, &pcb->ec, tmpsize = sizeof(uint32_t));
		offset += tmpsize;

	tmpsize = _tamanioIndiceCodigo(pcb->indiceCodigo);
	tmpBuffer = _serializarIndiceCodigo(pcb->indiceCodigo, tmpsize);
	memcpy(buffer + offset, &tmpsize, sizeof(uint32_t));
	memcpy(buffer + offset, tmpBuffer, tmpsize);
	offset += tmpsize;
	free(tmpBuffer);

	tmpsize = _tamanioIndiceEtiquetas(pcb->indiceEtiquetas);
	tmpBuffer = _serializarIndiceEtiquetas(pcb->indiceEtiquetas, tmpsize);
	memcpy(buffer + offset, &tmpsize, sizeof(uint32_t));
	memcpy(buffer + offset, tmpBuffer, tmpsize);
	offset += tmpsize;
	free(tmpBuffer);

	tmpsize = _tamanioIndiceStack(pcb->indiceStack);
	tmpBuffer = _serializarIndiceStack(pcb->indiceStack, tmpsize);
	memcpy(buffer + offset, &tmpsize, sizeof(uint32_t));
	memcpy(buffer + offset, tmpBuffer, tmpsize);
	offset += tmpsize;
	free(tmpBuffer);

	return buffer;
}

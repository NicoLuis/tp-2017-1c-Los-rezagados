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
	pcb->indiceStack = list_create();
	//todo: ver q pija son los indices

	list_add(lista_PCBs, pcb);
	return pcb->pid;
}


void llenarCargarIndicesPCB(int pidPCB, char* codigo){
	bool _buscarPCB(t_PCB* pcb){
		return pcb->pid == pidPCB;
	}

	t_PCB* pcb = list_find(lista_PCBs, (void*) _buscarPCB);
	t_metadata_program* metadata = metadata_desde_literal(codigo);

	pcb->pc = metadata->instruccion_inicio;

	pcb->indiceEtiquetas.size = metadata->etiquetas_size;
	pcb->indiceEtiquetas.serializado = metadata->etiquetas;

	pcb->indiceCodigo.size = metadata->instrucciones_size;
	pcb->indiceCodigo.serializado = metadata->instrucciones_serializado;

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









void *_serializarIndiceStack(t_list* indiceStack, int tamanio){
	int offset = 0, tmpsize, i = 0;
	void *buffer = malloc(tamanio);

	for(; i < list_size(indiceStack) ; i++){
		t_Stack *aux = list_get(indiceStack, i);
		memcpy(buffer + offset, &aux->retPos, tmpsize = sizeof(t_num));
		offset += tmpsize;
		memcpy(buffer + offset, &aux->retVar[0], tmpsize = sizeof(t_num));
		offset += tmpsize;
		memcpy(buffer + offset, &aux->retVar[1], tmpsize = sizeof(t_num));
		offset += tmpsize;
		memcpy(buffer + offset, &aux->retVar[2], tmpsize = sizeof(t_num));
		offset += tmpsize;

		// todo: terminnar de serializar listas args y vars

		free(aux);
	}

	return buffer;
}


int _tamanioIndiceStack(t_list* indiceStack){
	//	todo: implementar
	return 0;
}

int tamanioTotalPCB(t_PCB* pcb){

	int tamanioIndiceCodigo =
			sizeof(t_size) + pcb->indiceCodigo.size * sizeof(t_intructions);
	int tamanioIndiceEtiquetas =
			sizeof(t_size) + pcb->indiceEtiquetas.size;

	return sizeof(t_num) * 4 +
		tamanioIndiceCodigo + tamanioIndiceEtiquetas + _tamanioIndiceStack(pcb->indiceStack);
}

void *serializarPCB(t_PCB* pcb){
	int offset = 0;
	t_num tmpsize;
	void *buffer = malloc(sizeof(t_PCB)), *tmpBuffer;

	memcpy(buffer + offset, &pcb->pid, tmpsize = sizeof(t_num));
		offset += tmpsize;
	memcpy(buffer + offset, &pcb->pc, tmpsize = sizeof(t_num));
		offset += tmpsize;
	memcpy(buffer + offset, &pcb->cantPagsCodigo, tmpsize = sizeof(t_num));
		offset += tmpsize;
	memcpy(buffer + offset, &pcb->ec, tmpsize = sizeof(t_num));
		offset += tmpsize;

	memcpy(buffer + offset, &pcb->indiceCodigo.size, tmpsize = sizeof(t_size));
		offset += tmpsize;
	memcpy(buffer + offset, &pcb->indiceCodigo.serializado, tmpsize = pcb->indiceCodigo.size * sizeof(t_intructions));
		offset += tmpsize;

	memcpy(buffer + offset, &pcb->indiceEtiquetas.size, tmpsize = sizeof(t_size));
		offset += tmpsize;
	memcpy(buffer + offset, &pcb->indiceEtiquetas.serializado, tmpsize = pcb->indiceEtiquetas.size);
		offset += tmpsize;

	tmpsize = _tamanioIndiceStack(pcb->indiceStack);
	tmpBuffer = _serializarIndiceStack(pcb->indiceStack, tmpsize);
	memcpy(buffer + offset, &tmpsize, sizeof(t_num));
	memcpy(buffer + offset, tmpBuffer, tmpsize);
	offset += tmpsize;
	free(tmpBuffer);

	return buffer;
}


t_list* _desserializarIndiceStack(void* buffer, t_num size){
	t_list *lista = list_create();

	return lista;
}


t_PCB *desserealizarPCB(void* buffer){

	t_PCB* pcb = malloc(sizeof(t_PCB));

	int offset = 0;
	t_num tmpsize;
	void *tmpBuffer;

	memcpy(&pcb->pid, buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;
	memcpy(&pcb->pc, buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;
	memcpy(&pcb->cantPagsCodigo, buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;
	memcpy(&pcb->ec, buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;

	memcpy(&pcb->indiceCodigo.size, buffer + offset, tmpsize = sizeof(t_size));
		offset += tmpsize;
	memcpy(&pcb->indiceCodigo.serializado, buffer + offset, tmpsize = pcb->indiceCodigo.size * sizeof(t_intructions));
		offset += tmpsize;

	memcpy(&pcb->indiceEtiquetas.size, buffer + offset, tmpsize = sizeof(t_size));
		offset += tmpsize;
	memcpy(&pcb->indiceEtiquetas.serializado, buffer + offset, tmpsize = pcb->indiceEtiquetas.size * sizeof(t_intructions));
		offset += tmpsize;

	memcpy(&tmpsize, buffer + offset, sizeof(t_num));
	tmpBuffer = malloc(tmpsize);
	memcpy(tmpBuffer, buffer + offset, tmpsize);
	pcb->indiceStack = _desserializarIndiceStack(tmpBuffer, tmpsize);
	offset += tmpsize;
	free(tmpBuffer);

	return pcb;
}

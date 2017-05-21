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



int _tamanioArgVar(t_list* lista){
	int i = 0, sizeTotal = 0;
	for(; i < list_size(lista) ; i++){
		t_StackMetadata *aux = list_get(lista, i);
		sizeTotal += sizeof(t_num)*4 + string_length(aux->id);
	}
	return sizeTotal;
}

void *_serializarArgVar(t_list* lista, int tamanio){
	int offset = 0, tmpsize, i = 0;
	void *buffer = malloc(tamanio);

	for(; i < list_size(lista) ; i++){
		t_StackMetadata *aux = list_get(lista, i);
		tmpsize = string_length(aux->id);
		memcpy(buffer + offset, &tmpsize, sizeof(t_num));
		offset += sizeof(t_num);
		memcpy(buffer + offset, aux->id, tmpsize);
		offset += tmpsize;
		memcpy(buffer + offset, &aux->posicionMemoria[0], tmpsize = sizeof(t_num));
		offset += tmpsize;
		memcpy(buffer + offset, &aux->posicionMemoria[1], tmpsize = sizeof(t_num));
		offset += tmpsize;
		memcpy(buffer + offset, &aux->posicionMemoria[2], tmpsize = sizeof(t_num));
		offset += tmpsize;
	}

	return buffer;
}

int _tamanioIndiceStack(t_list* indiceStack){
	int i = 0, sizeTotal = 0;
	for(; i < list_size(indiceStack) ; i++){
		t_Stack *aux = list_get(indiceStack, i);
		sizeTotal += sizeof(t_num)*6;
		sizeTotal += _tamanioArgVar(aux->args);
		sizeTotal += _tamanioArgVar(aux->vars);
	}
	return sizeTotal;
}

void *_serializarIndiceStack(t_list* indiceStack, int tamanio){
	int offset = 0, tmpsize, i = 0;
	void *buffer = malloc(tamanio), *tmpBuffer;

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

		tmpsize = _tamanioArgVar(aux->args);
		tmpBuffer = _serializarArgVar(aux->args, tmpsize);
		memcpy(buffer + offset, &tmpsize, sizeof(t_num));
		offset += sizeof(t_num);
		memcpy(buffer + offset, tmpBuffer, tmpsize);
		offset += tmpsize;
		free(tmpBuffer);

		tmpsize = _tamanioArgVar(aux->vars);
		tmpBuffer = _serializarArgVar(aux->vars, tmpsize);
		memcpy(buffer + offset, &tmpsize, sizeof(t_num));
		offset += sizeof(t_num);
		memcpy(buffer + offset, tmpBuffer, tmpsize);
		offset += tmpsize;
		free(tmpBuffer);
	}

	return buffer;
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
	memcpy(buffer + offset, &pcb->indiceCodigo.bloqueSerializado, tmpsize = pcb->indiceCodigo.size * sizeof(t_intructions));
		offset += tmpsize;

	memcpy(buffer + offset, &pcb->indiceEtiquetas.size, tmpsize = sizeof(t_size));
		offset += tmpsize;
	memcpy(buffer + offset, &pcb->indiceEtiquetas.bloqueSerializado, tmpsize = pcb->indiceEtiquetas.size);
		offset += tmpsize;

	tmpsize = _tamanioIndiceStack(pcb->indiceStack);
	tmpBuffer = _serializarIndiceStack(pcb->indiceStack, tmpsize);
	memcpy(buffer + offset, &tmpsize, sizeof(t_num));
	offset += sizeof(t_num);
	memcpy(buffer + offset, tmpBuffer, tmpsize);
	offset += tmpsize;
	free(tmpBuffer);

	return buffer;
}

void _desserializarArgVar(void* buffer, int tamanio, t_list* lista){
	int offset = 0, tmpsize;

	while(offset < tamanio){
		t_StackMetadata *aux = malloc(sizeof(t_StackMetadata));
		memcpy(&tmpsize, buffer + offset, tmpsize = sizeof(t_num));
		offset += sizeof(t_num);
		memcpy(buffer + offset, aux->id, tmpsize);
		offset += tmpsize;
		memcpy(&aux->posicionMemoria[0], buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;
		memcpy(&aux->posicionMemoria[1], buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;
		memcpy(&aux->posicionMemoria[2], buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;
	}
}

void _desserializarIndiceStack(void* buffer, t_num size, t_list* indiceStack){
	indiceStack = list_create();

	int offset = 0, tmpsize;
	void *tmpBuffer;

	while(offset < size){

		t_Stack *aux = malloc(sizeof(t_Stack));

		memcpy(&aux->retPos, buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;
		memcpy(&aux->retVar[0], buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;
		memcpy(&aux->retVar[1], buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;
		memcpy(&aux->retVar[2], buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;

		memcpy(&tmpsize, buffer + offset, sizeof(t_num));
		offset += sizeof(t_num);
		tmpBuffer = malloc(tmpsize);
		memcpy(tmpBuffer, buffer + offset, tmpsize);
		offset += tmpsize;
		_desserializarArgVar(tmpBuffer, tmpsize, aux->args);
		free(tmpBuffer);

		memcpy(&tmpsize, buffer + offset, sizeof(t_num));
		offset += sizeof(t_num);
		tmpBuffer = malloc(tmpsize);
		memcpy(tmpBuffer, buffer + offset, tmpsize);
		offset += tmpsize;
		_desserializarArgVar(tmpBuffer, tmpsize, aux->vars);
		free(tmpBuffer);

		list_add(indiceStack, aux);
	}

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
	memcpy(&pcb->indiceCodigo.bloqueSerializado, buffer + offset, tmpsize = pcb->indiceCodigo.size * sizeof(t_intructions));
		offset += tmpsize;

	memcpy(&pcb->indiceEtiquetas.size, buffer + offset, tmpsize = sizeof(t_size));
		offset += tmpsize;
	memcpy(&pcb->indiceEtiquetas.bloqueSerializado, buffer + offset, tmpsize = pcb->indiceEtiquetas.size * sizeof(t_intructions));
		offset += tmpsize;

	memcpy(&tmpsize, buffer + offset, sizeof(t_num));
	offset += sizeof(t_num);
	tmpBuffer = malloc(tmpsize);
	memcpy(tmpBuffer, buffer + offset, tmpsize);
	_desserializarIndiceStack(tmpBuffer, tmpsize, pcb->indiceStack);
	offset += tmpsize;
	free(tmpBuffer);

	return pcb;
}


void liberarPCB(t_PCB* pcb){
	void _destruirStackMetadata(t_StackMetadata* stack){
		free(stack);
	}
	void _destruirIndiceStack(t_Stack* stack){
		list_destroy_and_destroy_elements(stack->args, (void*) _destruirStackMetadata);
		list_destroy_and_destroy_elements(stack->vars, (void*) _destruirStackMetadata);
		free(stack);
	}
	free(pcb->indiceCodigo.bloqueSerializado);
	free(pcb->indiceEtiquetas.bloqueSerializado);
	list_destroy_and_destroy_elements(pcb->indiceStack, (void*) _destruirIndiceStack);
	free(pcb);

}


#include "pcb.h"
#include "libreriaKernel.h"

int tamanioArgVar(t_list* lista){
	return (sizeof(t_num)*3 + sizeof(char)) * list_size(lista);
}

void *serializarArgVar(t_list* lista, int tamanio){
	int offset = 0, tmpsize, i = 0;
	void *buffer = malloc(tamanio);

	for(; i < list_size(lista) ; i++){
		t_StackMetadata *aux = list_get(lista, i);
		memcpy(buffer + offset, &aux->id, tmpsize = sizeof(char));
		offset += tmpsize;
		memcpy(buffer + offset, &aux->posicionMemoria.pagina, tmpsize = sizeof(t_num));
		offset += tmpsize;
		memcpy(buffer + offset, &aux->posicionMemoria.offset, tmpsize = sizeof(t_num));
		offset += tmpsize;
		memcpy(buffer + offset, &aux->posicionMemoria.size, tmpsize = sizeof(t_num));
		offset += tmpsize;
	}

	return buffer;
}

int tamanioIndiceStack(t_list* indiceStack){
	int i = 0, sizeTotal = 0;
	for(; i < list_size(indiceStack) ; i++){
		t_Stack *aux = list_get(indiceStack, i);
		sizeTotal += sizeof(t_num)*6;
		sizeTotal += tamanioArgVar(aux->args);
		sizeTotal += tamanioArgVar(aux->vars);
	}
	return sizeTotal;
}

void *serializarIndiceStack(t_list* indiceStack, int tamanio){
	int offset = 0, tmpsize, i = 0;
	void *buffer = malloc(tamanio), *tmpBuffer;

	for(; i < list_size(indiceStack) ; i++){
		t_Stack *aux = list_get(indiceStack, i);
		memcpy(buffer + offset, &aux->retPos, tmpsize = sizeof(t_num));
		offset += tmpsize;
		memcpy(buffer + offset, &aux->retVar.pagina, tmpsize = sizeof(t_num));
		offset += tmpsize;
		memcpy(buffer + offset, &aux->retVar.offset, tmpsize = sizeof(t_num));
		offset += tmpsize;
		memcpy(buffer + offset, &aux->retVar.size, tmpsize = sizeof(t_num));
		offset += tmpsize;

		tmpsize = tamanioArgVar(aux->args);
		tmpBuffer = serializarArgVar(aux->args, tmpsize);
		memcpy(buffer + offset, &tmpsize, sizeof(t_num));
		offset += sizeof(t_num);
		memcpy(buffer + offset, tmpBuffer, tmpsize);
		offset += tmpsize;
		free(tmpBuffer);

		tmpsize = tamanioArgVar(aux->vars);
		tmpBuffer = serializarArgVar(aux->vars, tmpsize);
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

	return sizeof(t_num8) + sizeof(t_num) * 6 +
		tamanioIndiceCodigo + tamanioIndiceEtiquetas + tamanioIndiceStack(pcb->indiceStack);
}

void *serializarPCB(t_PCB* pcb){
	int offset = 0;
	t_num tmpsize;
	void *buffer = malloc(tamanioTotalPCB(pcb)), *tmpBuffer;

	memcpy(buffer + offset, &pcb->pid, tmpsize = sizeof(t_num8));
		offset += tmpsize;
	memcpy(buffer + offset, &pcb->pc, tmpsize = sizeof(t_num));
		offset += tmpsize;
	memcpy(buffer + offset, &pcb->cantPagsCodigo, tmpsize = sizeof(t_num));
		offset += tmpsize;
	memcpy(buffer + offset, &pcb->cantPagsStack, tmpsize = sizeof(t_num));
		offset += tmpsize;
	memcpy(buffer + offset, &pcb->exitCode, tmpsize = sizeof(t_num));
		offset += tmpsize;
	memcpy(buffer + offset, &pcb->sp, tmpsize = sizeof(t_num));
		offset += tmpsize;

	memcpy(buffer + offset, &pcb->indiceCodigo.size, tmpsize = sizeof(t_size));
		offset += tmpsize;
	memcpy(buffer + offset, pcb->indiceCodigo.bloqueSerializado, tmpsize = pcb->indiceCodigo.size * sizeof(t_intructions));
		offset += tmpsize;
	memcpy(buffer + offset, &pcb->indiceEtiquetas.size, tmpsize = sizeof(t_size));
		offset += tmpsize;
	memcpy(buffer + offset, pcb->indiceEtiquetas.bloqueSerializado, tmpsize = pcb->indiceEtiquetas.size);
		offset += tmpsize;

	tmpsize = tamanioIndiceStack(pcb->indiceStack);
	tmpBuffer = serializarIndiceStack(pcb->indiceStack, tmpsize);
	memcpy(buffer + offset, &tmpsize, sizeof(t_num));
	offset += sizeof(t_num);
	memcpy(buffer + offset, tmpBuffer, tmpsize);
	offset += tmpsize;
	free(tmpBuffer);

	return buffer;
}

t_list* desserializarArgVar(void* buffer, int tamanio){
	t_list* lista = list_create();
	int offset = 0, tmpsize;

	while(offset < tamanio){
		t_StackMetadata *aux = malloc(sizeof(t_StackMetadata));
		memcpy(&aux->id, buffer + offset, tmpsize = sizeof(char));
		offset += tmpsize;
		memcpy(&aux->posicionMemoria.pagina, buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;
		memcpy(&aux->posicionMemoria.offset, buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;
		memcpy(&aux->posicionMemoria.size, buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;
		list_add(lista, aux);
	}
	return lista;
}

t_list* desserializarIndiceStack(void* buffer, t_num size){
	t_list* indiceStack = list_create();
	int offset = 0, tmpsize;
	void *tmpBuffer;

	while(offset < size){

		t_Stack *aux = malloc(sizeof(t_Stack));

		memcpy(&aux->retPos, buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;
		memcpy(&aux->retVar.pagina, buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;
		memcpy(&aux->retVar.offset, buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;
		memcpy(&aux->retVar.size, buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;

		memcpy(&tmpsize, buffer + offset, sizeof(t_num));
		offset += sizeof(t_num);
		tmpBuffer = malloc(tmpsize);
		memcpy(tmpBuffer, buffer + offset, tmpsize);
		offset += tmpsize;
		aux->args = desserializarArgVar(tmpBuffer, tmpsize);
		free(tmpBuffer);

		memcpy(&tmpsize, buffer + offset, sizeof(t_num));
		offset += sizeof(t_num);
		tmpBuffer = malloc(tmpsize);
		memcpy(tmpBuffer, buffer + offset, tmpsize);
		offset += tmpsize;
		aux->vars = desserializarArgVar(tmpBuffer, tmpsize);
		free(tmpBuffer);

		list_add(indiceStack, aux);
	}
	return indiceStack;
}


t_PCB *desserealizarPCB(void* buffer){

	t_PCB* pcb = malloc(sizeof(t_PCB));

	int offset = 0;
	t_num tmpsize;
	void *tmpBuffer;

	memcpy(&pcb->pid, buffer + offset, tmpsize = sizeof(t_num8));
		offset += tmpsize;
	memcpy(&pcb->pc, buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;
	memcpy(&pcb->cantPagsCodigo, buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;
	memcpy(&pcb->cantPagsStack, buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;
	memcpy(&pcb->exitCode, buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;
	memcpy(&pcb->sp, buffer + offset, tmpsize = sizeof(t_num));
		offset += tmpsize;

	memcpy(&pcb->indiceCodigo.size, buffer + offset, tmpsize = sizeof(t_size));
		offset += tmpsize;
	pcb->indiceCodigo.bloqueSerializado = malloc(pcb->indiceCodigo.size * sizeof(t_intructions));
	memcpy(pcb->indiceCodigo.bloqueSerializado, buffer + offset, tmpsize = pcb->indiceCodigo.size * sizeof(t_intructions));
		offset += tmpsize;

	memcpy(&pcb->indiceEtiquetas.size, buffer + offset, tmpsize = sizeof(t_size));
		offset += tmpsize;
	pcb->indiceEtiquetas.bloqueSerializado = malloc(pcb->indiceEtiquetas.size);
	memcpy(pcb->indiceEtiquetas.bloqueSerializado, buffer + offset, tmpsize = pcb->indiceEtiquetas.size);
		offset += tmpsize;

	memcpy(&tmpsize, buffer + offset, sizeof(t_num));
	offset += sizeof(t_num);
	tmpBuffer = malloc(tmpsize);
	memcpy(tmpBuffer, buffer + offset, tmpsize);
	pcb->indiceStack = desserializarIndiceStack(tmpBuffer, tmpsize);
	offset += tmpsize;
	free(tmpBuffer);

	return pcb;
}


void liberarPCB(t_PCB* pcb, bool soloStructs){

	void _destruirIndiceStack(t_Stack* stack){
		list_destroy_and_destroy_elements(stack->args, free);
		list_destroy_and_destroy_elements(stack->vars, free);
		free(stack);
	}
	free(pcb->indiceCodigo.bloqueSerializado);
	free(pcb->indiceEtiquetas.bloqueSerializado);
	list_destroy_and_destroy_elements(pcb->indiceStack, (void*) _destruirIndiceStack);
	if(!soloStructs)
		free(pcb);
}

/*
 * planificacion.c
 *
 *  Created on: 15/5/2017
 *      Author: utnso
 */


#include "planificacion.h"

#include "operacionesPCBKernel.h"

void planificar(){

	while(1){
		sem_wait(&sem_cantColaReady);

		if( string_equals_ignore_case(algoritmo, "FIFO") )
			planificar_FIFO();
		else if( string_equals_ignore_case(algoritmo, "RR") )
			planificar_RR();
		else{
			log_error(logKernel, "Solo funco con FIFO o RR");
			exit(1);
		}
	}

}


void planificar_RR(){

	usleep(quantumSleep * 1000);
	sem_post(&sem_cantColaReady);		//una vez q entre xq wait me resto 1 sin cambiar el tamaniodeCola
	int quantumRestante = quantum, cpuUsada;

	log_trace(logKernel, "Inicio rafaga RR con quantum: %d", quantum);
	int pidPCB = (int) queue_pop(cola_Ready);
	int _es_PCB(t_PCB* p){
		return p->pid == pidPCB;
	}

	t_PCB* pcb = list_find(lista_PCBs, (void*) _es_PCB);
	if(pcb == NULL)
		log_error(logKernel, "no existe el proceso con pid %d", pidPCB);

	cpuUsada = (int) list_take_and_remove(lista_cpus, 1);

	uint32_t size = tamanioTotalPCB(pcb);
	void* pcbSerializado = serializarPCB(pcb);
	msg_enviar_separado(ENVIO_PCB, size, pcbSerializado, cpuUsada);
	free(pcbSerializado);

	log_trace(logKernel, "Planifico proceso %d", pcb->pid);
	while(quantumRestante > 0){

		if(quantumRestante > 1)
			msg_enviar_separado(EJECUTAR_INSTRUCCION, 1, 0, cpuUsada);
		if(quantumRestante == 1)
			msg_enviar_separado(EJECUTAR_ULTIMA_INSTRUCCION, 1, 0, cpuUsada);
		t_msg* msgRecibido = msg_recibir(cpuUsada);
		switch(msgRecibido->tipoMensaje){
		case OK:
			quantumRestante--;
			break;

		case ENVIO_PCB:
			msg_recibir_data(cpuUsada, msgRecibido);
			pcb = desserealizarPCB(msgRecibido);
			list_add(lista_cpus, &cpuUsada);
			break;
		case 0:
			log_trace(logKernel, "Se desconecto cpu %d", cpuUsada);
			setearExitCode(pcb->pid, -20);
			quantumRestante = 0;
		}
	}

	log_trace(logKernel, "Fin rafaga RR");
}




void planificar_FIFO(){

	sem_post(&sem_cantColaReady);		//una vez q entre xq wait me resto 1 sin cambiar el tamaniodeCola

	log_trace(logKernel, "Inicio FIFO");
	int pidPCB;
	memcpy(&pidPCB, queue_peek(cola_Ready), sizeof(int));
	int _es_PCB(t_PCB* p){
		return p->pid == pidPCB;
	}

	t_PCB* pcb = list_find(lista_PCBs, (void*) _es_PCB);
	if(pcb == NULL)
		log_error(logKernel, "no existe el proceso con pid %d", pidPCB);

	int cpuUsada;
	memcpy(&cpuUsada,  list_remove(lista_cpus, 0), sizeof(int));
	log_trace(logKernel, "cpuUsada %d", cpuUsada);

	sigoFIFO = 1;
	t_infosocket* info = malloc(sizeof(t_infosocket));
	info->pid = pcb->pid; info->socket = cpuUsada;
	list_add(lista_PCB_cpu, info);

	bool _esCpu(t_infosocket* a){
		return a->socket == cpuUsada;
	}
	void _lib(t_infosocket* a){
		log_trace(logKernel, "FREE 7");
		free(a);
	}

	uint32_t size = tamanioTotalPCB(pcb);
	void* pcbSerializado = serializarPCB(pcb);
	log_trace(logKernel, "good 7");
	msg_enviar_separado(ENVIO_PCB, size, pcbSerializado, cpuUsada);
	log_trace(logKernel, "good 7");

	while(sigoFIFO){

		log_trace(logKernel, "Planifico proceso %d", pcb->pid);

		msg_enviar_separado(EJECUTAR_INSTRUCCION, 1, 0, cpuUsada);

		t_msg* msgRecibido = msg_recibir(cpuUsada);

		switch(msgRecibido->tipoMensaje){
		case OK:
			log_trace(logKernel, "Recibi OK");
			break;

		case ENVIO_PCB:		// si me devuelve el PCB es porque fue la ultima instruccion
			log_trace(logKernel, "Recibi PCB");
			msg_recibir_data(cpuUsada, msgRecibido);
			pcb = desserealizarPCB(msgRecibido);
			list_add(lista_cpus, &cpuUsada);
			list_remove_and_destroy_by_condition(lista_PCB_cpu, (void*) _esCpu, (void*) _lib);
			queue_pop(cola_Ready);
			sigoFIFO = 0;
			setearExitCode(pcb->pid, 0);

			break;
		case 0:
			log_trace(logKernel, "Se desconecto cpu %d", cpuUsada);
			list_remove_and_destroy_by_condition(lista_PCB_cpu, (void*) _esCpu, (void*) _lib);
			setearExitCode(pcb->pid, -20);
			queue_pop(cola_Ready);
			sigoFIFO = 0;
			break;
		}

	}

	free(pcbSerializado);
	log_trace(logKernel, "Fin FIFO");

}


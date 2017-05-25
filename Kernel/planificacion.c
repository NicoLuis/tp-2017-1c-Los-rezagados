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

	pthread_mutex_lock(&mut_planificacion);
	usleep(quantumSleep * 1000);
	sem_post(&sem_cantColaReady);		//una vez q entre xq wait me resto 1 sin cambiar el tamaniodeCola
	quantumRestante = quantum;

	log_trace(logKernel, "Inicio rafaga RR con quantum: %d", quantum);
	int pidPCB = (int) queue_pop(cola_Ready);
	int _es_PCB(t_PCB* p){
		return p->pid == pidPCB;
	}

	t_PCB* pcb = list_find(lista_PCBs, (void*) _es_PCB);
	if(pcb == NULL)
		log_error(logKernel, "no existe el proceso con pid %d", pidPCB);

	t_cpu* cpuUsada = list_remove(lista_cpus, 0);
	log_trace(logKernel, "cpuUsada %d", cpuUsada->socket);

	uint32_t size = tamanioTotalPCB(pcb);
	void* pcbSerializado = serializarPCB(pcb);
	msg_enviar_separado(ENVIO_PCB, size, pcbSerializado, cpuUsada->socket);
	free(pcbSerializado);

	log_trace(logKernel, "Planifico proceso %d", pcb->pid);
	while(quantumRestante > 0){

		if(quantumRestante > 1)
			msg_enviar_separado(EJECUTAR_INSTRUCCION, 1, 0, cpuUsada->socket);
		if(quantumRestante == 1)
			msg_enviar_separado(EJECUTAR_ULTIMA_INSTRUCCION, 1, 0, cpuUsada->socket);

		sem_wait(&cpuUsada->sem);
		pthread_mutex_lock(&mut_planificacion);

		log_trace(logKernel, "Planifico correctamente");
	}

	log_trace(logKernel, "Fin rafaga RR");
	list_add(lista_cpus, cpuUsada);
}




void planificar_FIFO(){

	sem_post(&sem_cantColaReady);		//una vez q entre xq wait me resto 1 sin cambiar el tamaniodeCola
	pthread_mutex_lock(&mut_planificacion);

	log_trace(logKernel, "Inicio FIFO");
	int pidPCB;
	memcpy(&pidPCB, queue_peek(cola_Ready), sizeof(int));
	int _es_PCB(t_PCB* p){
		return p->pid == pidPCB;
	}

	t_PCB* pcb = list_find(lista_PCBs, (void*) _es_PCB);
	if(pcb == NULL)
		log_error(logKernel, "no existe el proceso con pid %d", pidPCB);

	t_cpu* cpuUsada = list_remove(lista_cpus, 0);
	log_trace(logKernel, "cpuUsada %d", cpuUsada->socket);

	sigoFIFO = 1;
	t_infosocket* info = malloc(sizeof(t_infosocket));
	info->pid = pcb->pid;
	info->socket = cpuUsada->socket;
	list_add(lista_PCB_cpu, info);

	bool _esCpu(t_infosocket* a){
		return a->socket == cpuUsada->socket;
	}
	void _liberar(t_infosocket* a){
		free(a);
	}

	uint32_t size = tamanioTotalPCB(pcb);
	void* pcbSerializado = serializarPCB(pcb);
	msg_enviar_separado(ENVIO_PCB, size, pcbSerializado, cpuUsada->socket);

	while(sigoFIFO){

		log_trace(logKernel, "Planifico proceso %d en cpu %d", pcb->pid, cpuUsada->socket);

		msg_enviar_separado(EJECUTAR_INSTRUCCION, 0, 0, cpuUsada->socket);

		sem_post(&cpuUsada->sem);
		pthread_mutex_lock(&mut_planificacion);

		log_trace(logKernel, "Planifico correctamente");
	}

	free(pcbSerializado);
	log_trace(logKernel, "Fin FIFO");

}


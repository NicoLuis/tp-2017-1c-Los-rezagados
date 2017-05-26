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
	int pidPCB;
	sem_wait(&sem_cantColaReady);
	memcpy(&pidPCB, queue_pop(cola_Ready), sizeof(t_num8));
	int _es_PCB(t_PCB* p){
		return p->pid == pidPCB;
	}
	queue_push(cola_Exec, &pidPCB);

	t_PCB* pcb = list_find(lista_PCBs, (void*) _es_PCB);
	if(pcb == NULL)
		log_error(logKernel, "no existe el proceso con pid %d", pidPCB);

	t_cpu* cpuUsada = list_remove(lista_cpus, 0);
	log_trace(logKernel, "cpuUsada %d", cpuUsada->socket);

	uint32_t size = tamanioTotalPCB(pcb);
	void* pcbSerializado = serializarPCB(pcb);
	msg_enviar_separado(ENVIO_PCB, size, pcbSerializado, cpuUsada->socket);
	free(pcbSerializado);

	int _proc(t_infoProceso* a){
		return a->pid == pcb->pid;
	}
	t_infoProceso* infoProc = list_find(infoProcs, (void*) _proc);

	log_trace(logKernel, "Planifico proceso %d", pcb->pid);
	pthread_mutex_lock(&mut_planificacion2);
	while(quantumRestante > 0){
		pthread_mutex_unlock(&mut_planificacion2);

		if(quantumRestante > 1)
			msg_enviar_separado(EJECUTAR_INSTRUCCION, 1, 0, cpuUsada->socket);
		if(quantumRestante == 1)
			msg_enviar_separado(EJECUTAR_ULTIMA_INSTRUCCION, 1, 0, cpuUsada->socket);

		infoProc->cantRafagas++;
		sem_wait(&cpuUsada->sem);
		pthread_mutex_lock(&mut_planificacion);

		log_trace(logKernel, "Planifico correctamente");
		pthread_mutex_lock(&mut_planificacion2);
	}

	log_trace(logKernel, "Fin rafaga RR");
	list_add(lista_cpus, cpuUsada);
}




void planificar_FIFO(){

	sem_post(&sem_cantColaReady);		//una vez q entre xq wait me resto 1 sin cambiar el tamaniodeCola
	pthread_mutex_lock(&mut_planificacion);

	log_trace(logKernel, "Inicio FIFO");
	t_num8 pidPCB;
	sem_wait(&sem_cantColaReady);
	memcpy(&pidPCB, queue_pop(cola_Ready), sizeof(t_num8));
	int _es_PCB(t_PCB* p){
		return p->pid == pidPCB;
	}
	int _cpuLibre(t_cpu* c){
		return c->libre;
	}
	queue_push(cola_Exec, &pidPCB);

	t_PCB* pcb = list_find(lista_PCBs, (void*) _es_PCB);
	if(pcb == NULL)
		log_error(logKernel, "no existe el proceso con pid %d", pidPCB);

	t_cpu* cpuUsada = list_find(lista_cpus, (void*) _cpuLibre);
	if(cpuUsada == NULL)
		log_error(logKernel, "No hay CPUS libres"); //todo: semaforo cnat cpus
	cpuUsada->libre = false;
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

	int _proc(t_infoProceso* a){
		return a->pid == pcb->pid;
	}
	t_infoProceso* infoProc = list_find(infoProcs, (void*) _proc);

	pthread_mutex_lock(&mut_planificacion2);
	while(sigoFIFO){
		pthread_mutex_unlock(&mut_planificacion2);

		log_trace(logKernel, "Planifico proceso %d en cpu %d", pcb->pid, cpuUsada->socket);

		msg_enviar_separado(EJECUTAR_INSTRUCCION, 0, 0, cpuUsada->socket);

		infoProc->cantRafagas++;
		sem_post(&cpuUsada->sem);
		pthread_mutex_lock(&mut_planificacion);

		log_trace(logKernel, "Planifico correctamente");
		pthread_mutex_lock(&mut_planificacion2);
	}
	pthread_mutex_unlock(&mut_planificacion2);

	free(pcbSerializado);
	log_trace(logKernel, "Fin FIFO");
	pthread_mutex_unlock(&mut_planificacion);
}


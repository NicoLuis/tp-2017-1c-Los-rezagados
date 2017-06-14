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
		log_trace(logKernel, "Inicio FIFO");

		t_num8 pidPCB = _sacarDeCola(0, cola_Ready, mutex_Ready);
		int _es_PCB(t_PCB* p){
			return p->pid == pidPCB;
		}
		int _cpuLibre(t_cpu* c){
			return c->libre;
		}
		sem_wait(&sem_cantCPUs);
		_ponerEnCola(pidPCB, cola_Exec, mutex_Exec);

		t_PCB* pcb = list_find(lista_PCBs, (void*) _es_PCB);
		if(pcb == NULL)
			log_error(logKernel, "no existe el proceso con pid %d", pidPCB);

		t_cpu* cpuUsada = list_find(lista_cpus, (void*) _cpuLibre);
		cpuUsada->libre = false;
		log_trace(logKernel, "cpuUsada %d", cpuUsada->socket);

		t_infosocket* info = malloc(sizeof(t_infosocket));
		info->pid = pcb->pid;
		info->socket = cpuUsada->socket;
		list_add(lista_PCB_cpu, info);

		uint32_t size = tamanioTotalPCB(pcb);
		void* pcbSerializado = serializarPCB(pcb);
		msg_enviar_separado(ENVIO_PCB, size, pcbSerializado, cpuUsada->socket);
		log_trace(logKernel, "ENVIO_PCB");

		int _proc(t_infoProceso* a){
			return a->pid == pcb->pid;
		}
		//t_infoProceso* infoProc = list_find(infoProcs, (void*) _proc);
		log_trace(logKernel, "Planifico proceso %d en cpu %d", pcb->pid, cpuUsada->socket);

		pthread_mutex_unlock(&cpuUsada->mutex);
		pthread_mutex_lock(&cpuUsada->mutex);

		free(pcbSerializado);
		log_trace(logKernel, "Fin FIFO");
		pthread_mutex_unlock(&cpuUsada->mutex);
	}
}


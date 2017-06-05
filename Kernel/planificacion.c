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
		sem_post(&sem_cantColaReady);		//una vez q entre xq wait me resto 1 sin cambiar el tamaniodeCola
		pthread_mutex_lock(&mut_planificacion);

		log_trace(logKernel, "Inicio FIFO");
		t_num8 pidPCB;
		sem_wait(&sem_cantColaReady);
		pidPCB = _sacarDeCola(0, cola_Ready, mutex_Ready);
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
		sem_post(&cpuUsada->sem);

		pthread_mutex_lock(&mut_planificacion2);
		pthread_mutex_unlock(&mut_planificacion2);

		pthread_mutex_lock(&mut_planificacion);

		free(pcbSerializado);
		log_trace(logKernel, "Fin FIFO");
	}
}


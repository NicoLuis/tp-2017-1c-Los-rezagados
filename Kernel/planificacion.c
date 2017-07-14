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
		log_trace(logKernel, "Busco CPU");

		int _cpuLibre(t_cpu* c){
			return c->libre;
		}

		t_cpu* cpuUsada;
		do{
		sem_wait(&sem_cantCPUs);
		_unlockLista_cpus();
		cpuUsada = list_find(lista_cpus, (void*) _cpuLibre);
		_unlockLista_cpus();
		} while(cpuUsada == NULL);

		t_num8 pidPCB = _sacarDeCola(0, cola_Ready, mutex_Ready);
		_ponerEnCola(pidPCB, cola_Exec, mutex_Exec);

		_lockLista_PCBs();
		int _es_PCB(t_PCB* p){
			return p->pid == pidPCB;
		}
		t_PCB* pcb = list_find(lista_PCBs, (void*) _es_PCB);
		_unlockLista_PCBs();
		if(pcb == NULL)
			log_error(logKernel, "no existe el proceso con pid %d", pidPCB);

		cpuUsada->libre = false;
		log_trace(logKernel, " [CPU %d] Libre - asigno pid %d", cpuUsada->socket, pidPCB);

		t_infosocket* info = malloc(sizeof(t_infosocket));
		info->pidPCB = pcb->pid;
		info->socket = cpuUsada->socket;
		list_add(lista_PCB_cpu, info);

		uint32_t size = tamanioTotalPCB(pcb);
		void* pcbSerializado = serializarPCB(pcb);
		if( msg_enviar_separado(ENVIO_PCB, size, pcbSerializado, cpuUsada->socket) < 0 )
			log_trace(logKernel, "No se envio un joraca");


		//log_trace(logKernel, "Planifico proceso %d en cpu %d", pcb->pid, cpuUsada->socket);

		//sem_wait(&cpuUsada->semaforo);

		free(pcbSerializado);
	}
}


















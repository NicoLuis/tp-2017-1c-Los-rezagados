/*
 * planificacion.c
 *
 *  Created on: 15/5/2017
 *      Author: utnso
 */


#include "planificacion.h"

#include "operacionesPCBKernel.h"

void finalizar(t_pid pidPCB);

void planificar(){

	while(1){

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
		log_trace(logKernel, "	");
		log_trace(logKernel, " [CPU %d] CPU Libre", cpuUsada->socket);
		sem_wait(&sem_cantColaReady);
		t_pid pidPCB = _sacarDeCola(0, cola_Ready, mutex_Ready);
		log_warning(logKernel, " 								[PID %d] SACO READY", pidPCB);

		_lockLista_PCBs();
		int _es_PCB(t_PCB* p){
			return p->pid == pidPCB;
		}
		t_PCB* pcb = list_find(lista_PCBs, (void*) _es_PCB);
		_unlockLista_PCBs();
		if(pcb == NULL)
			log_error(logKernel, "no existe el proceso con pid %d", pidPCB);
		else{
			if(pcb->exitCode != SIN_ASIGNAR){

				finalizar(pcb->pid);

			}else{

				log_trace(logKernel, " [CPU %d | PID %d] Inicio Ejecucion",  cpuUsada->socket, pidPCB);
				_ponerEnCola(pidPCB, cola_Exec, mutex_Exec);

				cpuUsada->libre = false;

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



	}
}




void finalizar(t_pid pidPCB){

	log_trace(logKernel, "Finalizo pid %d", pidPCB);

	int _buscarPID(t_infosocket* i){
		return i->pidPCB == pidPCB;
	}

	int i=0, sizeLista = list_size(lista_tabla_de_procesos);
	//log_trace(logKernel, "sizeLista %d", sizeLista);
	for(;i < sizeLista;i++){
		//log_trace(logKernel, "i %d", i);
		t_tabla_proceso* tablaProceso = list_get(lista_tabla_de_procesos, i);
		if(tablaProceso != NULL)
			//log_trace(logKernel, "NULL");
		//else
		if(tablaProceso->pid == pidPCB){
			list_remove(lista_tabla_de_procesos, i);
			int j=0, sizeLista2 = list_size(tablaProceso->lista_entradas_de_proceso);
			//log_trace(logKernel, "sizeLista %d", sizeLista2);
			for(; j<sizeLista2 ;j++){
				t_entrada_proceso* entradaProcesoBuscado = list_remove(tablaProceso->lista_entradas_de_proceso, j);
				int _buscarPorIndice(t_entrada_GlobalFile* entradaGlobal){return entradaGlobal->indiceGlobalTable == entradaProcesoBuscado->referenciaGlobalTable;}

				pthread_mutex_lock(&mutex_tablaGlobal);
				t_entrada_GlobalFile* entradaGlobalBuscada = list_remove_by_condition(lista_tabla_global, (void*) _buscarPorIndice);
				if(entradaGlobalBuscada != NULL){
					//log_trace(logKernel, "NULL");
				//else{
					entradaGlobalBuscada->Open --;
					if(entradaGlobalBuscada->Open == 0)
						free(entradaGlobalBuscada);
					else
						list_add(lista_tabla_global, entradaGlobalBuscada);
				}
				pthread_mutex_unlock(&mutex_tablaGlobal);
				free(entradaProcesoBuscado);
			}
			list_destroy(tablaProceso->lista_entradas_de_proceso);
			free(tablaProceso);
		}
	}

	list_remove_and_destroy_by_condition(lista_PCB_consola, (void*) _buscarPID, free);

	bool _buscarPCB(t_PCB* pcb){
		return pcb->pid == pidPCB;
	}

	_lockLista_PCBs();
	t_PCB* pcb = list_find(lista_PCBs, (void*) _buscarPCB);
	if(pcb == NULL)
		log_warning(logKernel, "No se encuentra pcb %d", pidPCB);
	else{
		if(!_estaEnCola(pcb->pid, cola_Exit, mutex_Exit))
			_ponerEnCola(pcb->pid, cola_Exit, mutex_Exit);
		liberarPCB(pcb, true);
	}
	_unlockLista_PCBs();

	pthread_mutex_lock(&mutex_listaSemaforos);
	void _sacarDeSemaforos(t_VariableSemaforo* sem){
		t_pid encontrado = _sacarDeCola(pidPCB, sem->colaBloqueados, sem->mutex_colaBloqueados);
		if(encontrado == pidPCB)
			sem->valorSemaforo++;
	}
	list_iterate(lista_variablesSemaforo, (void*) _sacarDeSemaforos);
	pthread_mutex_unlock(&mutex_listaSemaforos);

	_lockMemoria();
	send(socket_memoria, &pidPCB, sizeof(t_pid), 0);
	msg_enviar_separado(FINALIZAR_PROGRAMA, 0, 0, socket_memoria);
	_unlockMemoria();
	sem_post(&sem_gradoMp);
	sem_post(&sem_cantCPUs);

	log_trace(logKernel, "FIN pid %d", pidPCB);
}













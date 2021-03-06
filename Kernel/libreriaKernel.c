/*
 * libreriaKernel.c
 *
 *  Created on: 3/4/2017
 *      Author: utnso
 */


#include "libreriaKernel.h"

#include "operacionesPCBKernel.h"
#include "planificacion.h"


t_puntero reservarMemoriaHeap(t_num cantBytes, t_PCB* pcb, int socket_cpu);	//si, pinto ponerlo aca
int liberarMemoriaHeap(t_puntero posicion, t_PCB* pcb, int socket_cpu);

void mostrarArchivoConfig() {

	system("clear");
	printf("El puerto del Programa es: %d\n", puertoPrograma);
	printf("El puerto de la CPU es: %d\n", puertoCPU);
	printf("La IP de la Memoria es: %s\n", ipMemoria);
	printf("El puerto de la Memoria es: %d\n", puertoMemoria);
	printf("La IP del File System es: %s\n", ipFileSystem);
	printf("El puerto del File System es: %d\n", puertoFS);
	printf("El Quantum es de: %d\n", quantum);
	printf("El Quantum de Sleep es de: %d\n", quantumSleep);
	printf("EL algoritmo de planificacion es: %s\n", algoritmo);
	printf("El grado de Multiprogramacion es: %d\n", gradoMultiprogramacion);
	//
	//
	//
	printf("El tamaño del Stack es de: %d\n", stackSize);
}

void inicializarSemaforosYVariables(char** ids, char** valores, char** shared_vars){
	lista_variablesSemaforo = list_create();
	int i;
	for(i = 0; ids[i] != NULL ;i++){
		t_VariableSemaforo* semaforo = malloc(sizeof(t_VariableSemaforo));
		semaforo->nombre = ids[i];
		semaforo->valorSemaforo = atoi(valores[i]);
		semaforo->colaBloqueados = queue_create();
		pthread_mutex_init(&(semaforo->mutex_colaBloqueados), NULL);
		list_add(lista_variablesSemaforo, semaforo);
	}

	lista_variablesCompartidas = list_create();
	for(i = 0; shared_vars[i] != NULL ;i++){
		t_VariableCompartida* variable = malloc(sizeof(t_VariableCompartida));
		variable->nombre = string_substring(shared_vars[i], 1, string_length(shared_vars[i])-1);
		variable->valor = 0;
		log_info(logKernel, "shared_vars[%d] %s", i, variable->nombre);
		list_add(lista_variablesCompartidas, variable);
	}
	sem_init(&sem_gradoMp, 0, gradoMultiprogramacion);
	sem_init(&sem_cantColaReady, 0, 0);
}



int handshake(int socket_cliente, int tipo){
	int retorno = 0;
	char* bufferEscucha = malloc(200);

	if(tipo == 0)		//si tipo = 0 pregunto, sino me preguntan
		send(socket_cliente, "Hola quien sos?", 16, 0);

	int bytesRecibidos = recv(socket_cliente, bufferEscucha, 50, 0);
	if (bytesRecibidos <= 0){
		free(bufferEscucha);
		return -1;
	}

	bufferEscucha[bytesRecibidos] = '\0';
	if(tipo == 0){
		if (strcmp("Hola soy la Consola", bufferEscucha) == 0){
			if (send(socket_cliente, "Conexion Aceptada", 18, 0) <= 0)
				retorno = -2;
			retorno = 1;
		}else
		if (strcmp("Hola soy la CPU", bufferEscucha) == 0){

			void* buffer = malloc(18 + 5 + sizeof(t_num)*2);
			memcpy(buffer, "Conexion Aceptada", 18);
			memcpy(buffer + 18, algoritmo, 5);
			memcpy(buffer + 18 + 5, &quantum, sizeof(t_num));
			memcpy(buffer + 18 + 5 + sizeof(t_num), &quantumSleep, sizeof(t_num));

			if (send(socket_cliente, buffer, 18 + 5 + sizeof(t_num)*2, 0) <= 0)
				retorno = -2;
			retorno = 2;
			free(buffer);
		}
	}else{
		if(strcmp("Hola quien sos?", bufferEscucha) != 0){
			free(bufferEscucha);
			return -2;
		}

		send(socket_cliente, "Hola soy el KERNEL", 19, 0);

		bytesRecibidos = recv(socket_cliente, bufferEscucha, 50, 0);
		if (bytesRecibidos <= 0){
			free(bufferEscucha);
			return -1;
		}

		bufferEscucha[bytesRecibidos] = '\0';

		if (strcmp("Conexion aceptada", bufferEscucha) == 0)
			retorno = 1;
	}

	free(bufferEscucha);

	return retorno;
}

void _sumarCantOpAlocar(t_pid pid, t_num cantBytes){
	_lockLista_infoProc();
	int _espidproc(t_infoProceso* a){ return a->pid == pid; }
	t_infoProceso* infP = list_remove_by_condition(infoProcs, (void*) _espidproc);
	infP->cantOp_alocar++;
	infP->canrBytes_alocar += cantBytes;
	list_add(infoProcs, infP);
	_unlockLista_infoProc();
}

void _sumarCantOpLiberar(t_pid pid, t_num cantBytes){
	_lockLista_infoProc();
	int _espidproc(t_infoProceso* a){ return a->pid == pid; }
	t_infoProceso* infP = list_remove_by_condition(infoProcs, (void*) _espidproc);
	infP->cantOp_liberar++;
	infP->canrBytes_liberar += cantBytes;
	list_add(infoProcs, infP);
	_unlockLista_infoProc();
}


void _sumarCantPagsHeap(t_pid pid, int cant){
	_lockLista_infoProc();
	int _espidproc(t_infoProceso* a){ return a->pid == pid; }
	t_infoProceso* infP = list_remove_by_condition(infoProcs, (void*) _espidproc);
	infP->cantPagsHeap = infP->cantPagsHeap + cant;
	list_add(infoProcs, infP);
	_unlockLista_infoProc();
}

int _cantPagsHeap(t_pid pid){
	int cant;
	_lockLista_infoProc();
	int _espidproc(t_infoProceso* a){ return a->pid == pid; }
	t_infoProceso* infP = list_find(infoProcs, (void*) _espidproc);
	cant = infP->cantPagsHeap;
	_unlockLista_infoProc();
	return cant;
}

void _sumarCantOpPriv(t_pid pid){
	_lockLista_infoProc();
	int _espidproc(t_infoProceso* a){ return a->pid == pid; }
	t_infoProceso* infP = list_remove_by_condition(infoProcs, (void*) _espidproc);
	infP->cantOpPriv++;
	list_add(infoProcs, infP);
	_unlockLista_infoProc();
}

t_PCB* recibir_pcb(int socket_cpu, t_msg* msgRecibido, bool flag_bloqueo){
	bool _esCPU2(t_infosocket* aux){
		return aux->socket == socket_cpu;
	}
	t_PCB* pcb = desserealizarPCB(msgRecibido->data);
	log_trace(logKernel, " [CPU %d | PID %d] Recibi PCB", socket_cpu, pcb->pid);
	_lockLista_PCB_cpu();
	list_remove_and_destroy_by_condition(lista_PCB_cpu, (void*) _esCPU2, free);
	_unlockLista_PCB_cpu();
	_sacarDeCola(pcb->pid, cola_Exec, mutex_Exec);

	if(flag_bloqueo)
		_ponerEnCola(pcb->pid, cola_Block, mutex_Block);

	_lockLista_infoProc();
	int _espidproc(t_infoProceso* a){ return a->pid == pcb->pid; }
	t_infoProceso* infP = list_remove_by_condition(infoProcs, (void*) _espidproc);
	infP->cantRafagas = pcb->cantRafagas;
	list_add(infoProcs, infP);
	_unlockLista_infoProc();

	return pcb;
}


void liberarCPU(t_cpu* cpu){
	cpu->libre = true;
	//pthread_mutex_unlock(&cpu->mutex);
	log_trace(logKernel, " [CPU %d] Libero CPU", cpu->socket);
	sem_post(&sem_cantCPUs);
}


void escucharCPU(int socket_cpu) {

	bool _esCPU(t_cpu* c){
		return c->socket == socket_cpu;
	}
	bool _esCPU2(t_infosocket* in){
		return in->socket == socket_cpu;
	}
	_lockLista_cpus();
	t_cpu* cpuUsada = list_find(lista_cpus, (void*) _esCPU);
	_unlockLista_cpus();


	while(1){

		//sem_wait(&cpuUsada->semaforo);
		pthread_mutex_lock(&mut_detengo_plani);
		pthread_mutex_unlock(&mut_detengo_plani);

		bool flag_finalizado = false;

		t_msg* msgRecibido = msg_recibir(socket_cpu);
		if(msgRecibido->longitud > 0)
			if(msg_recibir_data(socket_cpu, msgRecibido) <= 0)
				msgRecibido->tipoMensaje = 0;

		_lockLista_PCB_cpu();
		t_infosocket* aux = list_find(lista_PCB_cpu, (void*) _esCPU2);
		t_pid pidaux = 0;
		if(aux != NULL)
			pidaux = aux->pidPCB;
		_unlockLista_PCB_cpu();

		int _es_PCB(t_PCB* p){
			return p->pid == pidaux;
		}

		_lockLista_PCBs();
		t_PCB* pcb = list_find(lista_PCBs, (void*) _es_PCB);
		_unlockLista_PCBs();
		bool _esPid(t_infosocket* a){ return a->pidPCB == pcb->pid; }

		t_VariableCompartida* varCompartida = malloc(sizeof(t_VariableCompartida));
		int _buscar_VarComp(t_VariableCompartida* p){
			return string_equals_ignore_case(p->nombre, varCompartida->nombre);
		}
		t_VariableSemaforo* varSemaforo = malloc(sizeof(t_VariableSemaforo));
		int _buscar_VarSem(t_VariableSemaforo* s){
			return string_equals_ignore_case(s->nombre, varSemaforo->nombre);
		}
		t_num tamanioNombre, offset = 0;
		int tmpsize;

//-------------- VARIABLES EMPLEADAS PARA LEER Y ESCRIBIR -------------------

		void* datos;

//-------------- VARIABLES EMPLEADAS PARA BORRAR Y CERRAR --------------------

		t_descriptor_archivo fdRecibido;
		t_pid pidRecibido;

		t_tabla_proceso* TablaProceso = malloc(sizeof(t_tabla_proceso));
		t_entrada_proceso* entradaProcesoBuscado = malloc(sizeof(t_entrada_proceso));
		t_entrada_GlobalFile* entradaGlobalBuscada = malloc(sizeof(t_entrada_GlobalFile));
		int _esFD(t_entrada_proceso *entradaP){return entradaP->fd == entradaProcesoBuscado->fd;}
		int _esIndiceGlobal(t_entrada_GlobalFile *entradaG){return entradaG->indiceGlobalTable == entradaGlobalBuscada->indiceGlobalTable;}

		int _buscarPid(t_tabla_proceso* tabla){ return pidRecibido == tabla->pid; }
		int _buscarFD(t_entrada_proceso* entradaP){ return fdRecibido == entradaP->fd; }
		int _buscarPorIndice(t_entrada_GlobalFile* entradaGlobal){return entradaGlobal->indiceGlobalTable == entradaProcesoBuscado->referenciaGlobalTable;}

		void _cargarEntradasxTabla(){

			log_trace(logKernel, " [CPU %d | PID %d] fdRecibido %d pidRecibido %d", socket_cpu, pcb->pid, fdRecibido, pidRecibido);

			TablaProceso = list_find(lista_tabla_de_procesos, (void*) _buscarPid); //tabla proceso es una lista

			if(TablaProceso == NULL){
				log_trace(logKernel, " [CPU %d | PID %d] no existe la tabla de este proceso", socket_cpu, pcb->pid);

				// terminar??
			} else {
				entradaProcesoBuscado = list_find(TablaProceso->lista_entradas_de_proceso, (void*) _buscarFD); //tabla proceso es una lista

				if(entradaProcesoBuscado == NULL){
					log_trace(logKernel, " [CPU %d | PID %d] no existe entrada de la tabla proceso que tenga a FD", socket_cpu, pcb->pid);
					// hacer algo??
				} else {

					entradaGlobalBuscada = list_find(lista_tabla_global, (void*) _buscarPorIndice);
					//entradaGlobalBuscada = list_remove_by_condition(lista_tabla_global, (void*) _buscarPorIndice);
				}
			}
		}

//-------------------------------------------------------------------------------
		t_num exitcode;
		switch(msgRecibido->tipoMensaje){



		case FINALIZAR_PROGRAMA:
			log_trace(logKernel, " [CPU %d | PID %d] Recibi FINALIZAR_PROGRAMA de CPU", socket_cpu, pcb->pid);
			flag_finalizado = true;
			//no break
		case ENVIO_PCB:		// si me devuelve el PCB es porque fue la ultima instruccion
			exitcode = 0;
			t_PCB* pcbRecibido = recibir_pcb(socket_cpu, msgRecibido, 0);
			if(flag_finalizado){
				_lockLista_PCB_consola();
				t_infosocket* info = list_remove_by_condition(lista_PCB_consola, (void*) _esPid);
				_unlockLista_PCB_consola();
				_lockLista_PCB_cpu();
				list_remove_and_destroy_by_condition(lista_PCB_cpu, (void*) _esPid, free);
				_unlockLista_PCB_cpu();
				if(info == NULL)
					log_trace(logKernel, " [CPU %d | PID %d] No se encuentra consola asociada", socket_cpu, pcb->pid);
				else
					msg_enviar_separado(FINALIZAR_PROGRAMA, sizeof(t_pid), &info->pidPCB, info->socket);
				sem_post(&sem_gradoMp);
				free(info);
				finalizarPid(pcb->pid);
			}

			_lockLista_PCBs();
			pcb = list_remove_by_condition(lista_PCBs, (void*) _es_PCB);
			if(pcb == NULL) log_warning(logKernel, " [CPU %d | PID %d] No se encuentra pcb en ENVIO_PCB", socket_cpu, pcb->pid);
			else if(pcb->exitCode != SIN_ASIGNAR){
				exitcode = pcb->exitCode;
				pcbRecibido->exitCode = exitcode;
			}
			liberarPCB(pcb, 0);
			list_add(lista_PCBs, pcbRecibido);
			_unlockLista_PCBs();

			if(flag_finalizado)
				setearExitCode(pcbRecibido->pid, exitcode);
			liberarCPU(cpuUsada);
			_ponerEnCola(pcbRecibido->pid, cola_Ready, mutex_Ready);
			sem_post(&sem_cantColaReady);
			break;




		case 0: case FIN_CPU:
			fprintf(stderr, PRINT_COLOR_YELLOW "La cpu %d se ha desconectado" PRINT_COLOR_RESET "\n", socket_cpu);
			log_trace(logKernel, " [CPU %d] Se desconecto la CPU", socket_cpu);
			//pthread_mutex_unlock(&cpuUsada->mutex);
			//si la cpu se desconecto la saco de la lista
			_lockLista_cpus();
			list_remove_and_destroy_by_condition(lista_cpus, (void*) _esCPU, free);
			_unlockLista_cpus();
			_lockLista_PCB_cpu();
			list_remove_and_destroy_by_condition(lista_PCB_cpu, (void*) _esCPU2, free);
			_unlockLista_PCB_cpu();
			if(pcb != NULL){
				_lockLista_PCB_consola();
				t_infosocket* infoc = list_find(lista_PCB_consola, (void*) _esPid);
				_unlockLista_PCB_consola();
				if(infoc != NULL)
					msg_enviar_separado(ERROR, sizeof(t_pid), &pcb->pid, infoc->socket);
				_sacarDeCola(pcb->pid, cola_Exec, mutex_Exec);
				setearExitCode(pcb->pid, SIN_DEFINICION);
				void _sacarDeSemaforos(t_VariableSemaforo* sem){
					t_pid encontrado = _sacarDeCola(pcb->pid, sem->colaBloqueados, sem->mutex_colaBloqueados);
					if(encontrado == pcb->pid)
						sem->valorSemaforo++;
				}
				list_iterate(lista_variablesSemaforo, (void*) _sacarDeSemaforos);
				_lockLista_PCBs();
				if(list_remove_by_condition(lista_PCBs, (void*) _es_PCB) == NULL) log_warning(logKernel, " [CPU %d | PID %d] No se encuentra pcb en FIN_CPU", socket_cpu, pcb->pid);
				list_add(lista_PCBs, pcb);
				_unlockLista_PCBs();
			}
			sem_post(&sem_gradoMp);
			pthread_exit(NULL);
			break;




		case DESCONECTAR:
		case EXCEPCION_MEMORIA:
		case STACKOVERFLOW:
		case PAGINA_INVALIDA:
		case ERROR:
			log_trace(logKernel, " [CPU %d | PID %d] Recibi ERROR de CPU", socket_cpu, pcb->pid);
			t_PCB* pcbRecibido2 = recibir_pcb(socket_cpu, msgRecibido, 0);
			_lockLista_PCB_consola();
			t_infosocket* info = list_remove_by_condition(lista_PCB_consola, (void*) _esPid);
			_unlockLista_PCB_consola();
			finalizarPid(pcb->pid);
			if(pcb->exitCode == SIN_ASIGNAR && pcbRecibido2->exitCode == SIN_ASIGNAR){
				if(msgRecibido->tipoMensaje == STACKOVERFLOW)
					pcbRecibido2->exitCode = ERROR_STACKOVERFLOW;
				if(msgRecibido->tipoMensaje == EXCEPCION_MEMORIA)
					pcbRecibido2->exitCode = ERROR_EXCEPCION_MEMORIA;
			}
			_lockLista_PCBs();
			pcb = list_remove_by_condition(lista_PCBs, (void*) _es_PCB);
			if(pcb == NULL) log_warning(logKernel, " [CPU %d | PID %d] No se encuentra pcb en ERROR", socket_cpu, pcb->pid);
			else if(pcb->exitCode != SIN_ASIGNAR){
				exitcode = pcb->exitCode;
				pcbRecibido2->exitCode = exitcode;
			}
			liberarPCB(pcb, true);
			list_add(lista_PCBs, pcbRecibido2);
			_unlockLista_PCBs();
			msg_enviar_separado(ERROR, sizeof(t_pid), &pcbRecibido2->pid, info->socket);
			sem_post(&sem_gradoMp);
			liberarCPU(cpuUsada);
			if(!_estaEnCola(pcbRecibido2->pid, cola_Ready, mutex_Ready)){
				_ponerEnCola(pcbRecibido2->pid, cola_Ready, mutex_Ready);
				sem_post(&sem_cantColaReady);
			}
			break;




		case GRABAR_VARIABLE_COMPARTIDA:
			log_trace(logKernel, " [CPU %d | PID %d] Recibi GRABAR_VARIABLE_COMPARTIDA de CPU", socket_cpu, pcb->pid);
			memcpy(&tamanioNombre, msgRecibido->data, sizeof(t_num));
			offset += sizeof(t_num);
			varCompartida->nombre = malloc(tamanioNombre+1);
			memcpy(varCompartida->nombre, msgRecibido->data + offset, tamanioNombre);
			varCompartida->nombre[tamanioNombre] = '\0';
			offset += tamanioNombre;
			memcpy(&varCompartida->valor, msgRecibido->data + offset, sizeof(t_valor_variable));
			log_info(logKernel, " [CPU %d | PID %d] Variable %s, valor %d", socket_cpu, pcb->pid, varCompartida->nombre, varCompartida->valor);

			t_VariableCompartida* varBuscada = list_find(lista_variablesCompartidas, (void*) _buscar_VarComp);
			if (varBuscada == NULL){
				log_error(logKernel, " [CPU %d | PID %d] Error no encontre VALOR_VARIABLE_COMPARTIDA", socket_cpu, pcb->pid);
				t_num exitCode = VARIABLE_COMPARTIDA_INEXISTENTE;
				msg_enviar_separado(ERROR, sizeof(t_num), &exitCode, socket_cpu);
			}else{
				varBuscada->valor = varCompartida->valor;
				msg_enviar_separado(GRABAR_VARIABLE_COMPARTIDA, 0, 0, socket_cpu);
				_sumarCantOpPriv(pcb->pid);
			}
			free(varCompartida->nombre);
			//sem_post(&cpuUsada->semaforo);
			break;




		case VALOR_VARIABLE_COMPARTIDA:
			log_trace(logKernel, " [CPU %d | PID %d] Recibi VALOR_VARIABLE_COMPARTIDA de CPU", socket_cpu, pcb->pid);
			memcpy(&tamanioNombre, &msgRecibido->longitud, sizeof(t_num));
			varCompartida->nombre = malloc(tamanioNombre+1);
			memcpy(varCompartida->nombre, msgRecibido->data, tamanioNombre);
			varCompartida->nombre[tamanioNombre] = '\0';
			log_info(logKernel, " [CPU %d | PID %d] Variable %s", socket_cpu, pcb->pid, varCompartida->nombre);

			t_VariableCompartida* varBuscad = list_find(lista_variablesCompartidas, (void*) _buscar_VarComp);
			if (varBuscad == NULL){
				log_error(logKernel, " [CPU %d | PID %d] Error no encontre VALOR_VARIABLE_COMPARTIDA", socket_cpu, pcb->pid);
				t_num exitCode = VARIABLE_COMPARTIDA_INEXISTENTE;
				msg_enviar_separado(ERROR, sizeof(t_num), &exitCode, socket_cpu);
			}else{
				log_info(logKernel, " [CPU %d | PID %d] Valor %d", socket_cpu, pcb->pid, varBuscad->valor);
				msg_enviar_separado(VALOR_VARIABLE_COMPARTIDA, sizeof(t_valor_variable), &varBuscad->valor, socket_cpu);
				_sumarCantOpPriv(pcb->pid);
			}
			free(varCompartida->nombre);
			//sem_post(&cpuUsada->semaforo);
			break;




		case SIGNAL:
			log_trace(logKernel, " [CPU %d | PID %d] Recibi SIGNAL de CPU", socket_cpu, pcb->pid);
			memcpy(&tamanioNombre, &msgRecibido->longitud, sizeof(t_num));
			varSemaforo->nombre = malloc(tamanioNombre+1);
			memcpy(varSemaforo->nombre, msgRecibido->data, tamanioNombre);
			varSemaforo->nombre[tamanioNombre] = '\0';
			log_info(logKernel, " [CPU %d | PID %d] Semaforo %s", socket_cpu, pcb->pid, varSemaforo->nombre);

			pthread_mutex_lock(&mutex_listaSemaforos);
			t_VariableSemaforo* semBuscado = list_remove_by_condition(lista_variablesSemaforo, (void*) _buscar_VarSem);
			if (semBuscado == NULL){
				log_error(logKernel, " [CPU %d | PID %d] Error no encontre SEMAFORO %s", socket_cpu, pcb->pid, varSemaforo->nombre);
				t_num exitCode = SEMAFORO_INEXISTENTE;
				msg_enviar_separado(ERROR, sizeof(t_num), &exitCode, socket_cpu);
			}else{
				semBuscado->valorSemaforo++;
				log_trace(logKernel, " [CPU %d | PID %d] Valor %d", socket_cpu, pcb->pid, semBuscado->valorSemaforo);
				if(semBuscado->valorSemaforo <= 0){
					t_pid pidADesbloquear;
					void* tmp = malloc(sizeof(t_pid));
					pthread_mutex_lock(&semBuscado->mutex_colaBloqueados);
					tmp = queue_pop(semBuscado->colaBloqueados);
					pthread_mutex_unlock(&semBuscado->mutex_colaBloqueados);
					memcpy(&pidADesbloquear, tmp, sizeof(t_pid));
					free(tmp);
					log_trace(logKernel, " [CPU %d | PID %d] Desbloqueo proceso %d", socket_cpu, pcb->pid, pidADesbloquear);

					if(_sacarDeCola(pidADesbloquear, cola_Block, mutex_Block) == pidADesbloquear){
						_ponerEnCola(pidADesbloquear, cola_Ready, mutex_Ready);
						sem_post(&sem_cantColaReady);
					}
				}
				list_add(lista_variablesSemaforo, semBuscado);
				msg_enviar_separado(SIGNAL, 0, 0, socket_cpu);
				_sumarCantOpPriv(pcb->pid);
			}
			pthread_mutex_unlock(&mutex_listaSemaforos);
			free(varSemaforo->nombre);
			//sem_post(&cpuUsada->semaforo);
			break;




		case WAIT:
			log_trace(logKernel, " [CPU %d | PID %d] Recibi WAIT de CPU", socket_cpu, pcb->pid);
			memcpy(&tamanioNombre, &msgRecibido->longitud, sizeof(t_num));
			varSemaforo->nombre = malloc(tamanioNombre+1);
			memcpy(varSemaforo->nombre, msgRecibido->data, tamanioNombre);
			varSemaforo->nombre[tamanioNombre] = '\0';
			log_info(logKernel, " [CPU %d | PID %d] Semaforo %s", socket_cpu, pcb->pid, varSemaforo->nombre);

			pthread_mutex_lock(&mutex_listaSemaforos);
			t_VariableSemaforo* semBuscad = list_remove_by_condition(lista_variablesSemaforo, (void*) _buscar_VarSem);
			if (semBuscad == NULL){
				log_error(logKernel, " [CPU %d | PID %d] Error no encontre SEMAFORO %s", socket_cpu, pcb->pid, varSemaforo->nombre);
				t_num exitCode = SEMAFORO_INEXISTENTE;
				msg_enviar_separado(ERROR, sizeof(t_num), &exitCode, socket_cpu);
			}else if(pcb->exitCode != SIN_ASIGNAR){
				semBuscad->valorSemaforo--;
				msg_enviar_separado(WAIT, sizeof(t_num), &semBuscad->valorSemaforo, socket_cpu);
				semBuscad->valorSemaforo++;
				list_add(lista_variablesSemaforo, semBuscad);
			}
			else{
				semBuscad->valorSemaforo--;
				log_trace(logKernel, " [CPU %d | PID %d] Valor semaforo %s %d", socket_cpu, pcb->pid, semBuscad->nombre, semBuscad->valorSemaforo);
				msg_enviar_separado(WAIT, sizeof(t_num), &semBuscad->valorSemaforo, socket_cpu);

				if(semBuscad->valorSemaforo < 0){
					void* tmp = malloc(sizeof(t_pid));
					memcpy(tmp, &pcb->pid, sizeof(t_pid));
					pthread_mutex_lock(&semBuscad->mutex_colaBloqueados);
					queue_push(semBuscad->colaBloqueados, tmp);
					pthread_mutex_unlock(&semBuscad->mutex_colaBloqueados);
					log_trace(logKernel, " [CPU %d | PID %d] Bloqueo proceso", socket_cpu, pcb->pid);
					//recibo pcb
					msg_destruir(msgRecibido);
					msgRecibido = msg_recibir(socket_cpu);
					if(msg_recibir_data(socket_cpu, msgRecibido) > 0){
						t_PCB* pcbRecibido = recibir_pcb(socket_cpu, msgRecibido, 1);
						_lockLista_PCBs();
						pcb = list_remove_by_condition(lista_PCBs, (void*) _es_PCB);
						if(pcb == NULL) log_warning(logKernel, " [CPU %d | PID %d] No se encuentra pcb en WAIT", socket_cpu, pcb->pid);
						else if(pcb->exitCode != SIN_ASIGNAR){
							exitcode = pcb->exitCode;
							pcbRecibido2->exitCode = exitcode;
						}
						liberarPCB(pcb, true);
						list_add(lista_PCBs, pcbRecibido);
						_unlockLista_PCBs();
						liberarCPU(cpuUsada);
						_sumarCantOpPriv(pcbRecibido->pid);
					}else
						log_warning(logKernel, "No recibi nada", socket_cpu);
				}else
					_sumarCantOpPriv(pcb->pid);

				list_add(lista_variablesSemaforo, semBuscad);
			}
			pthread_mutex_unlock(&mutex_listaSemaforos);
			free(varSemaforo->nombre);
			//sem_post(&cpuUsada->semaforo);
			break;




		case ASIGNACION_MEMORIA:
			log_trace(logKernel, " [CPU %d | PID %d] Recibi ASIGNACION_MEMORIA de CPU", socket_cpu, pcb->pid);
			t_num cantBytes;
			memcpy(&cantBytes, msgRecibido->data, sizeof(t_valor_variable));
			log_trace(logKernel, " [CPU %d | PID %d] ASIGNACION_MEMORIA %d bytes", socket_cpu, pcb->pid, cantBytes);

			/*//recibo pcb
			tmpsize = msgRecibido->longitud - sizeof(t_valor_variable);
			void* tmpbuffer = malloc(tmpsize);
			memcpy(tmpbuffer, msgRecibido->data + sizeof(t_valor_variable), tmpsize);
			pcb = desserealizarPCB(tmpbuffer);
			free(tmpbuffer);*/
			bool flag_se_agrego_pag = 0;
			if(pcb->cantPagsHeap < _cantPagsHeap(pcb->pid))
				flag_se_agrego_pag = 1;

			t_puntero direccion = reservarMemoriaHeap(cantBytes, pcb, socket_cpu);

			void* tmpb = malloc(sizeof(t_puntero) + sizeof(bool));
			memcpy(tmpb, &direccion, sizeof(t_puntero));
			memcpy(tmpb + sizeof(t_puntero), &flag_se_agrego_pag, sizeof(bool));

			if((int)direccion > 0)
				msg_enviar_separado(ASIGNACION_MEMORIA, sizeof(t_puntero) + sizeof(bool), tmpb, socket_cpu);
			else{
				t_num exitCode = -direccion;
				msg_enviar_separado(ERROR, sizeof(t_num), &exitCode, socket_cpu);
			}
			free(tmpb);


			_lockLista_PCBs();
			if(list_remove_by_condition(lista_PCBs, (void*) _es_PCB) == NULL) log_warning(logKernel, " [CPU %d | PID %d] No se encuentra pcb en ASIGNACION_MEMORIA", socket_cpu, pcb->pid);
			list_add(lista_PCBs, pcb);
			_unlockLista_PCBs();

			_sumarCantOpPriv(pcb->pid);
			//sem_post(&cpuUsada->semaforo);
			break;



		case LIBERAR:
			log_trace(logKernel, " [CPU %d | PID %d] Recibi LIBERAR de CPU", socket_cpu, pcb->pid);
			t_puntero posicion;
			memcpy(&posicion, msgRecibido->data, msgRecibido->longitud);
			log_trace(logKernel, " [CPU %d | PID %d] LIBERAR posicion %d", socket_cpu, pcb->pid, posicion);

			if( liberarMemoriaHeap(posicion, pcb, socket_cpu) < 0 ){
				t_num exitCode = ERROR_EXCEPCION_MEMORIA;
				msg_enviar_separado(ERROR, sizeof(t_num), &exitCode, socket_cpu);
			}
			else
				msg_enviar_separado(LIBERAR, 0, 0, socket_cpu);

			_lockLista_PCBs();
			if(list_remove_by_condition(lista_PCBs, (void*) _es_PCB) == NULL) log_warning(logKernel, " [CPU %d | PID %d] No se encuentra pcb en LIBERAR", socket_cpu, pcb->pid);
			list_add(lista_PCBs, pcb);
			_unlockLista_PCBs();

			_sumarCantOpPriv(pcb->pid);
			//sem_post(&cpuUsada->semaforo);
			break;




		case MOVER_ANSISOP:
			log_trace(logKernel, " [CPU %d | PID %d] Recibi MOVER_ANSISOP de CPU", socket_cpu, pcb->pid);
			t_valor_variable cursorRecibido;

			memcpy(&fdRecibido, msgRecibido->data, sizeof(t_descriptor_archivo));
			memcpy(&cursorRecibido, msgRecibido->data + sizeof(t_descriptor_archivo), sizeof(t_valor_variable));
			memcpy(&pidRecibido, msgRecibido->data + sizeof(t_descriptor_archivo) + sizeof(t_valor_variable), sizeof(t_pid));

			_cargarEntradasxTabla();
			bool error = 1;

			if(TablaProceso != NULL)
			if(entradaProcesoBuscado != NULL)
			if(entradaGlobalBuscada != NULL){
				entradaProcesoBuscado->cursor = cursorRecibido;
				log_trace(logKernel, " [CPU %d | PID %d] se seteo el cursor", socket_cpu, pcb->pid);
				msg_enviar_separado(MOVER_ANSISOP,0,0,socket_cpu);
				error = 0;
			}
			if(error)
				msg_enviar_separado(ERROR,0,0,socket_cpu);

			//sem_post(&cpuUsada->semaforo);
			break;




		case LEER_ANSISOP:
			_lockFS();

			log_trace(logKernel, " [CPU %d | PID %d] Recibi LEER_ANSISOP de CPU", socket_cpu, pcb->pid);
			t_valor_variable tamanio;

			memcpy(&fdRecibido, msgRecibido->data + offset, tmpsize = sizeof(t_descriptor_archivo));
			offset += tmpsize;
			memcpy(&tamanio, msgRecibido->data + offset, tmpsize = sizeof(t_valor_variable));
			offset += tmpsize;
			memcpy(&pidRecibido, msgRecibido->data + offset, tmpsize = sizeof(t_pid));
			offset += tmpsize;

			_cargarEntradasxTabla();

			if(TablaProceso != NULL)
			if(entradaProcesoBuscado != NULL)
			if(entradaGlobalBuscada != NULL){
				if(entradaProcesoBuscado->bandera.lectura == true){
					t_num sizePath = string_length(entradaGlobalBuscada->FilePath);
					void* buffer = malloc(sizeof(t_num) + sizePath + sizeof(t_valor_variable)*2);

					offset = 0;
					tmpsize = 0;
					memcpy(buffer, &sizePath, tmpsize = sizeof(t_num));
					offset += tmpsize;
					memcpy(buffer + offset, entradaGlobalBuscada->FilePath, tmpsize = sizePath);
					offset += tmpsize;
					memcpy(buffer+ offset, &entradaProcesoBuscado->cursor, tmpsize = sizeof(t_valor_variable));
					offset += tmpsize;
					memcpy(buffer+ offset, &tamanio, tmpsize = sizeof(t_valor_variable));
					offset += tmpsize;

					msg_enviar_separado(OBTENER_DATOS, offset, buffer, socket_fs);

					free(buffer);

					t_msg* msgDatosObtenidos = msg_recibir(socket_fs);

					if(msgDatosObtenidos->tipoMensaje == OBTENER_DATOS){

						msg_recibir_data(socket_fs, msgDatosObtenidos);
						msg_enviar_separado(OBTENER_DATOS, msgDatosObtenidos->longitud, msgDatosObtenidos->data, socket_cpu);
						msg_destruir(msgDatosObtenidos);

					}else{
						msg_destruir(msgDatosObtenidos);
						log_error(logKernel, " [CPU %d | PID %d] Error al leer", socket_cpu, pcb->pid);
						t_num exitcode = msgDatosObtenidos->tipoMensaje;
						msg_enviar_separado(ERROR, sizeof(t_num), &exitcode, socket_cpu);
					}
				}else{
					log_error(logKernel, " [CPU %d | PID %d] No tiene permisos de escritura", socket_cpu, pcb->pid);

					t_num exitCode = LECTURA_SIN_PERMISOS;
					msg_enviar_separado(ERROR, sizeof(t_num), &exitCode, socket_cpu);

				}

			}
			_unlockFS();
			//sem_post(&cpuUsada->semaforo);
			break;




		case ESCRIBIR_FD:

			log_trace(logKernel, " [CPU %d | PID %d] Recibi ESCRIBIR_FD de CPU", socket_cpu, pcb->pid);
			t_valor_variable sizeInformacion;

			memcpy(&fdRecibido, msgRecibido->data + offset, tmpsize = sizeof(t_descriptor_archivo));
			offset += tmpsize;
			memcpy(&sizeInformacion, msgRecibido->data + offset, tmpsize = sizeof(t_valor_variable));
			offset += tmpsize;
			datos = malloc(sizeInformacion);
			memcpy(datos, msgRecibido->data + offset, tmpsize = sizeInformacion);
			offset += tmpsize;
			memcpy(&pidRecibido, msgRecibido->data + offset, tmpsize = sizeof(t_pid));
			offset += tmpsize;

			if(fdRecibido == 1){
				log_trace(logKernel, " [CPU %d | PID %d] Imprimo por consola: %s", socket_cpu, pcb->pid, datos);
				_lockLista_PCB_consola();
				t_infosocket* info = list_find(lista_PCB_consola, (void*) _esPid);
				_unlockLista_PCB_consola();
				if(info == NULL){
					log_trace(logKernel, " [CPU %d | PID %d] No se encuentra consola asociada", socket_cpu, pcb->pid);
					t_num exitCode = DESCONEXION_CONSOLA;
					msg_enviar_separado(ERROR, sizeof(t_num), &exitCode, socket_cpu);
				}else{
					msg_enviar_separado(IMPRIMIR_TEXTO, sizeInformacion, datos, info->socket);
					msg_enviar_separado(ESCRIBIR_FD, 0, 0, socket_cpu);
				}

			}else{

				_lockFS();
				log_trace(logKernel, " [CPU %d | PID %d] Escribo en fd %d: %s", socket_cpu, pcb->pid, fdRecibido, datos);

				_cargarEntradasxTabla();

				if(TablaProceso != NULL)
				if(entradaProcesoBuscado != NULL)
				if(entradaGlobalBuscada != NULL){

					if (entradaProcesoBuscado->bandera.escritura == 1){
						t_num sizePath = string_length(entradaGlobalBuscada->FilePath);
						void* buffer = malloc(sizeof(t_num) + sizePath
									   + sizeof(t_valor_variable) + sizeof(t_valor_variable) + sizeInformacion);
						offset = 0;
						tmpsize = 0;
						memcpy(buffer + offset, &sizePath, tmpsize = sizeof(t_num));
						offset += tmpsize;
						memcpy(buffer + offset, entradaGlobalBuscada->FilePath, tmpsize = sizePath);
						offset += tmpsize;
						memcpy(buffer+ offset, &entradaProcesoBuscado->cursor, tmpsize = sizeof(t_valor_variable));
						offset += tmpsize;
						memcpy(buffer+ offset, &sizeInformacion, tmpsize = sizeof(t_valor_variable));
						offset += tmpsize;
						memcpy(buffer+ offset, datos, tmpsize = sizeInformacion);
						offset += tmpsize;

						msg_enviar_separado(GUARDAR_DATOS, offset, buffer, socket_fs);

						t_msg* msgRecibido2 = msg_recibir(socket_fs);

						if(msgRecibido2->tipoMensaje == GUARDAR_DATOS){
							log_trace(logKernel, " [CPU %d | PID %d] Escribio bien", socket_cpu, pcb->pid);
							msg_enviar_separado(ESCRIBIR_FD, 0, 0, socket_cpu);
						}else{
							log_error(logKernel, " [CPU %d | PID %d] Error al escribir", socket_cpu, pcb->pid);
							t_num exitCode = msgRecibido2->tipoMensaje;
							msg_enviar_separado(ERROR, sizeof(t_num), &exitCode, socket_cpu);
						}
						free(buffer);
						msg_destruir(msgRecibido2);

					}else{
						log_error(logKernel, " [CPU %d | PID %d] No tiene permisos de escritura", socket_cpu, pcb->pid);
						t_num exitCode = ESCRITURA_SIN_PERMISOS;
						msg_enviar_separado(ERROR, sizeof(t_num), &exitCode, socket_cpu);
					}
				}
				_unlockFS();
			}
			_sumarCantOpPriv(pcb->pid);
			free(datos);
			//sem_post(&cpuUsada->semaforo);
			break;




		case ABRIR_ANSISOP:
			_lockFS();
			log_trace(logKernel, " [CPU %d | PID %d] Recibi la siguiente operacion ABRIR_ANSISOP de CPU", socket_cpu, pcb->pid);
			t_direccion_archivo pathArchivo;
			t_num longitudPath;
			t_banderas flags;
			t_pid pid;
			offset = 0;
			memcpy(&longitudPath, msgRecibido->data + offset, tmpsize = sizeof(t_num));
			offset += tmpsize;
			pathArchivo = malloc(longitudPath+1);
			memcpy(pathArchivo, msgRecibido->data + offset, tmpsize = longitudPath);
			pathArchivo[longitudPath] = '\0';
			offset += tmpsize;
			memcpy(&flags, msgRecibido->data + offset, tmpsize = sizeof(t_banderas));
			offset += tmpsize;
			memcpy(&pid, msgRecibido->data + offset, tmpsize = sizeof(t_pid));
			offset += tmpsize;

//----------------------- validacion existencia archivo --------------------

			msg_enviar_separado(VALIDAR_ARCHIVO, longitudPath, pathArchivo, socket_fs);
			t_msg* msgValidacionObtenida = msg_recibir(socket_fs);

			bool seguirEjecutando = true;

			if(msgValidacionObtenida->tipoMensaje == VALIDAR_ARCHIVO){
				log_trace(logKernel, "El archivo existe %s", pathArchivo);
			}else{
				if(msgValidacionObtenida->tipoMensaje == ARCHIVO_INEXISTENTE && flags.creacion == true){

					log_trace(logKernel, "Se crea el archivo %s", pathArchivo);
					msg_enviar_separado(CREAR_ARCHIVO, longitudPath, pathArchivo, socket_fs);

					msg_destruir(msgValidacionObtenida);
					msgValidacionObtenida = msg_recibir(socket_fs);

					if(msgValidacionObtenida->tipoMensaje != CREAR_ARCHIVO ){
						log_trace(logKernel, " error al crear el arcihvo: %s", pathArchivo, socket_cpu);
						seguirEjecutando = false;
						t_num exitCode = msgValidacionObtenida->tipoMensaje;
						msg_enviar_separado(ERROR, sizeof(t_num), &exitCode, socket_cpu);
					}else{
						log_trace(logKernel, " se creo el archivo: %s", pathArchivo, socket_cpu);
					}
				}else{
					seguirEjecutando = false;
					t_num exitcode = msgValidacionObtenida->tipoMensaje;
					msg_enviar_separado(ERROR, sizeof(t_num), &exitcode, socket_cpu);
				}
			}

			msg_destruir(msgValidacionObtenida);

//------------------- valido si se puede seguir ejecutando -----------------------
		if(seguirEjecutando){
			// busco si ya tengo este path en la tabla global
			int _buscarPath(t_entrada_GlobalFile* a){ return string_equals_ignore_case(a->FilePath,pathArchivo); }
			t_entrada_GlobalFile* entradaGlobalOld = list_find(lista_tabla_global, (void*) _buscarPath);

			t_descriptor_archivo indiceActualGlobal;

			if(entradaGlobalOld == NULL){
				log_trace(logKernel, " [CPU %d | PID %d] Se abre el archivo por primera vez", socket_cpu, pcb->pid);
				// si es nulo significa qe se abre el archivo por primera vez
				t_entrada_GlobalFile * entradaGlobalNew = malloc(sizeof(t_entrada_GlobalFile));
				entradaGlobalNew->FilePath = malloc(longitudPath + 1);
				memcpy(entradaGlobalNew->FilePath, pathArchivo, longitudPath);
				entradaGlobalNew->FilePath[longitudPath] = '\0';

				entradaGlobalNew->Open = 1;
				entradaGlobalNew->indiceGlobalTable = indiceGlobal; // indiceGlobal = 0 seteado en kernel.c
				indiceActualGlobal = indiceGlobal; //me guardo el indice antes de aumentarlo
				indiceGlobal++;
				list_add(lista_tabla_global, entradaGlobalNew);
			} else {
				// de lo contrario este se encuentra abierto y un proceso esta qeriendo abrirlo
				list_remove_by_condition(lista_tabla_global, (void*) _buscarPath);
				entradaGlobalOld->Open++;
				indiceActualGlobal = entradaGlobalOld->indiceGlobalTable;
				list_add(lista_tabla_global, entradaGlobalOld);
			}

			// seteo variable necesarias para la tabla de procesos
			t_entrada_proceso *entradaProcesoNew = malloc(sizeof(t_entrada_proceso));

			entradaProcesoNew->bandera = flags;
			entradaProcesoNew->referenciaGlobalTable = indiceActualGlobal;
			entradaProcesoNew->cursor = 0; // seteo del cursor



			// busco si ya tengo la tabla para este id de proceso
			int _espid(t_tabla_proceso* a){ return a->pid == pid; }
			pthread_mutex_lock(&mutex_tablaProcesos);
			t_tabla_proceso* tablaProceso = list_remove_by_condition(lista_tabla_de_procesos, (void*) _espid);

			if(tablaProceso == NULL){
				//significa q no existe la tabla buscada => la creo
				tablaProceso = malloc(sizeof(t_tabla_proceso));
				tablaProceso->pid = pid;
				tablaProceso->lista_entradas_de_proceso = list_create();
				tablaProceso->fdMax = 2;
			}
			entradaProcesoNew->fd = tablaProceso->fdMax + 1;
			tablaProceso->fdMax++;

			// en este momento ya estan cargados los flags, el indice y la referencia a tabla global
			list_add(tablaProceso->lista_entradas_de_proceso, entradaProcesoNew);
			list_add(lista_tabla_de_procesos, tablaProceso);
			pthread_mutex_unlock(&mutex_tablaProcesos);

			// le mando a cpu el fd asignado
			msg_enviar_separado(ABRIR_ANSISOP, sizeof(t_descriptor_archivo), &entradaProcesoNew->fd, socket_cpu);

			free(pathArchivo);

		}
		_sumarCantOpPriv(pcb->pid);
		_unlockFS();
		//sem_post(&cpuUsada->semaforo);
		break;




		case BORRAR_ANSISOP:


			_lockFS();
			log_trace(logKernel, " [CPU %d | PID %d] Recibi la siguiente operacion BORRAR_ANSISOP de CPU", socket_cpu, pcb->pid);

			memcpy(&fdRecibido, msgRecibido->data, sizeof(t_descriptor_archivo));
			memcpy(&pidRecibido, msgRecibido->data + sizeof(t_descriptor_archivo), sizeof(t_pid));

			_cargarEntradasxTabla();

			if(TablaProceso != NULL)
			if(entradaProcesoBuscado != NULL)
			if(entradaGlobalBuscada != NULL){
				if(entradaGlobalBuscada->Open == 1){
					msg_enviar_separado(BORRAR, string_length(entradaGlobalBuscada->FilePath), entradaGlobalBuscada->FilePath, socket_fs);
					//list_remove_by_condition(lista_tabla_global, (void*) _esIndiceGlobal);
					//list_remove_by_condition(TablaProceso->lista_entradas_de_proceso, (void*) _esFD);
					
					t_msg* msgRecibidoBorrar = msg_recibir(socket_fs);
					if(msgRecibidoBorrar->tipoMensaje == BORRAR){
						log_trace(logKernel, "[CPU %d | PID %d] Borro correctamente", socket_cpu, pcb->pid);
						msg_enviar_separado(BORRAR,0,0,socket_cpu);
					}else{
						log_error(logKernel, "[CPU %d | PID %d] Error al borrar", socket_cpu, pcb->pid);
						t_num exitcode = msgValidacionObtenida->tipoMensaje;
						msg_enviar_separado(ERROR, sizeof(t_num), &exitcode, socket_cpu);
					}
					msg_destruir(msgRecibidoBorrar);
					
					
				}else{
					log_trace(logKernel, " [CPU %d | PID %d] el archivo esta siendo usado por %d procesos, no se puede borrar", socket_cpu, entradaGlobalBuscada->Open);
					msg_enviar_separado(ERROR,0,0,socket_cpu);
				}
			}
			_sumarCantOpPriv(pcb->pid);
			_unlockFS();
			//sem_post(&cpuUsada->semaforo);
			break;



		case CERRAR_ANSISOP:

			log_trace(logKernel, "[CPU %d | PID %d] Recibi la siguiente operacion CERRAR_ANSISOP de CPU", socket_cpu, pcb->pid);

			memcpy(&fdRecibido, msgRecibido->data, sizeof(t_descriptor_archivo));
			memcpy(&pidRecibido, msgRecibido->data + sizeof(t_descriptor_archivo), sizeof(t_pid));

			_cargarEntradasxTabla();

			if(TablaProceso != NULL)
			if(entradaProcesoBuscado != NULL)
			if(entradaGlobalBuscada != NULL){
				pthread_mutex_lock(&mutex_tablaGlobal);
				entradaGlobalBuscada = list_remove_by_condition(lista_tabla_global, (void*) _buscarPorIndice);
				if(entradaGlobalBuscada == NULL){
					log_trace(logKernel, "No encontre entrada");
					msg_enviar_separado(ERROR,0,0,socket_cpu);
				}else{
					// saco la entrada global buscada
					entradaGlobalBuscada->Open --;

					//int _esFD(t_entrada_proceso *entradaP){return entradaP->indice == entradaProcesoBuscado->indice;}
					list_remove_by_condition(TablaProceso->lista_entradas_de_proceso, (void*) _esFD);
					if(entradaGlobalBuscada->Open == 0){
						// no hago nada ya que la saque antes
						free(entradaGlobalBuscada);
					} else {
						// vuelvo a meter la entrada que modifique en la tabla global
						list_add(lista_tabla_global, entradaGlobalBuscada);
					}
					msg_enviar_separado(CERRAR_ANSISOP,0,0,socket_cpu);
				}
				pthread_mutex_unlock(&mutex_tablaGlobal);
			}

			_sumarCantOpPriv(pcb->pid);
			//sem_post(&cpuUsada->semaforo);
			break;
		} //end switch

		free(varCompartida);
		free(varSemaforo);
		msg_destruir(msgRecibido);
	}
}



typedef struct {
	t_pid pid;
	t_num size;
	char* script;
}_t_hiloEspera;

void enviarScriptAMemoria(_t_hiloEspera* aux){

	log_trace(logKernel, "Envio script a memoria");

	int socketConsola;
	bool _buscarPCB(t_PCB* pcb){
		return pcb->pid == aux->pid;
	}
	void _buscarConsola(t_infosocket* a){
		if(a->pidPCB == aux->pid)
			socketConsola = a->socket;
	}

	_lockLista_PCB_consola();
	list_iterate(lista_PCB_consola, (void*) _buscarConsola);
	_unlockLista_PCB_consola();

	sem_wait(&sem_gradoMp);
	_sacarDeCola(aux->pid, cola_New, mutex_New);

	_lockLista_PCBs();
	t_PCB* pcb = list_find(lista_PCBs, (void*) _buscarPCB);
	_unlockLista_PCBs();

	_lockMemoria();
	send(socket_memoria, &aux->pid, sizeof(t_pid), 0);
	msg_enviar_separado(INICIALIZAR_PROGRAMA, aux->size, aux->script, socket_memoria);
	send(socket_memoria, &pcb->cantPagsStack, sizeof(t_num16), 0);
	t_num8 respuesta;
	recv(socket_memoria, &respuesta, sizeof(t_num8), 0);
	_unlockMemoria();

	switch(respuesta){
	case OK:
		log_trace(logKernel, "Recibi OK de memoria");
		pcb->cantPagsCodigo = (string_length(aux->script) / tamanioPag);
		pcb->cantPagsCodigo = (string_length(aux->script) % tamanioPag) == 0? pcb->cantPagsCodigo: pcb->cantPagsCodigo + 1;
		pcb->sp = pcb->cantPagsCodigo * tamanioPag;
		msg_enviar_separado(respuesta, sizeof(t_pid), &pcb->pid, socketConsola);

		_lockLista_infoProc();
		t_infoProceso* infP = malloc(sizeof(t_infoProceso));
		bzero(infP, sizeof(t_infoProceso));
		infP->pid = aux->pid;
		list_add(infoProcs, infP);
		_unlockLista_infoProc();

		_ponerEnCola(aux->pid, cola_Ready, mutex_Ready);
		sem_post(&sem_cantColaReady);
		break;
	case MARCOS_INSUFICIENTES:
		log_trace(logKernel, "MARCOS INSUFICIENTES");
		msg_enviar_separado(MARCOS_INSUFICIENTES, 0, 0, socketConsola);
		setearExitCode(pcb->pid, RECURSOS_INSUFICIENTES);
		break;
	default:
		log_trace(logKernel, "RECIBI cualquier cosa %d", respuesta);
	}
	free(aux->script);
	free(aux);
}

void atender_consola(int socket_consola){

	t_msg* msgRecibido = msg_recibir(socket_consola);
	if(msgRecibido->longitud > 0)
		if(msg_recibir_data(socket_consola, msgRecibido) <= 0)
			msgRecibido->tipoMensaje = 0;

	void* script;	//si lo declaro adentro del switch se queja
	void _finalizarPIDs(t_infosocket* i){
		if(i->socket == socket_consola){
			int pidAux = i->pidPCB;
			finalizarPid(pidAux);
			setearExitCode(pidAux, DESCONEXION_CONSOLA);
		}
	}


	switch(msgRecibido->tipoMensaje){

	case ENVIO_CODIGO:
		log_trace(logKernel, "Recibi script de consola %d", socket_consola);
		pid++;
		script = malloc(msgRecibido->longitud);
		memcpy(script, msgRecibido->data, msgRecibido->longitud);
		log_info(logKernel, "\n%s", script);

		t_pid pidActual = crearPCB(socket_consola, script);

		_lockLista_PCB_consola();
		t_infosocket* info = malloc(sizeof(t_infosocket));
		info->pidPCB = pidActual;
		info->socket = socket_consola;
		list_add(lista_PCB_consola, info);
		_unlockLista_PCB_consola();

		_ponerEnCola(pidActual, cola_New, mutex_New);

		_t_hiloEspera* aux = malloc(sizeof(_t_hiloEspera));
		aux->pid = pidActual;
		aux->script = malloc(msgRecibido->longitud);
		aux->size = msgRecibido->longitud;
		memcpy(aux->script, script, msgRecibido->longitud);

		pthread_attr_t atributo;
		pthread_attr_init(&atributo);
		pthread_attr_setdetachstate(&atributo, PTHREAD_CREATE_DETACHED);
		pthread_t hiloEspera;
		pthread_create(&hiloEspera, &atributo,(void*) enviarScriptAMemoria, aux);
		pthread_attr_destroy(&atributo);

		free(script);
		break;

	case FINALIZAR_PROGRAMA:
		//msg_recibir_data(socket_consola, msgRecibido);
		log_trace(logKernel, "Recibi FINALIZAR_PROGRAMA de consola %d", socket_consola);
		t_pid pidAFinalizar;
		memcpy(&pidAFinalizar, msgRecibido->data, sizeof(t_pid));
		log_trace(logKernel, "FINALIZAR_PROGRAMA pid %d", pidAFinalizar);
		finalizarPid(pidAFinalizar);
		setearExitCode(pidAFinalizar, COMANDO_FINALIZAR);
		break;

	case 0: case DESCONECTAR:
		fprintf(stderr, PRINT_COLOR_YELLOW "La consola %d se ha desconectado" PRINT_COLOR_RESET "\n", socket_consola);
		log_trace(logKernel, "Recibi DESCONECTAR de consola %d", socket_consola);
		_lockLista_PCB_consola();
		list_iterate(lista_PCB_consola, (void*) _finalizarPIDs);
		_unlockLista_PCB_consola();

		//si la consola se desconecto la saco de la lista
		bool _esConsola(int socketC){ return socketC == socket_consola; }
		_lockLista_Consolas();
		list_remove_and_destroy_by_condition(lista_consolas, (void*) _esConsola, free);
		_unlockLista_Consolas();
		close(socket_consola);
		FD_CLR(socket_consola, &fdsMaestro);
		break;

	}

	msg_destruir(msgRecibido);

}






void consolaKernel(){

	/*
	 * Faltan implementar solo estos:
	2. Obtener para un proceso dado:
		c. Obtener la tabla de archivos abiertos por el proceso.
	3. Obtener la tabla global de archivos.
	*/

	while(1){
		char* comando = malloc(200);
		fgets(comando, 200, stdin);

		comando[strlen(comando)-1] = '\0';
		log_trace(logKernel, "comando ingresado: %s", comando);

		if(string_equals_ignore_case(comando, "list procs")){
			printf("Mostrar todos? (s/n) ");
			fgets(comando, 200, stdin);
			comando[strlen(comando)-1] = '\0';
			int i;

			if(string_starts_with(comando, "s") || string_starts_with(comando, "n")){
				printf(PRINT_COLOR_CYAN "   ····  PID  ····   PC  ···· CantPags ···· ExitCode ····   Cola   ···· " PRINT_COLOR_RESET "\n");
				_lockLista_PCBs();
				for(i = 0; i < list_size(lista_PCBs); i++){
					t_PCB* pcbA = list_get(lista_PCBs, i);
					char* cola = " -- ";
					char* exitCode;
					int cantPags = pcbA->cantPagsCodigo + pcbA->cantPagsStack + pcbA->cantPagsHeap;
					if(cantPags == stackSize)
						cantPags = 0;

					if(pcbA->exitCode == SIN_ASIGNAR)
						exitCode = " - ";
					else if(pcbA->exitCode == FINALIZO_CORRECTAMENTE)
						exitCode = "0";
					else
						exitCode = string_from_format("-%d", pcbA->exitCode);
					if(_estaEnCola(pcbA->pid, cola_New, mutex_New))
						cola = "NEW";
					else if(_estaEnCola(pcbA->pid, cola_Ready, mutex_Ready))
						cola = "READY";
					else if(_estaEnCola(pcbA->pid, cola_Exec, mutex_Exec))
						cola = "EXEC";
					else if(_estaEnCola(pcbA->pid, cola_Block, mutex_Block))
						cola = "BLOCK";
					else if(_estaEnCola(pcbA->pid, cola_Exit, mutex_Exit))
						cola = "EXIT";

					if(string_starts_with(comando, "s") || !string_equals_ignore_case(cola, " -- "))
						printf("   ····   %2d  ····  %3d  ····   %3d    ····    %3s   ····  %5s   ···· \n",
							pcbA->pid, pcbA->pc, cantPags, exitCode, cola);
				}
				_unlockLista_PCBs();

			}else printf("Flasheaste era s/n \n");

		}else if(string_starts_with(comando, "info ")){

			int pidPCB = atoi(string_substring_from(comando, 5));

			int _espid(t_infoProceso* a){ return a->pid == pidPCB; }
			_lockLista_infoProc();
			t_infoProceso* infP = list_find(infoProcs, (void*) _espid);
			if(infP == NULL)
				fprintf(stderr, PRINT_COLOR_YELLOW "No existe el pid %d" PRINT_COLOR_RESET "\n", pidPCB);
			else{

				printf("Elegir opcion: \n"
						PRINT_COLOR_BLUE "  a -" PRINT_COLOR_RESET " Cantidad de rafagas ejecutadas \n"
						PRINT_COLOR_BLUE "  b -" PRINT_COLOR_RESET " Cantidad de operaciones privilegiadas que ejecutó \n"
						PRINT_COLOR_BLUE "  c -" PRINT_COLOR_RESET " Tabla de archivos abiertos por el proceso \n"
						PRINT_COLOR_BLUE "  d -" PRINT_COLOR_RESET " Cantidad de páginas de Heap utilizadas \n"
						PRINT_COLOR_BLUE "  e -" PRINT_COLOR_RESET " Cantidad de acciones alocar/liberar realizadas \n");

				switch(getchar()){
				case 'a':
					printf( "Cantidad de rafagas ejecutadas: %d \n", infP->cantRafagas);
					break;
				case 'b':
					printf( "Cantidad de operaciones privilegiadas que ejecutó: %d \n", infP->cantOpPriv);
					break;
				case 'c':

					printf( PRINT_COLOR_RESET );
					int fdActual;
					int _buscarPid(t_tabla_proceso* tabla){ return pidPCB == tabla->pid; }
					int _buscarFD(t_entrada_proceso* entradaP){ return fdActual == entradaP->fd; }

					printf(PRINT_COLOR_CYAN "   ····    FD    ····   Flags  ···· GlobalFD ···· " PRINT_COLOR_RESET "\n");

					t_tabla_proceso* tablaProceso = list_find(lista_tabla_de_procesos, (void*) _buscarPid);
					if(tablaProceso != NULL){
						for(fdActual = 3; fdActual <= tablaProceso->fdMax; fdActual++){

							t_entrada_proceso* entradaProcesoBuscado = list_find(tablaProceso->lista_entradas_de_proceso, (void*) _buscarFD);
							if(entradaProcesoBuscado != NULL){

								char* flags = string_new();
								if(entradaProcesoBuscado->bandera.lectura)
									string_append(&flags,"r");
								if(entradaProcesoBuscado->bandera.escritura)
									string_append(&flags,"w");
								if(entradaProcesoBuscado->bandera.creacion)
									string_append(&flags,"c");

								printf("   ····    %2d    ····    %3s   ····   %3d    ····  \n",  fdActual, flags, entradaProcesoBuscado->referenciaGlobalTable);

							}
						}
					}


					break;
				case 'd':
					printf( "Cantidad de páginas de Heap utilizadas: %d \n", infP->cantPagsHeap);
					break;
				case 'e':
					printf( "Cantidad de operaciones alocar: %d \n", infP->cantOp_alocar);
					printf( "Se alocaron %d bytes \n", infP->canrBytes_alocar);
					printf( "Cantidad de operaciones liberar: %d \n", infP->cantOp_liberar);
					printf( "Se liberaron %d bytes \n", infP->canrBytes_liberar);
					if(infP->canrBytes_alocar > infP->canrBytes_liberar)
						printf( "No se liberaron todas las estructuras del heap \n");
					break;
				default:
					fprintf(stderr, PRINT_COLOR_YELLOW "Capo que me tiraste?" PRINT_COLOR_RESET "\n");
				}

			}
			_unlockLista_infoProc();
		}else if(string_equals_ignore_case(comando, "tabla archivos")){

			int maxPID = 0;
			void _maxPID(t_PCB* p){ if(p->pid > maxPID) maxPID = p->pid; }
			_lockLista_PCBs();
			list_iterate(lista_PCBs, (void*) _maxPID);
			_unlockLista_PCBs();

			int fdActual, pidActual;
			int _buscarPid(t_tabla_proceso* tabla){ return pidActual == tabla->pid; }
			int _buscarFD(t_entrada_proceso* entradaP){ return fdActual == entradaP->fd; }

			printf(PRINT_COLOR_CYAN "   ····  Indice  ····   Open   ····      File " PRINT_COLOR_RESET "\n");

			for(pidActual = 1; pidActual <= maxPID; pidActual++){
				t_tabla_proceso* tablaProceso = list_find(lista_tabla_de_procesos, (void*) _buscarPid);
				if(tablaProceso != NULL){
					for(fdActual = 3; fdActual <= tablaProceso->fdMax; fdActual++){

						t_entrada_proceso* entradaProcesoBuscado = list_find(tablaProceso->lista_entradas_de_proceso, (void*) _buscarFD);
						if(entradaProcesoBuscado != NULL){

							int _buscarPorIndice(t_entrada_GlobalFile* entradaGlobal){return entradaGlobal->indiceGlobalTable == entradaProcesoBuscado->referenciaGlobalTable;}
							t_entrada_GlobalFile* entradaGlobalBuscada = list_find(lista_tabla_global, (void*) _buscarPorIndice);

							if(entradaGlobalBuscada != NULL)
								printf("   ····   %3d    ····   %3d    ····   %s \n",
									entradaGlobalBuscada->indiceGlobalTable, entradaGlobalBuscada->Open, entradaGlobalBuscada->FilePath);

						}
					}
				}
			}


		}else if(string_starts_with(comando, "grado mp ")){

			int valorSem, nuevoGMP, enEjecucion, i;
			nuevoGMP = atoi(string_substring_from(comando, 9));
			sem_getvalue(&sem_gradoMp, &valorSem);
			enEjecucion = gradoMultiprogramacion-valorSem;

			log_trace(logKernel, "Cambio grado multiprogramacion, anterior %d enEjecucion %d nuevo %d",
					gradoMultiprogramacion, enEjecucion, nuevoGMP);

			void _esperarGradoMP(){
				log_trace(logKernel, "Bajo grado de MP");
				sem_wait(&sem_gradoMp);
				log_trace(logKernel, "Lo baje :)");
			}

			if(nuevoGMP < gradoMultiprogramacion && nuevoGMP < valorSem){
				log_trace(logKernel, "Waiteo gradoMP");
				pthread_attr_t atributo;
				pthread_attr_init(&atributo);
				pthread_attr_setdetachstate(&atributo, PTHREAD_CREATE_DETACHED);
				pthread_t hiloEspera;
				for(i = 0; i < valorSem - nuevoGMP; i++){
					pthread_create(&hiloEspera, &atributo,(void*) _esperarGradoMP, NULL);
				}
				pthread_attr_destroy(&atributo);

			}else{
				if(nuevoGMP < enEjecucion){
					log_trace(logKernel, "Waiteo gradoMP_2");
					pthread_attr_t atributo;
					pthread_attr_init(&atributo);
					pthread_attr_setdetachstate(&atributo, PTHREAD_CREATE_DETACHED);
					pthread_t hiloEspera;
					for(i = 0; i < enEjecucion - nuevoGMP ; i++){
						pthread_create(&hiloEspera, &atributo,(void*) _esperarGradoMP, NULL);
					}
					pthread_attr_destroy(&atributo);
				}else{
					log_trace(logKernel, "Posteo gradoMP");
					for(i = 0; i < nuevoGMP - valorSem; i++){
						log_trace(logKernel, "Subo grado de MP");
						sem_post(&sem_gradoMp);
					}
				}
				gradoMultiprogramacion = nuevoGMP;
				fprintf(stderr, PRINT_COLOR_BLUE "Nuevo grado de multiprogramacion: %d" PRINT_COLOR_RESET "\n", gradoMultiprogramacion);
			}

		}else if(string_starts_with(comando, "kill ")){

			int pidKill = atoi(string_substring_from(comando, 5));

			int _es_PCB(t_PCB* p){
				return p->pid == pidKill;
			}
			_lockLista_PCBs();
			t_PCB* pcbKill = list_find(lista_PCBs, (void*) _es_PCB);
			_unlockLista_PCBs();
			if(pcbKill == NULL)
				fprintf(stderr, PRINT_COLOR_YELLOW "No existe el PID %d" PRINT_COLOR_RESET "\n", pidKill);
			else{
				if(pcbKill->exitCode != SIN_ASIGNAR){
					log_trace(logKernel,  "El PID %d ya finalizo", pidKill);
					fprintf(stderr, PRINT_COLOR_YELLOW "El PID %d ya finalizo" PRINT_COLOR_RESET "\n", pidKill);
				}else{
					finalizarPid(pidKill);
					setearExitCode(pidKill, COMANDO_FINALIZAR);
					fprintf(stderr, PRINT_COLOR_BLUE "Finalizo PID %d" PRINT_COLOR_RESET "\n", pidKill);
				}
			}

		}else if(string_equals_ignore_case(comando, "variables")){

			printf(PRINT_COLOR_CYAN "   ····  VariableCompartida  ····   Valor   ···· " PRINT_COLOR_RESET "\n");
			int i, sizelista = list_size(lista_variablesCompartidas);
			for(i=0;i < sizelista; i++){
				t_VariableCompartida* v = list_get(lista_variablesCompartidas, i);
				printf( "   ····  %18s  ····    %3d    ···· \n", v->nombre, v->valor);
			}

			printf(PRINT_COLOR_CYAN "   ····       Semaforo       ····   Valor   ···· " PRINT_COLOR_RESET "\n");
			sizelista = list_size(lista_variablesSemaforo);
			for(i=0;i < sizelista; i++){
				t_VariableSemaforo* s = list_get(lista_variablesSemaforo, i);
				printf( "   ····  %18s  ····    %3d    ···· \n", s->nombre, s->valorSemaforo);
			}

		}else if(string_equals_ignore_case(comando, "detener")){

			log_trace(logKernel, "Detengo planificacion");
			pthread_mutex_lock(&mut_detengo_plani);

		}else if(string_equals_ignore_case(comando, "reanudar")){

			log_trace(logKernel, "Reanudo planificacion");
			pthread_mutex_unlock(&mut_detengo_plani);

		}else if(string_equals_ignore_case(comando, "exitcodes")){

			printf("Exit Codes:\n"
					PRINT_COLOR_BLUE "  ○ " PRINT_COLOR_CYAN "  0 " PRINT_COLOR_RESET "El programa finalizó correctamente\n"
					PRINT_COLOR_BLUE "  ○ " PRINT_COLOR_CYAN " -1 " PRINT_COLOR_RESET "No se pudieron reservar recursos para ejecutar el programa\n"
					PRINT_COLOR_BLUE "  ○ " PRINT_COLOR_CYAN " -2 " PRINT_COLOR_RESET "El programa intentó acceder a un archivo que no existe\n"
					PRINT_COLOR_BLUE "  ○ " PRINT_COLOR_CYAN " -3 " PRINT_COLOR_RESET "El programa intentó leer un archivo sin permisos\n"
					PRINT_COLOR_BLUE "  ○ " PRINT_COLOR_CYAN " -4 " PRINT_COLOR_RESET "El programa intentó escribir un archivo sin permisos\n"
					PRINT_COLOR_BLUE "  ○ " PRINT_COLOR_CYAN " -5 " PRINT_COLOR_RESET "Excepción de memoria\n"
					PRINT_COLOR_BLUE "  ○ " PRINT_COLOR_CYAN " -6 " PRINT_COLOR_RESET "Finalizado a través de desconexión de consola\n"
					PRINT_COLOR_BLUE "  ○ " PRINT_COLOR_CYAN " -7 " PRINT_COLOR_RESET "Finalizado a través del comando Finalizar Programa de la consola\n"
					PRINT_COLOR_BLUE "  ○ " PRINT_COLOR_CYAN " -8 " PRINT_COLOR_RESET "Se intentó reservar más memoria que el tamaño de una página\n"
					PRINT_COLOR_BLUE "  ○ " PRINT_COLOR_CYAN " -9 " PRINT_COLOR_RESET "No se pueden asignar más páginas al proceso\n"
					PRINT_COLOR_BLUE "  ○ " PRINT_COLOR_CYAN "-11 " PRINT_COLOR_RESET "Error de sintaxis del script\n"
					PRINT_COLOR_BLUE "  ○ " PRINT_COLOR_CYAN "-12 " PRINT_COLOR_RESET "StackOverflow\n"
					PRINT_COLOR_BLUE "  ○ " PRINT_COLOR_CYAN "-13 " PRINT_COLOR_RESET "Se intento acceder a un semaforo inexistente\n"
					PRINT_COLOR_BLUE "  ○ " PRINT_COLOR_CYAN "-14 " PRINT_COLOR_RESET "Se intento acceder a una variable compartida inexistente\n"
					PRINT_COLOR_BLUE "  ○ " PRINT_COLOR_CYAN "-15 " PRINT_COLOR_RESET "No hay bloques libres en el FileSystem\n"
					PRINT_COLOR_BLUE "  ○ " PRINT_COLOR_CYAN "-20 " PRINT_COLOR_RESET "Error sin definición\n");

		}else if(string_equals_ignore_case(comando, "help")){
			printf("Comandos:\n"
					PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "list procs: " PRINT_COLOR_RESET "Obtener listado de procesos\n"
					PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "info [pid]: " PRINT_COLOR_RESET "Obtener informacion de proceso\n"
					PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "tabla archivos: " PRINT_COLOR_RESET "Obtener tabla global de archivos\n"
					PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "grado mp [grado]: " PRINT_COLOR_RESET "Setear grado de multiprogramacion\n"
					PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "kill [pid]: " PRINT_COLOR_RESET "Finalizar proceso\n"
					PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "detener: " PRINT_COLOR_RESET "Detener la planificación\n"
					PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "reanudar: " PRINT_COLOR_RESET "Reanuda la planificación\n"
					PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "variables: " PRINT_COLOR_RESET "Muestra los valores actuales de las variables\n"
					PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "exitcodes: " PRINT_COLOR_RESET "Listado de exit codes\n");

		}else if(string_equals_ignore_case(comando, "logCancer")){
			flag_logCancer = !flag_logCancer;
		}else if(string_equals_ignore_case(comando, "\0")){
		}else
			fprintf(stderr, PRINT_COLOR_YELLOW "El comando '%s' no es valido" PRINT_COLOR_RESET "\n", comando);

		free(comando);
	}
}




void terminarKernel(){			//aca libero todos

	list_destroy(lista_consolas);
	list_destroy_and_destroy_elements(lista_cpus, free);
	list_destroy_and_destroy_elements(lista_PCB_consola, (void*) free);
	list_destroy_and_destroy_elements(lista_PCB_cpu, (void*) free);

	void _freePCBs(t_PCB* pcb){ liberarPCB(pcb, false); }
	list_destroy_and_destroy_elements(lista_PCBs, (void*) _freePCBs);

	queue_destroy_and_destroy_elements(cola_New, (void*) free);
	queue_destroy_and_destroy_elements(cola_Ready, (void*) free);
	queue_destroy_and_destroy_elements(cola_Exec, (void*) free);
	queue_destroy_and_destroy_elements(cola_Block, (void*) free);
	queue_destroy_and_destroy_elements(cola_Exit, (void*) free);

	list_destroy_and_destroy_elements(lista_variablesCompartidas, free);
	void _destruirSemaforos(t_VariableSemaforo* variable){
		queue_destroy_and_destroy_elements(variable->colaBloqueados, free);
		free(variable);
	}
	list_destroy_and_destroy_elements(lista_variablesSemaforo, (void*) _destruirSemaforos);

	FD_ZERO(&fdsMaestro);

	log_trace(logKernel, "Terminando Proceso");
	log_destroy(logKernel);
	exit(1);
}



void finalizarPid(t_pid pid){
	log_trace(logKernel, "Finalizando pid %d", pid);
	int _buscarPID(t_infosocket* i){
		return i->pidPCB == pid;
	}

	t_infosocket* info;
	_lockLista_PCB_cpu();
	info = list_find(lista_PCB_cpu, (void*) _buscarPID);
	_unlockLista_PCB_cpu();
	if(info == NULL){
		log_info(logKernel, "No hay cpu ejecutando pid %d", pid);
		
		if(_sacarDeCola(pid, cola_Block, mutex_Block) == pid){
			_ponerEnCola(pid, cola_Ready, mutex_Ready);
			sem_post(&sem_cantColaReady);
		}else{
			if(_sacarDeCola(pid, cola_New, mutex_New) == pid){
				_ponerEnCola(pid, cola_Ready, mutex_Ready);
				sem_post(&sem_cantColaReady);
			}
		}
		_ponerEnCola(pid, cola_Exit, mutex_Exit);

		pthread_mutex_lock(&mutex_listaSemaforos);
		void _sacarDeSemaforos(t_VariableSemaforo* sem){
			log_trace(logKernel, "Pre sem %s", sem->nombre);
			t_pid encontrado = _sacarDeCola(pid, sem->colaBloqueados, sem->mutex_colaBloqueados);
			if(encontrado == pid)
				sem->valorSemaforo++;
		}
		list_iterate(lista_variablesSemaforo, (void*) _sacarDeSemaforos);
		pthread_mutex_unlock(&mutex_listaSemaforos);
	}

	log_trace(logKernel, "Fin Finalizando pid %d", pid);
}

void _lockLista_cpus(){
	pthread_mutex_lock(&mutex_cpus);
}
void _unlockLista_cpus(){
	pthread_mutex_unlock(&mutex_cpus);
}
void _lockLista_Consolas(){
	pthread_mutex_lock(&mutex_consolas);
}
void _unlockLista_Consolas(){
	pthread_mutex_unlock(&mutex_consolas);
}
void _lockLista_PCBs(){
	pthread_mutex_lock(&mutex_PCBs);
}
void _unlockLista_PCBs(){
	pthread_mutex_unlock(&mutex_PCBs);
}
void _lockLista_PCB_consola(){
	pthread_mutex_lock(&mutex_PCB_consola);
}
void _unlockLista_PCB_consola(){
	pthread_mutex_unlock(&mutex_PCB_consola);
}
void _lockLista_PCB_cpu(){
	pthread_mutex_lock(&mutex_PCB_cpu);
}
void _unlockLista_PCB_cpu(){
	pthread_mutex_unlock(&mutex_PCB_cpu);
}
void _lockLista_infoProc(){
	pthread_mutex_lock(&mutex_infoProcs);
}
void _unlockLista_infoProc(){
	pthread_mutex_unlock(&mutex_infoProcs);
}

void _lockMemoria(){
	pthread_mutex_lock(&mutex_Solicitud_Memoria);
}
void _unlockMemoria(){
	pthread_mutex_unlock(&mutex_Solicitud_Memoria);
}

void _lockFS(){
	pthread_mutex_lock(&mutex_Solicitud_FS);
}
void _unlockFS(){
	pthread_mutex_unlock(&mutex_Solicitud_FS);
}








int asignarNuevaPag(t_PCB* pcb, int socket_cpu, bool flag_es_primera){
	int _esHeap(t_heap* h){
		return h->pid == pcb->pid;
	}
	int nroPag = 0;
	void _buscarMayorNumPag(t_heap* h){
		if(h->pid == pcb->pid && nroPag < h->nroPag)
			nroPag = h->nroPag;
	}

	log_trace(logKernel, " [CPU %d | PID %d] Asigno nueva pagina a proceso", socket_cpu, pcb->pid);
	_lockMemoria();
	send(socket_memoria, &pcb->pid, sizeof(t_pid), 0);
	msg_enviar_separado(ASIGNACION_MEMORIA, 0, 0, socket_memoria);
	t_msg* msgRecibido = msg_recibir(socket_memoria);
	if(msgRecibido->tipoMensaje != ASIGNACION_MEMORIA){
		_unlockMemoria();
		if(msgRecibido->tipoMensaje == PAGINA_INVALIDA){
			log_error(logKernel, " [CPU %d | PID %d] No hay mas pagina libres", socket_cpu, pcb->pid);
			msg_destruir(msgRecibido);
			return -CANT_PAGS_INSUFICIENTES;
		}else{
			log_error(logKernel, " [CPU %d | PID %d] No recibi ASIGNACION_MEMORIA sino %d", socket_cpu, pcb->pid, msgRecibido->tipoMensaje);
			msg_destruir(msgRecibido);
			return -ERROR_EXCEPCION_MEMORIA;
		}
	}
	msg_destruir(msgRecibido);
	_unlockMemoria();

	t_heap* nuevaHeap = malloc(sizeof(t_heap));
	nuevaHeap->pid = pcb->pid;
	if(flag_es_primera)
		nuevaHeap->nroPag = 0;
	else{
		list_iterate(tabla_heap, (void*) _buscarMayorNumPag);
		nuevaHeap->nroPag = nroPag + 1;
	}

	log_trace(logKernel, " [CPU %d | PID %d] Nueva pag numero %d", socket_cpu, pcb->pid, nuevaHeap->nroPag);

	t_posicion puntero;
	puntero.pagina = pcb->cantPagsCodigo + pcb->cantPagsStack + nuevaHeap->nroPag;
	puntero.offset = 0;
	puntero.size = sizeof(t_HeapMetadata);

	t_HeapMetadata metadata;
	metadata.size = tamanioPag - sizeof(t_HeapMetadata);
	metadata.isFree = true;

	void* buffer = malloc(sizeof(t_posicion) + sizeof(t_HeapMetadata));
	memcpy(buffer, &puntero, sizeof(t_posicion));
	memcpy(buffer + sizeof(t_posicion), &metadata, sizeof(t_HeapMetadata));

	_lockMemoria();
	send(socket_memoria, &pcb->pid, sizeof(t_pid), 0);
	msg_enviar_separado(ESCRITURA_PAGINA, sizeof(t_posicion) + sizeof(t_HeapMetadata), buffer, socket_memoria);
	free(buffer);
	msgRecibido = msg_recibir(socket_memoria);
	if(msgRecibido->tipoMensaje != ESCRITURA_PAGINA){
		log_error(logKernel, " [CPU %d | PID %d] No recibi ESCRITURA_PAGINA sino %d", socket_cpu, pcb->pid, msgRecibido->tipoMensaje);
		msg_destruir(msgRecibido);
		_unlockMemoria();
		return -ERROR_EXCEPCION_MEMORIA;
	}
	msg_destruir(msgRecibido);
	_unlockMemoria();

	_sumarCantPagsHeap(pcb->pid, 1);

	list_add(tabla_heap, nuevaHeap);
	return 0;
}




// si hay error en esta funcion devuelve el exitCode
t_puntero encontrarHueco(t_num cantBytes, t_PCB* pcb, int socket_cpu){

	int _esHeap(t_heap* h){
		return h->pid == pcb->pid;
	}
	bool _menorPag(t_heap* h1, t_heap* h2){
		return h1->nroPag <= h2->nroPag;
	}

	t_list* listAux = list_filter(tabla_heap, (void*) _esHeap);
	list_sort(listAux, (void*) _menorPag);

	void* buffer;
	t_posicion punteroAnterior;
	t_HeapMetadata heapMetadataAnterior = {0, 0};
	int i=0, j=0, offset=0, tmpsize;

	for(; i < list_size(listAux); i++){
		t_heap* h = list_get(listAux, i);
		if(h->nroPag == i){
			j = 0;
			while(j < tamanioPag){
				//envio solicitud
				t_posicion puntero;
				puntero.pagina = pcb->cantPagsCodigo + pcb->cantPagsStack + h->nroPag;
				puntero.offset = j;
				puntero.size = sizeof(t_HeapMetadata);
				_lockMemoria();
				send(socket_memoria, &pcb->pid, sizeof(t_pid), 0);
				msg_enviar_separado(LECTURA_PAGINA, sizeof(t_posicion), &puntero, socket_memoria);

				//recibo
				t_msg* msgRecibido = msg_recibir(socket_memoria);
				if(msgRecibido->tipoMensaje != LECTURA_PAGINA){
					log_error(logKernel, " [CPU %d | PID %d] No recibi LECTURA_PAGINA sino %d", socket_cpu, pcb->pid, msgRecibido->tipoMensaje);
					if(msgRecibido->tipoMensaje == EXCEPCION_MEMORIA || msgRecibido->tipoMensaje == PAGINA_INVALIDA)
						return ERROR_EXCEPCION_MEMORIA;
					if(msgRecibido->tipoMensaje == STACKOVERFLOW)
						return ERROR_STACKOVERFLOW;
					else
						return SIN_DEFINICION;
				}
				msg_recibir_data(socket_memoria, msgRecibido);
				t_HeapMetadata heapMetadata;
				memcpy(&heapMetadata, msgRecibido->data, sizeof(t_HeapMetadata));
				if(flag_logCancer)
				log_trace(logKernel, " [CPU %d | PID %d] 	busco hueco heapMetadata -> size %d free %d", socket_cpu, pcb->pid, heapMetadata.size, heapMetadata.isFree);
				msg_destruir(msgRecibido);
				_unlockMemoria();

				if(heapMetadataAnterior.isFree && heapMetadata.isFree && punteroAnterior.pagina == puntero.pagina){
					puntero.offset = punteroAnterior.offset;
					heapMetadata.size = heapMetadata.size + sizeof(t_HeapMetadata) + heapMetadataAnterior.size;

					buffer = malloc(sizeof(t_posicion) + sizeof(t_HeapMetadata));
					memcpy(buffer + offset, &puntero, tmpsize = sizeof(t_posicion));
					offset += tmpsize;
					memcpy(buffer + offset, &heapMetadata, tmpsize = sizeof(t_HeapMetadata));
					offset += tmpsize;

					_lockMemoria();
					send(socket_memoria, &pcb->pid, sizeof(t_pid), 0);
					msg_enviar_separado(ESCRITURA_PAGINA, offset, buffer, socket_memoria);
					offset = 0;

					t_msg* msgRecibido3 = msg_recibir(socket_memoria);
					if(msgRecibido3->tipoMensaje != ESCRITURA_PAGINA){
						log_error(logKernel, " [CPU %d | PID %d] No recibi ESCRITURA_PAGINA sino %d", socket_cpu, pcb->pid, msgRecibido3->tipoMensaje);
						if(msgRecibido->tipoMensaje == EXCEPCION_MEMORIA || msgRecibido->tipoMensaje == PAGINA_INVALIDA)
							return ERROR_EXCEPCION_MEMORIA;
						if(msgRecibido->tipoMensaje == STACKOVERFLOW)
							return ERROR_STACKOVERFLOW;
						else
							return SIN_DEFINICION;
					}
					msg_destruir(msgRecibido3);

					_unlockMemoria();
					free(buffer);
				}


				if(heapMetadata.isFree && cantBytes <= heapMetadata.size ){
					log_trace(logKernel, " [CPU %d | PID %d] Encontre hueco", socket_cpu, pcb->pid);
					t_HeapMetadata heapMetadataProx;
					heapMetadataProx.isFree = true;
					heapMetadataProx.size = heapMetadata.size - cantBytes - sizeof(t_HeapMetadata);
					heapMetadata.isFree = false;
					heapMetadata.size = cantBytes;

					buffer = malloc(sizeof(t_posicion) + sizeof(t_HeapMetadata)*2);
					memcpy(buffer + offset, &puntero, tmpsize = sizeof(t_posicion));
					offset += tmpsize;
					memcpy(buffer + offset, &heapMetadata, tmpsize = sizeof(t_HeapMetadata));
					offset += tmpsize;
					memcpy(buffer + offset, &heapMetadataProx, tmpsize = sizeof(t_HeapMetadata));
					offset += tmpsize;

					_lockMemoria();
					send(socket_memoria, &pcb->pid, sizeof(t_pid), 0);
					msg_enviar_separado(ESCRITURA_PAGINA, offset, buffer, socket_memoria);
					offset = 0;

					t_msg* msgRecibido2 = msg_recibir(socket_memoria);
					if(msgRecibido2->tipoMensaje != ESCRITURA_PAGINA){
						log_error(logKernel, " [CPU %d | PID %d] No recibi ESCRITURA_PAGINA sino %d", socket_cpu, pcb->pid, msgRecibido2->tipoMensaje);
						if(msgRecibido->tipoMensaje == EXCEPCION_MEMORIA || msgRecibido->tipoMensaje == PAGINA_INVALIDA)
							return ERROR_EXCEPCION_MEMORIA;
						if(msgRecibido->tipoMensaje == STACKOVERFLOW)
							return ERROR_STACKOVERFLOW;
						else
							return SIN_DEFINICION;
					}
					msg_destruir(msgRecibido2);
					_unlockMemoria();

					free(buffer);
					list_destroy(listAux);
					return puntero.pagina * tamanioPag + puntero.offset;

				}else
					//sigo buscando
					j = j + sizeof(t_HeapMetadata) + heapMetadata.size;
					punteroAnterior = puntero;
					heapMetadataAnterior = heapMetadata;
			}
		}
	}
	list_destroy(listAux);
	// no encontre hueco
	return -1;
}



// si hay error en esta funcion devuelve el exitCode
t_puntero reservarMemoriaHeap(t_num cantBytes, t_PCB* pcb, int socket_cpu){
	int _esHeap(t_heap* h){
		return h->pid == pcb->pid;
	}
	bool _menorPag(t_heap* h1, t_heap* h2){
		return h1->nroPag >= h2->nroPag;
	}

	int retorno;
	if(cantBytes > tamanioPag - sizeof(t_HeapMetadata)*2){
		log_warning(logKernel, " [CPU %d | PID %d] Se quiere asignar mas que tamanio de pagina", socket_cpu, pcb->pid);
		return -MAS_MEMORIA_TAMANIO_PAG;
	}

	if( list_count_satisfying(tabla_heap, (void*)_esHeap) == 0 ){
		log_trace(logKernel, " [CPU %d | PID %d] Se reserva primer pagina heap", socket_cpu, pcb->pid);
		retorno = asignarNuevaPag(pcb, socket_cpu, 1);
		if(retorno < 0)
			return retorno;
	}

	retorno = encontrarHueco(cantBytes, pcb, socket_cpu);
	if( retorno == -1 ){
		log_warning(logKernel, " [CPU %d | PID %d] No encontre hueco", socket_cpu, pcb->pid);
		retorno = asignarNuevaPag(pcb, socket_cpu, 0);
		if(retorno < 0)
			return retorno;
		retorno = encontrarHueco(cantBytes, pcb, socket_cpu);
	}
	if( retorno < -1 )
		return retorno;
	if( retorno == -1 )
		return -SIN_DEFINICION;
	_sumarCantOpAlocar(pcb->pid, cantBytes);

	return retorno + sizeof(t_HeapMetadata);
}


t_HeapMetadata _recibirHeapMetadata(t_pid pid, t_posicion puntero, int socket_cpu){
	t_HeapMetadata heapMetadata;
	heapMetadata.isFree = false;

	log_trace(logKernel, " [CPU %d | PID %d] LECTURA_PAGINA HEAP %d %d %d", socket_cpu, pid, puntero.pagina, puntero.offset, puntero.size);

	if(puntero.offset + sizeof(t_HeapMetadata) >= tamanioPag)
		return heapMetadata;

	_lockMemoria();
	send(socket_memoria, &pid, sizeof(t_pid), 0);
	msg_enviar_separado(LECTURA_PAGINA, sizeof(t_posicion), &puntero, socket_memoria);

	//recibo
	t_msg* msgRecibido = msg_recibir(socket_memoria);
	if(msgRecibido->tipoMensaje != LECTURA_PAGINA){
		log_error(logKernel, " [CPU %d | PID %d] No recibi LECTURA_PAGINA sino %d", socket_cpu, pid, msgRecibido->tipoMensaje);
		if(msgRecibido->tipoMensaje == EXCEPCION_MEMORIA || msgRecibido->tipoMensaje == PAGINA_INVALIDA)
			return heapMetadata;
		if(msgRecibido->tipoMensaje == STACKOVERFLOW)
			return heapMetadata;	//todo: considerar error de stackoverflow
		else
			return heapMetadata;
		return heapMetadata;
	}
	msg_recibir_data(socket_memoria, msgRecibido);
	memcpy(&heapMetadata, msgRecibido->data, sizeof(t_HeapMetadata));
	msg_destruir(msgRecibido);
	_unlockMemoria();

	return heapMetadata;
}

int liberarMemoriaHeap(t_puntero posicion, t_PCB* pcb, int socket_cpu){
	int _esHeap(t_heap* h){
		return h->pid == pcb->pid;
	}
	bool flag_se_libero_pag = 0;

	posicion = posicion - sizeof(t_HeapMetadata);

	//envio solicitud
	t_posicion puntero;
	puntero.pagina = posicion / tamanioPag;
	puntero.offset = posicion % tamanioPag;
	puntero.size = sizeof(t_HeapMetadata);

	t_HeapMetadata heapMetadata = _recibirHeapMetadata(pcb->pid, puntero, socket_cpu);
	log_error(logKernel, " [CPU %d | PID %d] 		heapMetadata size %d free %d", socket_cpu, pid, heapMetadata.size, heapMetadata.isFree);

	if(heapMetadata.isFree == true){
		log_warning(logKernel, " [CPU %d | PID %d] Quiero liberar algo liberado", socket_cpu, pid);
		return -1;
	}

	heapMetadata.isFree = true;
	_sumarCantOpLiberar(pcb->pid, heapMetadata.size);

	/* no tengo q buscar el proximo
	puntero.offset = (posicion % tamanioPag) + sizeof(t_HeapMetadata) + heapMetadata.size;
	t_HeapMetadata heapMetadataProx = _recibirHeapMetadata(pcb->pid, puntero);
	log_error(logKernel, "		heapMetadataProx size %d free %d", heapMetadataProx.size, heapMetadataProx.isFree);

	if(heapMetadataProx.isFree == true){
		log_trace(logKernel, "El proximo esta libre, lo uno");
		heapMetadata.size = heapMetadata.size + sizeof(t_HeapMetadata) + heapMetadataProx.size;
	}
	puntero.offset = posicion % tamanioPag;
	*/

	void* buffer = malloc(sizeof(t_posicion) + sizeof(t_HeapMetadata));
	memcpy(buffer, &puntero, sizeof(t_posicion));
	memcpy(buffer + sizeof(t_posicion), &heapMetadata, sizeof(t_HeapMetadata));

	_lockMemoria();
	send(socket_memoria, &pcb->pid, sizeof(t_pid), 0);

	if(heapMetadata.size == tamanioPag - sizeof(t_HeapMetadata))
		msg_enviar_separado(LIBERAR, sizeof(t_posicion) + sizeof(t_HeapMetadata), buffer, socket_memoria);
	else
		msg_enviar_separado(ESCRITURA_PAGINA, sizeof(t_posicion) + sizeof(t_HeapMetadata), buffer, socket_memoria);

	t_msg* msgRecibido = msg_recibir(socket_memoria);
	if(msgRecibido->tipoMensaje == LIBERAR){
		log_trace(logKernel, " [CPU %d | PID %d] Se LIBERO", socket_cpu, pid);
		flag_se_libero_pag = 1;
		_sumarCantPagsHeap(pcb->pid, -1);
		list_remove_and_destroy_by_condition(tabla_heap, (void*) _esHeap, free);
	}else if(msgRecibido->tipoMensaje != ESCRITURA_PAGINA){
		log_error(logKernel, " [CPU %d | PID %d] No recibi ESCRITURA_PAGINA o LIBERAR sino %d", socket_cpu, pid, msgRecibido->tipoMensaje);
		if(msgRecibido->tipoMensaje == EXCEPCION_MEMORIA || msgRecibido->tipoMensaje == PAGINA_INVALIDA)
			return -4;
		if(msgRecibido->tipoMensaje == STACKOVERFLOW)
			return -3;	//todo: considerar error de stackoverflow
		else
			return -2;
	}
	msg_destruir(msgRecibido);
	_unlockMemoria();

	return flag_se_libero_pag;
}

/*
 * libreriaKernel.c
 *
 *  Created on: 3/4/2017
 *      Author: utnso
 */


#include "libreriaKernel.h"

#include "operacionesPCBKernel.h"
#include "planificacion.h"


t_puntero reservarMemoriaHeap(t_num cantBytes, t_PCB* pcb);	//si, pinto ponerlo aca
int liberarMemoriaHeap(t_puntero posicion, t_PCB* pcb);

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

t_PCB* recibir_pcb(int socket_cpu, t_msg* msgRecibido, bool flag_bloqueo, bool flag_colaReady){
	bool _esCPU2(t_infosocket* aux){
		return aux->socket == socket_cpu;
	}
	log_trace(logKernel, "Recibi PCB");
	t_PCB* pcb = desserealizarPCB(msgRecibido->data);
	_lockLista_PCB_cpu();
	list_remove_and_destroy_by_condition(lista_PCB_cpu, (void*) _esCPU2, free);
	_unlockLista_PCB_cpu();
	_sacarDeCola(pcb->pid, cola_Exec, mutex_Exec);

	if(flag_bloqueo)
		_ponerEnCola(pcb->pid, cola_Block, mutex_Block);
	if(flag_colaReady){
		_ponerEnCola(pcb->pid, cola_Ready, mutex_Ready);
		sem_post(&sem_cantColaReady);
	}

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
	pthread_mutex_unlock(&cpu->mutex);
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

		pthread_mutex_lock(&cpuUsada->mutex);
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

			log_trace(logKernel, "fdRecibido %d pidRecibido %d", fdRecibido, pidRecibido);

			TablaProceso = list_find(lista_tabla_de_procesos, (void*) _buscarPid); //tabla proceso es una lista

			if(TablaProceso == NULL){
				log_trace(logKernel, "no existe la tabla de este proceso");

				// terminar??
			} else {
				entradaProcesoBuscado = list_find(TablaProceso->lista_entradas_de_proceso, (void*) _buscarFD); //tabla proceso es una lista
				if(entradaProcesoBuscado == NULL){
					log_trace(logKernel, "no existe entrada de la tabla proceso que tenga a FD");
					// hacer algo??
				} else {

					entradaGlobalBuscada = list_find(lista_tabla_global, (void*) _buscarPorIndice);
					//entradaGlobalBuscada = list_remove_by_condition(lista_tabla_global, (void*) _buscarPorIndice);
				}
			}
		}

//-------------------------------------------------------------------------------

		switch(msgRecibido->tipoMensaje){



		case FINALIZAR_PROGRAMA:
			log_trace(logKernel, "Recibi FINALIZAR_PROGRAMA de CPU");
			flag_finalizado = true;
			//no break
		case ENVIO_PCB:		// si me devuelve el PCB es porque fue la ultima instruccion
			pcb = recibir_pcb(socket_cpu, msgRecibido, 0, !flag_finalizado);
			if(flag_finalizado){
				bool _esPid(t_infosocket* a){ return a->pidPCB == pcb->pid; }
				_lockLista_PCB_consola();
				t_infosocket* info = list_remove_by_condition(lista_PCB_consola, (void*) _esPid);
				_unlockLista_PCB_consola();
				_lockLista_PCB_cpu();
				list_remove_and_destroy_by_condition(lista_PCB_cpu, (void*) _esPid, free);
				_unlockLista_PCB_cpu();
				if(info == NULL)
					log_trace(logKernel, "No se encuentra consola asociada a pid %d", pcb->pid);
				msg_enviar_separado(FINALIZAR_PROGRAMA, sizeof(t_pid), &info->pidPCB, info->socket);
				sem_post(&sem_gradoMp);
				free(info);
				finalizarPid(pcb->pid);
				if((int)pcb->exitCode > 0){
					pcb->exitCode = 0;
					_ponerEnCola(pcb->pid, cola_Exit, mutex_Exit);
					liberarPCB(pcb, true);
				}
			}
			_lockLista_PCBs();
			if(list_remove_by_condition(lista_PCBs, (void*) _es_PCB) == NULL) log_warning(logKernel, "No se encuentra pcb en ENVIO_PCB");
			list_add(lista_PCBs, pcb);
			_unlockLista_PCBs();
			liberarCPU(cpuUsada);
			break;




		case 0: case FIN_CPU:
			fprintf(stderr, PRINT_COLOR_YELLOW "La cpu %d se ha desconectado" PRINT_COLOR_RESET "\n", socket_cpu);
			log_trace(logKernel, "La desconecto la cpu %d", socket_cpu);
			pthread_mutex_unlock(&cpuUsada->mutex);
			//si la cpu se desconecto la saco de la lista
			_lockLista_cpus();
			list_remove_and_destroy_by_condition(lista_cpus, (void*) _esCPU, free);
			_unlockLista_cpus();
			_lockLista_PCB_cpu();
			list_remove_and_destroy_by_condition(lista_PCB_cpu, (void*) _esCPU2, free);
			_unlockLista_PCB_cpu();
			if(pcb != NULL){
				_sacarDeCola(pcb->pid, cola_Exec, mutex_Exec);
				setearExitCode(pcb->pid, SIN_DEFINICION);
				void _sacarDeSemaforos(t_VariableSemaforo* sem){
					t_pid encontrado = _sacarDeCola(pcb->pid, sem->colaBloqueados, sem->mutex_colaBloqueados);
					if(encontrado == pcb->pid)
						sem->valorSemaforo++;
				}
				list_iterate(lista_variablesSemaforo, (void*) _sacarDeSemaforos);
				_lockLista_PCBs();
				if(list_remove_by_condition(lista_PCBs, (void*) _es_PCB) == NULL) log_warning(logKernel, "No se encuentra pcb en FIN_CPU");
				list_add(lista_PCBs, pcb);
				_unlockLista_PCBs();
			}
			sem_post(&sem_gradoMp);
			pthread_exit(NULL);
			break;




		case ERROR:
			log_trace(logKernel, "Recibi ERROR de CPU");
			pcb = recibir_pcb(socket_cpu, msgRecibido, 0, 0);
			finalizarPid(pcb->pid);
			if((int)pcb->exitCode > 0){
				pcb->exitCode = ERROR_SCRIPT;
				_ponerEnCola(pcb->pid, cola_Exit, mutex_Exit);
				liberarPCB(pcb, true);
			}
			_lockLista_PCBs();
			if(list_remove_by_condition(lista_PCBs, (void*) _es_PCB) == NULL) log_warning(logKernel, "No se encuentra pcb en ERROR");
			list_add(lista_PCBs, pcb);
			_unlockLista_PCBs();
			_lockLista_PCB_consola();
			t_infosocket* info = list_find(lista_PCB_consola, (void*) _esPid);
			_unlockLista_PCB_consola();
			msg_enviar_separado(ERROR, sizeof(t_pid), &pcb->pid, info->socket);
			sem_post(&sem_gradoMp);
			liberarCPU(cpuUsada);
			break;




		case GRABAR_VARIABLE_COMPARTIDA:
			log_trace(logKernel, "Recibi GRABAR_VARIABLE_COMPARTIDA de CPU");
			memcpy(&tamanioNombre, msgRecibido->data, sizeof(t_num));
			offset += sizeof(t_num);
			varCompartida->nombre = malloc(tamanioNombre+1);
			memcpy(varCompartida->nombre, msgRecibido->data + offset, tamanioNombre);
			varCompartida->nombre[tamanioNombre] = '\0';
			offset += tamanioNombre;
			memcpy(&varCompartida->valor, msgRecibido->data + offset, sizeof(t_valor_variable));
			log_info(logKernel, "Variable %s, valor %d", varCompartida->nombre, varCompartida->valor);

			t_VariableCompartida* varBuscada = list_find(lista_variablesCompartidas, (void*) _buscar_VarComp);
			if (varBuscada == NULL){
				log_error(logKernel, "Error no encontre VALOR_VARIABLE_COMPARTIDA");
				msg_enviar_separado(ERROR, 0, 0, socket_cpu);
			}else{
				varBuscada->valor = varCompartida->valor;
				msg_enviar_separado(GRABAR_VARIABLE_COMPARTIDA, 0, 0, socket_cpu);
				_sumarCantOpPriv(pcb->pid);
			}
			free(varCompartida->nombre);
			pthread_mutex_unlock(&cpuUsada->mutex);
			break;




		case VALOR_VARIABLE_COMPARTIDA:
			log_trace(logKernel, "Recibi VALOR_VARIABLE_COMPARTIDA de CPU");
			memcpy(&tamanioNombre, &msgRecibido->longitud, sizeof(t_num));
			varCompartida->nombre = malloc(tamanioNombre+1);
			memcpy(varCompartida->nombre, msgRecibido->data, tamanioNombre);
			varCompartida->nombre[tamanioNombre] = '\0';
			log_info(logKernel, "Variable %s", varCompartida->nombre);

			t_VariableCompartida* varBuscad = list_find(lista_variablesCompartidas, (void*) _buscar_VarComp);
			if (varBuscad == NULL){
				log_error(logKernel, "Error no encontre VALOR_VARIABLE_COMPARTIDA");
				msg_enviar_separado(ERROR, 0, 0, socket_cpu);
			}else{
				log_info(logKernel, "Valor %d", varBuscad->valor);
				msg_enviar_separado(VALOR_VARIABLE_COMPARTIDA, sizeof(t_valor_variable), &varBuscad->valor, socket_cpu);
				_sumarCantOpPriv(pcb->pid);
			}
			free(varCompartida->nombre);
			pthread_mutex_unlock(&cpuUsada->mutex);
			break;




		case SIGNAL:
			log_trace(logKernel, "Recibi SIGNAL de CPU");
			memcpy(&tamanioNombre, &msgRecibido->longitud, sizeof(t_num));
			varSemaforo->nombre = malloc(tamanioNombre+1);
			memcpy(varSemaforo->nombre, msgRecibido->data, tamanioNombre);
			varSemaforo->nombre[tamanioNombre] = '\0';
			log_info(logKernel, "Semaforo %s", varSemaforo->nombre);

			t_VariableSemaforo* semBuscado = list_remove_by_condition(lista_variablesSemaforo, (void*) _buscar_VarSem);
			if (semBuscado == NULL){
				log_error(logKernel, "Error no encontre SEMAFORO");
				msg_enviar_separado(ERROR, 0, 0, socket_cpu);
			}else{
				semBuscado->valorSemaforo++;
				log_trace(logKernel, "Valor %d", semBuscado->valorSemaforo);
				if(semBuscado->valorSemaforo <= 0){
					t_pid pidADesbloquear;
					void* tmp = malloc(sizeof(t_pid));
					pthread_mutex_lock(&semBuscado->mutex_colaBloqueados);
					tmp = queue_pop(semBuscado->colaBloqueados);
					pthread_mutex_unlock(&semBuscado->mutex_colaBloqueados);
					memcpy(&pidADesbloquear, tmp, sizeof(t_pid));
					free(tmp);
					log_trace(logKernel, "Desbloqueo pid %d", pidADesbloquear);

					_sacarDeCola(pidADesbloquear, cola_Block, mutex_Block);
					_ponerEnCola(pidADesbloquear, cola_Ready, mutex_Ready);
					sem_post(&sem_cantColaReady);
				}
				list_add(lista_variablesSemaforo, semBuscado);
				msg_enviar_separado(SIGNAL, 0, 0, socket_cpu);
				_sumarCantOpPriv(pcb->pid);
			}
			free(varSemaforo->nombre);
			pthread_mutex_unlock(&cpuUsada->mutex);
			break;




		case WAIT:
			log_trace(logKernel, "Recibi WAIT de CPU");
			memcpy(&tamanioNombre, &msgRecibido->longitud, sizeof(t_num));
			varSemaforo->nombre = malloc(tamanioNombre+1);
			memcpy(varSemaforo->nombre, msgRecibido->data, tamanioNombre);
			varSemaforo->nombre[tamanioNombre] = '\0';
			log_info(logKernel, "Semaforo %s", varSemaforo->nombre);

			t_VariableSemaforo* semBuscad = list_remove_by_condition(lista_variablesSemaforo, (void*) _buscar_VarSem);
			if (semBuscad == NULL){
				log_error(logKernel, "Error no encontre SEMAFORO");
				msg_enviar_separado(ERROR, 0, 0, socket_cpu);
			}else{
				semBuscad->valorSemaforo--;
				log_trace(logKernel, "Valor semaforo %s %d", semBuscad->nombre, semBuscad->valorSemaforo);
				msg_enviar_separado(WAIT, sizeof(t_num), &semBuscad->valorSemaforo, socket_cpu);

				if(semBuscad->valorSemaforo < 0){
					void* tmp = malloc(sizeof(t_pid));
					memcpy(tmp, &pcb->pid, sizeof(t_pid));
					pthread_mutex_lock(&semBuscad->mutex_colaBloqueados);
					queue_push(semBuscad->colaBloqueados, tmp);
					pthread_mutex_unlock(&semBuscad->mutex_colaBloqueados);
					log_trace(logKernel, "Bloqueo pid %d", pcb->pid);
					//recibo pcb
					msg_destruir(msgRecibido);
					msgRecibido = msg_recibir(socket_cpu);
					if(msg_recibir_data(socket_cpu, msgRecibido) > 0){
						pcb = recibir_pcb(socket_cpu, msgRecibido, 1, 0);
						_lockLista_PCBs();
						if(list_remove_by_condition(lista_PCBs, (void*) _es_PCB) == NULL) log_warning(logKernel, "No se encuentra pcb en WAIT");
						list_add(lista_PCBs, pcb);
						_unlockLista_PCBs();
						liberarCPU(cpuUsada);
					}else
						log_warning(logKernel, "No recibi nada");
				}

				list_add(lista_variablesSemaforo, semBuscad);
				_sumarCantOpPriv(pcb->pid);
			}
			free(varSemaforo->nombre);
			pthread_mutex_unlock(&cpuUsada->mutex);
			break;




		case ASIGNACION_MEMORIA:
			log_trace(logKernel, "Recibi ASIGNACION_MEMORIA de CPU");
			t_num cantBytes;
			memcpy(&cantBytes, msgRecibido->data, sizeof(t_valor_variable));
			log_trace(logKernel, "ASIGNACION_MEMORIA %d bytes", cantBytes);

			/*//recibo pcb
			tmpsize = msgRecibido->longitud - sizeof(t_valor_variable);
			void* tmpbuffer = malloc(tmpsize);
			memcpy(tmpbuffer, msgRecibido->data + sizeof(t_valor_variable), tmpsize);
			pcb = desserealizarPCB(tmpbuffer);
			free(tmpbuffer);*/
			bool flag_se_agrego_pag = 0;
			if(pcb->cantPagsHeap < _cantPagsHeap(pcb->pid))
				flag_se_agrego_pag = 1;

			t_puntero direccion = reservarMemoriaHeap(cantBytes, pcb);

			void* tmpb = malloc(sizeof(t_puntero) + sizeof(bool));
			memcpy(tmpb, &direccion, sizeof(t_puntero));
			memcpy(tmpb + sizeof(t_puntero), &flag_se_agrego_pag, sizeof(bool));

			if((int)direccion > 0)
				msg_enviar_separado(ASIGNACION_MEMORIA, sizeof(t_puntero) + sizeof(bool), tmpb, socket_cpu);
			else
				msg_enviar_separado(ERROR, 0, 0, socket_cpu);
			free(tmpb);

			_lockLista_PCBs();
			if(list_remove_by_condition(lista_PCBs, (void*) _es_PCB) == NULL) log_warning(logKernel, "No se encuentra pcb en ASIGNACION_MEMORIA");
			list_add(lista_PCBs, pcb);
			_unlockLista_PCBs();

			_sumarCantOpPriv(pcb->pid);
			break;



		case LIBERAR:
			log_trace(logKernel, "Recibi LIBERAR de CPU");
			t_puntero posicion;
			memcpy(&posicion, msgRecibido->data, msgRecibido->longitud);
			log_trace(logKernel, "LIBERAR posicion %d", posicion);

			if( liberarMemoriaHeap(posicion, pcb) < 0 )
				msg_enviar_separado(ERROR, 0, 0, socket_cpu);
			else
				msg_enviar_separado(LIBERAR, 1, 0, socket_cpu);

			_lockLista_PCBs();
			if(list_remove_by_condition(lista_PCBs, (void*) _es_PCB) == NULL) log_warning(logKernel, "No se encuentra pcb en LIBERAR");
			list_add(lista_PCBs, pcb);
			_unlockLista_PCBs();

			_sumarCantOpPriv(pcb->pid);
			break;
//----------------------------------------------------------------------------------------------------
		case MOVER_ANSISOP:
			log_trace(logKernel, "Recibi MOVER_ANSISOP de CPU");
			t_valor_variable cursorRecibido;

			memcpy(&fdRecibido, msgRecibido->data, sizeof(t_descriptor_archivo));
			memcpy(&cursorRecibido, msgRecibido->data + sizeof(t_descriptor_archivo), sizeof(t_valor_variable));
			memcpy(&pidRecibido, msgRecibido->data + sizeof(t_descriptor_archivo) + sizeof(t_valor_variable), sizeof(t_pid));

			_cargarEntradasxTabla();

			if(entradaProcesoBuscado == NULL){
				log_trace(logKernel, "no existe la entrada buscada");
			}else{
				entradaProcesoBuscado->cursor = cursorRecibido;
				log_trace(logKernel, "se seteo el cursor");
			}

			break;




		case LEER_ANSISOP:
			log_trace(logKernel, "Recibi LEER_ANSISOP de CPU");
			t_valor_variable tamanio;

			memcpy(&fdRecibido, msgRecibido->data + offset, tmpsize = sizeof(t_descriptor_archivo));
			offset += tmpsize;
			memcpy(&tamanio, msgRecibido->data + offset, tmpsize = sizeof(t_valor_variable));
			offset += tmpsize;
			memcpy(&pidRecibido, msgRecibido->data + offset, tmpsize = sizeof(t_pid));
			offset += tmpsize;

			_cargarEntradasxTabla();

			if(entradaGlobalBuscada == NULL){
				log_trace(logKernel, "no existe la entrada buscada");
			}else{

				if(entradaProcesoBuscado->bandera.lectura == 1){
					t_num sizePath = string_length(entradaGlobalBuscada->FilePath);
					void* buffer = malloc(sizeof(t_num) + sizePath + sizeof(t_valor_variable)*2);
					log_trace(logKernel, "pi");

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
						log_trace(logKernel, "lei bien");

						msg_recibir_data(socket_fs, msgDatosObtenidos);
						msg_enviar_separado(OBTENER_DATOS, msgDatosObtenidos->longitud, msgDatosObtenidos->data, socket_cpu);
						msg_destruir(msgDatosObtenidos);

					}else{
						msg_destruir(msgDatosObtenidos);
						log_error(logKernel, "Error al leer");
						// todo: manejar error NO_EXISTE_ARCHIVO o algo parecido
					}
				}else{
					log_error(logKernel, "No tiene permisos de escritura");
					// todo: hacer algo mas tipo enviar un error
				}

			}
			break;




		case ESCRIBIR_FD:

			log_trace(logKernel, "Recibi ESCRIBIR_FD de CPU");
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
				log_trace(logKernel, "Imprimo por consola: %s", datos);
				_lockLista_PCB_consola();
				t_infosocket* info = list_find(lista_PCB_consola, (void*) _esPid);
				_unlockLista_PCB_consola();
				if(info == NULL)
					log_trace(logKernel, "No se ecuentra consola asociada a pid %d", pcb->pid);
				msg_enviar_separado(IMPRIMIR_TEXTO, sizeInformacion, datos, info->socket);
				msg_enviar_separado(ESCRIBIR_FD, 0, 0, socket_cpu);
			}else{

				if (entradaProcesoBuscado->bandera.escritura == 1){

					log_trace(logKernel, "Escribo en fd %d: %s", fdRecibido, datos);

					_cargarEntradasxTabla();

					if(entradaGlobalBuscada == NULL){
						log_trace(logKernel, "no existe la entrada buscada");
					}else{
						t_num sizePath = string_length(entradaGlobalBuscada->FilePath);
						void* buffer = malloc(sizeof(t_num) + sizePath
									   + sizeof(t_valor_variable) + sizeof(t_valor_variable)) + sizeInformacion;
						offset = 0;
						tmpsize = 0;
						memcpy(buffer, &sizePath, tmpsize = sizeof(t_num));
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
						free(buffer);

						msg_destruir(msgRecibido);
						msgRecibido = msg_recibir(socket_fs);

						if(msgRecibido->tipoMensaje == GUARDAR_DATOS){
							log_trace(logKernel, "Escribio bien");
							msg_recibir_data(socket_fs, msgRecibido);
							msg_enviar_separado(GUARDAR_DATOS, 0, 0, socket_cpu);
						}else{
							log_error(logKernel, "Error al escribir");
							// todo: manejar error NO_EXISTE_ARCHIVO o algo parecido
						}

					}
				}else{
					log_error(logKernel, "No tiene permisos de lectura");
					// todo: hacer algo mas tipo enviar un error
				}

			}

			_sumarCantOpPriv(pcb->pid);
			free(datos);
			pthread_mutex_unlock(&cpuUsada->mutex);
			break;


//-----------------------------------------------------------------------------------------

		case ABRIR_ANSISOP:
			// fixme cuando ejecute 2do script q abre archivo exploto -> buscar porque
			log_trace(logKernel, "Recibi la siguiente operacion ABRIR_ANSISOP de CPU");
			t_direccion_archivo pathArchivo;
			t_num longitudPath;
			t_banderas flags;
			t_pid pid;
			offset = 0;
			memcpy(&longitudPath, msgRecibido->data + offset, tmpsize = sizeof(t_num));
			offset += tmpsize;
			pathArchivo = malloc(longitudPath + 1);
			memcpy(pathArchivo, msgRecibido->data + offset, tmpsize = longitudPath);
			pathArchivo[longitudPath] = '\0';
			offset += tmpsize;
			memcpy(&flags, msgRecibido->data + offset, tmpsize = sizeof(t_banderas));
			offset += tmpsize;
			memcpy(&pid, msgRecibido->data + offset, tmpsize = sizeof(t_pid));
			offset += tmpsize;

//-------------- busco si ya tengo este path en la tabla global ---------

			int _buscarPath(t_entrada_GlobalFile* a){ return string_equals_ignore_case(a->FilePath,pathArchivo); }
			t_entrada_GlobalFile* entradaGlobalOld = list_find(lista_tabla_global, (void*) _buscarPath);

			t_descriptor_archivo indiceActualGlobal;

			if(entradaGlobalOld == NULL){
				log_trace(logKernel, "Se abre el archivo por primera vez");
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

//-------------- seteo variable necesarias para la tabla de procesos ------------------------------

			t_entrada_proceso *entradaProcesoNew = malloc(sizeof(t_entrada_proceso));

			entradaProcesoNew->bandera = flags;

			if(flags.creacion){

				// todo: chequear si existe primero

				// si no exite crearlo
				msg_enviar_separado(CREAR_ARCHIVO, longitudPath, pathArchivo, socket_fs);

			}

			entradaProcesoNew->referenciaGlobalTable = indiceActualGlobal;

			entradaProcesoNew->cursor = 0; // seteo del cursor

			//-------------- busco si ya tengo la tabla para este id de proceso -------------------------

			int _espid(t_tabla_proceso* a){ return a->pid == pid; }
			// todo: poner un mutex lock lista_tabla_de_procesos
			t_tabla_proceso* tablaProceso = list_remove_by_condition(lista_tabla_de_procesos, (void*) _esPid);

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
			// todo: poner un mutex UNLOCK lista_tabla_de_procesos

			// le mando a cpu el fd asignado
			msg_enviar_separado(ABRIR_ANSISOP, sizeof(t_descriptor_archivo), &entradaProcesoNew->fd, socket_cpu);

			free(pathArchivo);

			_sumarCantOpPriv(pcb->pid);
			break;




		case BORRAR_ANSISOP:

			log_trace(logKernel, "Recibi la siguiente operacion BORRAR_ANSISOP de CPU");

			memcpy(&fdRecibido, msgRecibido->data, sizeof(t_descriptor_archivo));
			memcpy(&pidRecibido, msgRecibido->data + sizeof(t_descriptor_archivo), sizeof(t_pid));

			_cargarEntradasxTabla();

			if(entradaGlobalBuscada == NULL){
				log_trace(logKernel, "no existe la entrada buscada");
			}else{
				if(entradaGlobalBuscada->Open == 1){
					msg_enviar_separado(BORRAR, string_length(entradaGlobalBuscada->FilePath), entradaGlobalBuscada->FilePath, socket_fs);
					list_remove_by_condition(lista_tabla_global, (void*) _esIndiceGlobal);
					list_remove_by_condition(TablaProceso->lista_entradas_de_proceso, (void*) _esFD);
					msg_enviar_separado(BORRAR,0,0,socket_cpu);
				}else{
					log_trace(logKernel, "el archivo esta siendo usado por %d procesos, no se puede borrar", entradaGlobalBuscada->Open);
					msg_enviar_separado(ERROR,0,0,socket_cpu);
				}
			}
			_sumarCantOpPriv(pcb->pid);
			break;



		case CERRAR_ANSISOP:

			log_trace(logKernel, "Recibi la siguiente operacion CERRAR_ANSISOP de CPU");

			memcpy(&fdRecibido, msgRecibido->data, sizeof(t_descriptor_archivo));
			memcpy(&pidRecibido, msgRecibido->data + sizeof(t_descriptor_archivo), sizeof(t_pid));

			_cargarEntradasxTabla();

			if(entradaGlobalBuscada == NULL){
				log_trace(logKernel, "no existe la entrada buscada");
			}else{
				entradaGlobalBuscada = list_remove_by_condition(lista_tabla_global, (void*) _buscarPorIndice);
				// saco la entrada global buscada
				entradaGlobalBuscada->Open --;

				//int _esFD(t_entrada_proceso *entradaP){return entradaP->indice == entradaProcesoBuscado->indice;}
				list_remove_by_condition(TablaProceso->lista_entradas_de_proceso, (void*) _esFD);
				if(entradaGlobalBuscada->Open == 0){
					// no hago nada ya que la saque antes
				} else {
					// vuelvo a meter la entrada que modifique en la tabla global
					list_add(lista_tabla_global, entradaGlobalBuscada);
				}
			}

			_sumarCantOpPriv(pcb->pid);
			break;
		} //end switch

		free(TablaProceso);
		free(entradaProcesoBuscado);
		free(entradaGlobalBuscada);
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

	send(socket_memoria, &aux->pid, sizeof(t_pid), 0);
	msg_enviar_separado(INICIALIZAR_PROGRAMA, aux->size, aux->script, socket_memoria);
	send(socket_memoria, &pcb->cantPagsStack, sizeof(t_num16), 0);
	t_num8 respuesta;
	recv(socket_memoria, &respuesta, sizeof(t_num8), 0);

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
			finalizarPid(i->pidPCB);
			setearExitCode(i->pidPCB, DESCONEXION_CONSOLA);
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
		list_remove_by_condition(lista_consolas, (void*) _esConsola);
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

					if((int)pcbA->exitCode > 0)
						exitCode = " - ";
					else
						exitCode = string_from_format("%d", (int)pcbA->exitCode);
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
						PRINT_COLOR_BLUE "  e -" PRINT_COLOR_RESET " Cantidad de acciones alocar realizadas \n"
						PRINT_COLOR_BLUE "  f -" PRINT_COLOR_RESET " Cantidad de acciones liberar realizadas \n" );

				switch(getchar()){
				case 'a':
					printf( "Cantidad de rafagas ejecutadas: %d \n", infP->cantRafagas);
					break;
				case 'b':
					printf( "Cantidad de operaciones privilegiadas que ejecutó: %d \n", infP->cantOpPriv);
					break;
				case 'c':
					printf( "Tabla de archivos abiertos por el proceso %d \n", infP->tablaArchivos);	// todo: implementar
					break;
				case 'd':
					printf( "Cantidad de páginas de Heap utilizadas: %d \n", infP->cantPagsHeap);
					break;
				case 'e':
					printf( "Cantidad de operaciones alocar: %d \n", infP->cantOp_alocar);
					printf( "Se alocaron %d bytes \n", infP->canrBytes_alocar);
					break;
				case 'f':
					printf( "Cantidad de operaciones liberar: %d \n", infP->cantOp_liberar);
					printf( "Se liberaron %d bytes \n", infP->canrBytes_liberar);
					break;
				default:
					fprintf(stderr, PRINT_COLOR_YELLOW "Capo que me tiraste?" PRINT_COLOR_RESET "\n");
				}

			}
			_unlockLista_infoProc();
		}else if(string_equals_ignore_case(comando, "tabla archivos")){

			// todo: implementar

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
				if((int)pcbKill->exitCode <= 0){
					log_trace(logKernel,  "El PID %d ya finalizo", pidKill);
					fprintf(stderr, PRINT_COLOR_YELLOW "El PID %d ya finalizo" PRINT_COLOR_RESET "\n", pidKill);
				}else{
					finalizarPid(pidKill);
					setearExitCode(pidKill, COMANDO_FINALIZAR);
					fprintf(stderr, PRINT_COLOR_BLUE "Finalizo PID %d" PRINT_COLOR_RESET "\n", pidKill);
				}
			}

		}else if(string_equals_ignore_case(comando, "detener")){

			log_trace(logKernel, "Detengo planificacion");
			pthread_mutex_lock(&mut_detengo_plani);

		}else if(string_equals_ignore_case(comando, "reanudar")){

			log_trace(logKernel, "Reanudo planificacion");
			pthread_mutex_unlock(&mut_detengo_plani);

		}else if(string_equals_ignore_case(comando, "help")){
			printf("Comandos:\n"
					PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "list procs: " PRINT_COLOR_RESET "Obtener listado de procesos\n"
					PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "info [pid]: " PRINT_COLOR_RESET "Obtener informacion de proceso\n"
					PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "tabla archivos: " PRINT_COLOR_RESET "Obtener informacion de proceso\n"
					PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "grado mp [grado]: " PRINT_COLOR_RESET "Setear grado de multiprogramacion\n"
					PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "kill [pid]: " PRINT_COLOR_RESET "Finalizar proceso\n"
					PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "detener: " PRINT_COLOR_RESET "Detener la planificación\n"
					PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "reanudar: " PRINT_COLOR_RESET "Reanuda la planificación\n");

		}else if(string_equals_ignore_case(comando, "\0")){
		}else
			fprintf(stderr, PRINT_COLOR_YELLOW "El comando '%s' no es valido" PRINT_COLOR_RESET "\n", comando);

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
	log_trace(logKernel, "Finalizo pid %d", pid);
	int _buscarPID(t_infosocket* i){
		return i->pidPCB == pid;
	}
	bool flag_hagoesto = 1;
	t_infosocket* info;
	_lockLista_PCB_cpu();
	info = list_find(lista_PCB_cpu, (void*) _buscarPID);
	_unlockLista_PCB_cpu();
	if(info == NULL){
		log_info(logKernel, "No se encuentra cpu ejecutando pid %d", pid);
		_sacarDeCola(pid, cola_Block, mutex_Block);
		if(_sacarDeCola(pid, cola_Ready, mutex_Ready) == pid)
			sem_wait(&sem_cantColaReady);
	}
	else{
		_sacarDeCola(pid, cola_Exec, mutex_Exec);
		int _buscarCPU(t_cpu* c){
			return c->socket == info->socket;
		}
		_lockLista_cpus();
		t_cpu* cpu = list_find(lista_cpus, (void*) _buscarCPU);
		_unlockLista_cpus();
		if(cpu == NULL)
			log_info(logKernel, "No se encuentra cpu ejecutando pid %d", pid);
		else{
			flag_hagoesto = 0;
			kill(cpu->pidProcesoCPU, SIGUSR2);
		}
	}

	if(flag_hagoesto){
		void _sacarDeSemaforos(t_VariableSemaforo* sem){
			t_pid encontrado = _sacarDeCola(pid, sem->colaBloqueados, sem->mutex_colaBloqueados);
			if(encontrado == pid)
				sem->valorSemaforo++;
		}
		list_iterate(lista_variablesSemaforo, (void*) _sacarDeSemaforos);

		send(socket_memoria, &pid, sizeof(t_pid), 0);
		msg_enviar_separado(FINALIZAR_PROGRAMA, 0, 0, socket_memoria);
		sem_post(&sem_gradoMp);
	}
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






void asignarNuevaPag(t_PCB* pcb){
	int _esHeap(t_heap* h){
		return h->pid == pcb->pid;
	}
	int nroPag = 0;
	void _buscarMayorNumPag(t_heap* h){
		if(h->pid == pcb->pid && nroPag < h->nroPag)
			nroPag = h->nroPag;
	}

	log_trace(logKernel, "Asigno nueva pagina a proceso %d", pcb->pid);
	send(socket_memoria, &pcb->pid, sizeof(t_pid), 0);
	msg_enviar_separado(ASIGNACION_MEMORIA, 0, 0, socket_memoria);
	t_msg* msgRecibido = msg_recibir(socket_memoria);
	if(msgRecibido->tipoMensaje != ASIGNACION_MEMORIA)
		log_error(logKernel, "No recibi ASIGNACION_MEMORIA sino %d", msgRecibido->tipoMensaje);
	//msg_recibir_data(socket_memoria, msgRecibido);
	msg_destruir(msgRecibido);

	t_heap* nuevaHeap = malloc(sizeof(t_heap));
	nuevaHeap->pid = pcb->pid;
	list_iterate(tabla_heap, (void*) _buscarMayorNumPag);
	nuevaHeap->nroPag = nroPag;
	log_trace(logKernel, "Nueva pag numero %d", nroPag);

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

	send(socket_memoria, &pcb->pid, sizeof(t_pid), 0);
	msg_enviar_separado(ESCRITURA_PAGINA, sizeof(t_posicion) + sizeof(t_HeapMetadata), buffer, socket_memoria);
	msgRecibido = msg_recibir(socket_memoria);
	if(msgRecibido->tipoMensaje != ESCRITURA_PAGINA)
		log_error(logKernel, "No recibi ESCRITURA_PAGINA sino %d", msgRecibido->tipoMensaje);
	msg_destruir(msgRecibido);

	_sumarCantPagsHeap(pcb->pid, 1);

	free(buffer);
	list_add(tabla_heap, nuevaHeap);
}




t_puntero encontrarHueco(t_num cantBytes, t_PCB* pcb){

	int _esHeap(t_heap* h){
		return h->pid == pcb->pid;
	}
	bool _menorPag(t_heap* h1, t_heap* h2){
		return h1->nroPag <= h2->nroPag;
	}

	t_list* listAux = list_filter(tabla_heap, (void*) _esHeap);
	list_sort(tabla_heap, (void*) _menorPag);

	int i=0, j=0, offset=0, tmpsize;

	for(; i < list_size(listAux); i++){
		t_heap* h = list_get(listAux, i);
		if(h->nroPag == i){
			j = 0;
			while(j < tamanioPag){
				//envio solicitud
				t_posicion puntero;
				puntero.pagina = pcb->cantPagsCodigo + pcb->cantPagsStack + i;
				puntero.offset = j;
				puntero.size = sizeof(t_HeapMetadata);
				send(socket_memoria, &pcb->pid, sizeof(t_pid), 0);
				msg_enviar_separado(LECTURA_PAGINA, sizeof(t_posicion), &puntero, socket_memoria);

				//recibo
				t_msg* msgRecibido = msg_recibir(socket_memoria);
				if(msgRecibido->tipoMensaje != LECTURA_PAGINA){
					log_error(logKernel, "No recibi LECTURA_PAGINA sino %d", msgRecibido->tipoMensaje);
					if(msgRecibido->tipoMensaje == STACKOVERFLOW)
						return -3;	//todo: considerar error de stackoverflow
					else
						return -2;
				}
				msg_recibir_data(socket_memoria, msgRecibido);
				t_HeapMetadata heapMetadata;
				memcpy(&heapMetadata, msgRecibido->data, sizeof(t_HeapMetadata));
				msg_destruir(msgRecibido);

				if(heapMetadata.isFree && cantBytes <= heapMetadata.size ){
					//encontre hueco
					t_HeapMetadata heapMetadataProx;
					heapMetadataProx.isFree = true;
					heapMetadataProx.size = heapMetadata.size - cantBytes - sizeof(t_HeapMetadata);
					heapMetadata.isFree = false;
					heapMetadata.size = cantBytes;

					void* buffer = malloc(sizeof(t_posicion) + sizeof(t_HeapMetadata)*2);
					memcpy(buffer + offset, &puntero, tmpsize = sizeof(t_posicion));
					offset += tmpsize;
					memcpy(buffer + offset, &heapMetadata, tmpsize = sizeof(t_HeapMetadata));
					offset += tmpsize;
					memcpy(buffer + offset, &heapMetadataProx, tmpsize = sizeof(t_HeapMetadata));
					offset += tmpsize;

					send(socket_memoria, &pcb->pid, sizeof(t_pid), 0);
					msg_enviar_separado(ESCRITURA_PAGINA, offset, buffer, socket_memoria);

					t_msg* msgRecibido2 = msg_recibir(socket_memoria);
					if(msgRecibido2->tipoMensaje != ESCRITURA_PAGINA){
						log_error(logKernel, "No recibi ESCRITURA_PAGINA sino %d", msgRecibido2->tipoMensaje);
						return -2;
					}
					msg_destruir(msgRecibido2);

					free(buffer);
					list_destroy(listAux);
					return puntero.pagina * tamanioPag + puntero.offset;

				}else
					//sigo buscando
					j = j + sizeof(t_HeapMetadata) + heapMetadata.size;
			}
		}
	}
	list_destroy(listAux);
	// no encontre hueco
	return -1;
}




t_puntero reservarMemoriaHeap(t_num cantBytes, t_PCB* pcb){
	int _esHeap(t_heap* h){
		return h->pid == pcb->pid;
	}
	bool _menorPag(t_heap* h1, t_heap* h2){
		return h1->nroPag >= h2->nroPag;
	}

	if(cantBytes > tamanioPag - sizeof(t_HeapMetadata)*2){
		log_warning(logKernel, "Se quiere asignar mas que tamanio de pagina");
		finalizarPid(pcb->pid);
		if((int)pcb->exitCode > 0){
			pcb->exitCode = MAS_MEMORIA_TAMANIO_PAG;
			_ponerEnCola(pcb->pid, cola_Exit, mutex_Exit);
			liberarPCB(pcb, true);
		}
		return -1;
	}

	if( list_count_satisfying(tabla_heap, (void*)_esHeap) == 0 ){
		log_trace(logKernel, "Se reserva primer pagina heap pid %d", pcb->pid);
		asignarNuevaPag(pcb);
	}

	int retorno = encontrarHueco(cantBytes, pcb);
	if( retorno == -2 )
		return -1;
	if( retorno == -1 ){
		log_trace(logKernel, "No encontre hueco");
		asignarNuevaPag(pcb);
		retorno = encontrarHueco(cantBytes, pcb);
	}
	_sumarCantOpAlocar(pcb->pid, cantBytes);

	return retorno;
}


t_HeapMetadata _recibirHeapMetadata(t_pid pid, t_posicion puntero){
	t_HeapMetadata heapMetadata;
	send(socket_memoria, &pid, sizeof(t_pid), 0);
	msg_enviar_separado(LECTURA_PAGINA, sizeof(t_posicion), &puntero, socket_memoria);

	//recibo
	t_msg* msgRecibido = msg_recibir(socket_memoria);
	if(msgRecibido->tipoMensaje != LECTURA_PAGINA){
		log_error(logKernel, "No recibi LECTURA_PAGINA sino %d", msgRecibido->tipoMensaje);
		return heapMetadata;
	}
	msg_recibir_data(socket_memoria, msgRecibido);
	memcpy(&heapMetadata, msgRecibido->data, sizeof(t_HeapMetadata));
	msg_destruir(msgRecibido);

	return heapMetadata;
}

int liberarMemoriaHeap(t_puntero posicion, t_PCB* pcb){
	int _esHeap(t_heap* h){
		return h->pid == pcb->pid;
	}
	bool flag_se_libero_pag = 0;

	//envio solicitud
	t_posicion puntero;
	puntero.pagina = posicion / tamanioPag;
	puntero.offset = posicion % tamanioPag;
	puntero.size = sizeof(t_HeapMetadata);

	t_HeapMetadata heapMetadata = _recibirHeapMetadata(pcb->pid, puntero);
	puntero.offset = (posicion % tamanioPag) + sizeof(t_HeapMetadata) + heapMetadata.size;
	t_HeapMetadata heapMetadataProx = _recibirHeapMetadata(pcb->pid, puntero);

	if(heapMetadata.isFree == true){
		log_warning(logKernel, "Quiero liberar algo liberado");
		return -1;
	}

	_sumarCantOpLiberar(pcb->pid, heapMetadata.size);

	heapMetadata.isFree = true;
	if(heapMetadataProx.isFree == true){
		log_trace(logKernel, "El proximo esta libre, lo uno");
		heapMetadata.size = heapMetadata.size + sizeof(t_HeapMetadata) + heapMetadataProx.size;
	}

	void* buffer = malloc(sizeof(t_posicion) + sizeof(t_HeapMetadata));
	memcpy(buffer, &puntero, sizeof(t_posicion));
	memcpy(buffer + sizeof(t_posicion), &heapMetadata, sizeof(t_HeapMetadata));

	send(socket_memoria, &pcb->pid, sizeof(t_pid), 0);

	if(heapMetadata.size == tamanioPag - sizeof(t_HeapMetadata))
		msg_enviar_separado(LIBERAR, sizeof(t_posicion) + sizeof(t_HeapMetadata), buffer, socket_memoria);
	else
		msg_enviar_separado(ESCRITURA_PAGINA, sizeof(t_posicion) + sizeof(t_HeapMetadata), buffer, socket_memoria);

	t_msg* msgRecibido = msg_recibir(socket_memoria);
	if(msgRecibido->tipoMensaje == LIBERAR){
		log_trace(logKernel, "Se LIBERO piola");
		flag_se_libero_pag = 1;
		_sumarCantPagsHeap(pcb->pid, -1);
		list_remove_and_destroy_by_condition(tabla_heap, (void*) _esHeap, free);
	}else if(msgRecibido->tipoMensaje != ESCRITURA_PAGINA){
		log_error(logKernel, "No recibi ESCRITURA_PAGINA o LIBERAR sino %d", msgRecibido->tipoMensaje);
		if(msgRecibido->tipoMensaje == STACKOVERFLOW)
			return -2;
		else
			return -1;
	}
	msg_destruir(msgRecibido);

	return flag_se_libero_pag;
}

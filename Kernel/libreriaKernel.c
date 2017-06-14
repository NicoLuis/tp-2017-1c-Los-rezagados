/*
 * libreriaKernel.c
 *
 *  Created on: 3/4/2017
 *      Author: utnso
 */


#include "libreriaKernel.h"

#include "operacionesPCBKernel.h"
#include "planificacion.h"

void mostrarArchivoConfig() {

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



t_PCB* recibir_pcb(int socket_cpu, t_msg* msgRecibido, bool flag_finalizado, bool flag_bloqueo){
	bool _esCPU2(t_infosocket* aux){
		return aux->socket == socket_cpu;
	}
	log_trace(logKernel, "Recibi PCB");
	t_PCB* pcb = desserealizarPCB(msgRecibido->data);
	_lockLista_PCB_cpu();
	list_remove_and_destroy_by_condition(lista_PCB_cpu, (void*) _esCPU2, free);
	_unlockLista_PCB_cpu();
	_sacarDeCola(pcb->pid, cola_Exec, mutex_Exec);
	if(flag_finalizado)
		finalizarPid(pcb->pid, 0);
	else{
		if(flag_bloqueo)
			_ponerEnCola(pcb->pid, cola_Block, mutex_Block);
		else{
			_ponerEnCola(pcb->pid, cola_Ready, mutex_Ready);
			sem_post(&sem_cantColaReady);
		}
	}
	return pcb;
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

		_lockLista_PCB_cpu();
		t_infosocket* aux = list_find(lista_PCB_cpu, (void*) _esCPU2);
		_unlockLista_PCB_cpu();
		int _es_PCB(t_PCB* p){
			return p->pid == aux->pid;
		}

		_lockLista_PCBs();
		t_PCB* pcb = list_find(lista_PCBs, (void*) _es_PCB);
		_unlockLista_PCBs();
		bool _esPid(t_infosocket* a){ return a->pid == pcb->pid; }

		t_VariableCompartida* varCompartida = malloc(sizeof(t_VariableCompartida));
		int _buscar_VarComp(t_VariableCompartida* p){
			return string_equals_ignore_case(p->nombre, varCompartida->nombre);
		}
		t_VariableSemaforo* varSemaforo = malloc(sizeof(t_VariableSemaforo));
		int _buscar_VarSem(t_VariableSemaforo* s){
			return string_equals_ignore_case(s->nombre, varSemaforo->nombre);
		}
		t_num tamanioNombre, offset = 0;

		switch(msgRecibido->tipoMensaje){
		case FINALIZAR_PROGRAMA:
			log_trace(logKernel, "Finalizo programa");
			flag_finalizado = true;
			//no break
		case ENVIO_PCB:		// si me devuelve el PCB es porque fue la ultima instruccion
			msg_recibir_data(socket_cpu, msgRecibido);
			pcb = recibir_pcb(socket_cpu, msgRecibido, flag_finalizado, 0);
			if(flag_finalizado){
				bool _esPid(t_infosocket* a){ return a->pid == pcb->pid; }
				_lockLista_PCB_consola();
				t_infosocket* info = list_find(lista_PCB_consola, (void*) _esPid);
				_unlockLista_PCB_consola();
				if(info == NULL)
					log_trace(logKernel, "No se ecuentra consola asociada a pid %d", pcb->pid);
				msg_enviar_separado(FINALIZAR_PROGRAMA, sizeof(t_num8), &info->pid, info->socket);
				sem_post(&sem_gradoMp);
			}
			_lockLista_PCBs();
			list_remove_by_condition(lista_PCBs, (void*) _es_PCB);
			list_add(lista_PCBs, pcb);
			_unlockLista_PCBs();
			cpuUsada->libre = true;
			pthread_mutex_unlock(&cpuUsada->mutex);
			sem_post(&sem_cantCPUs);
			break;
		case 0: case FIN_CPU:
			fprintf(stderr, "La cpu %d se ha desconectado \n", socket_cpu);
			log_trace(logKernel, "La desconecto la cpu %d", socket_cpu);
			//si la cpu se desconecto la saco de la lista
			_lockLista_cpus();
			list_remove_by_condition(lista_cpus, (void*) _esCPU);
			_unlockLista_cpus();
			_lockLista_PCB_cpu();
			list_remove_and_destroy_by_condition(lista_PCB_cpu, (void*) _esCPU2, free);
			_unlockLista_PCB_cpu();
			_sacarDeCola(pcb->pid, cola_Exec, mutex_Exec);
			setearExitCode(pcb->pid, SIN_DEFINICION);
			void _sacarDeSemaforos(t_VariableSemaforo* sem){
				t_num8 encontrado = _sacarDeCola(pcb->pid, sem->colaBloqueados, sem->mutex_colaBloqueados);
				if(encontrado == pcb->pid)
					sem->valorSemaforo++;
			}
			list_iterate(lista_variablesSemaforo, (void*) _sacarDeSemaforos);
			_lockLista_PCBs();
			list_remove_by_condition(lista_PCBs, (void*) _es_PCB);
			list_add(lista_PCBs, pcb);
			_unlockLista_PCBs();
			sem_post(&sem_gradoMp);
			pthread_mutex_unlock(&cpuUsada->mutex);
			pthread_exit(NULL);
			break;
		case ERROR:
			log_trace(logKernel, "Recibi ERROR");
			msg_recibir_data(socket_cpu, msgRecibido);
			pcb = recibir_pcb(socket_cpu, msgRecibido, 1, 0);
			pcb->exitCode = -11;	//-11: error de sintaxis en script
			_lockLista_PCBs();
			list_remove_by_condition(lista_PCBs, (void*) _es_PCB);
			list_add(lista_PCBs, pcb);
			_unlockLista_PCBs();
			cpuUsada->libre = true;
			sem_post(&sem_cantCPUs);
			_lockLista_PCB_consola();
			t_infosocket* info = list_find(lista_PCB_consola, (void*) _esPid);
			_unlockLista_PCB_consola();
			msg_enviar_separado(ERROR, sizeof(t_num8), &pcb->pid, info->socket);
			sem_post(&sem_gradoMp);
			pthread_mutex_unlock(&cpuUsada->mutex);
			break;
		case ESCRIBIR_FD:
			log_trace(logKernel, "Recibi ESCRIBIR_FD");
			msg_recibir_data(socket_cpu, msgRecibido);
			int size = msgRecibido->longitud - sizeof(t_puntero), fd;
			void* informacion = malloc(size);
			memcpy(&fd, msgRecibido->data, sizeof(t_puntero));
			memcpy(informacion, msgRecibido->data + sizeof(t_puntero), size);

			if(fd == 1){
				log_trace(logKernel, "Imprimo por consola: %s", informacion);
				_lockLista_PCB_consola();
				t_infosocket* info = list_find(lista_PCB_consola, (void*) _esPid);
				_unlockLista_PCB_consola();
				if(info == NULL)
					log_trace(logKernel, "No se ecuentra consola asociada a pid %d", pcb->pid);
				msg_enviar_separado(IMPRIMIR_TEXTO, size, informacion, info->socket);
				msg_enviar_separado(ESCRIBIR_FD, 0, 0, socket_cpu);
			}else{
				log_trace(logKernel, "Escribo en fd %d: %s", fd, informacion);
				//todo: lo trato como si fuese del FS
			}

			int _espidproc(t_infoProceso* a){ return a->pid == pcb->pid; }
			t_infoProceso* infP = list_remove_by_condition(infoProcs, (void*) _espidproc);
			infP->cantOpPriv++;
			list_add(infoProcs, infP);

			free(informacion);
			pthread_mutex_unlock(&cpuUsada->mutex);
			break;
		case GRABAR_VARIABLE_COMPARTIDA:
			log_trace(logKernel, "Recibi GRABAR_VARIABLE_COMPARTIDA");

			msg_recibir_data(socket_cpu, msgRecibido);
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
			log_trace(logKernel, "Recibi VALOR_VARIABLE_COMPARTIDA");

			msg_recibir_data(socket_cpu, msgRecibido);
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
			log_trace(logKernel, "Recibi SIGNAL");

			msg_recibir_data(socket_cpu, msgRecibido);
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
					t_num8 pidADesbloquear;
					void* tmp = malloc(sizeof(t_num8));
					pthread_mutex_lock(&semBuscado->mutex_colaBloqueados);
					tmp = queue_pop(semBuscado->colaBloqueados);
					pthread_mutex_unlock(&semBuscado->mutex_colaBloqueados);
					memcpy(&pidADesbloquear, tmp, sizeof(t_num8));
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
			log_trace(logKernel, "Recibi WAIT");

			msg_recibir_data(socket_cpu, msgRecibido);
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
					void* tmp = malloc(sizeof(t_num8));
					memcpy(tmp, &pcb->pid, sizeof(t_num8));
					pthread_mutex_lock(&semBuscad->mutex_colaBloqueados);
					queue_push(semBuscad->colaBloqueados, tmp);
					pthread_mutex_unlock(&semBuscad->mutex_colaBloqueados);
					log_trace(logKernel, "Bloqueo pid %d", pcb->pid);
					//recibo pcb
					msg_destruir(msgRecibido);
					msgRecibido = msg_recibir(socket_cpu);
					msg_recibir_data(socket_cpu, msgRecibido);
					pcb = recibir_pcb(socket_cpu, msgRecibido, 0, 1);
					_lockLista_PCBs();
					list_remove_by_condition(lista_PCBs, (void*) _es_PCB);
					list_add(lista_PCBs, pcb);
					_unlockLista_PCBs();
					cpuUsada->libre = true;
					pthread_mutex_unlock(&cpuUsada->mutex);
					sem_post(&sem_cantCPUs);
				}

				list_add(lista_variablesSemaforo, semBuscad);
				_sumarCantOpPriv(pcb->pid);
			}
			free(varSemaforo->nombre);
			pthread_mutex_unlock(&cpuUsada->mutex);
			break;
		case ABRIR_ANSISOP:
			log_trace(logKernel, "Recibi la siguiente operacion ABRIR_ANSISOP de CPU");
			msg_recibir_data(socket_cpu, msgRecibido);
			//t_num longitud_msj = 0;
			t_direccion_archivo pathArchivo;
			t_num longitudPath;
			t_banderas flags;
			t_num8 pid;
			memcpy(&longitudPath, msgRecibido->data, sizeof(t_num));
			pathArchivo = malloc(longitudPath) + 1;
			memcpy(pathArchivo, msgRecibido->data + sizeof(t_num), longitudPath);
			pathArchivo[longitudPath] = '\0';
			memcpy(&flags, msgRecibido->data + sizeof(t_num) + longitudPath, sizeof(t_banderas));
			memcpy(&pid, msgRecibido->data + sizeof(t_num) + longitudPath + sizeof(t_banderas),sizeof(t_num8));

//-------------- seteo variables de la tabla global --------------

			t_entrada_GlobalFile * entradaGlobalNew = malloc(sizeof(t_entrada_GlobalFile));


//-------------- busco si ya tengo este path en la tabla global ---------

			int _buscarPath(t_entrada_GlobalFile* a){ return string_equals_ignore_case(a->FilePath,pathArchivo); }
			t_entrada_GlobalFile* entradaGlobalOld = list_find(lista_tabla_global, (void*) _buscarPath);

			uint indiceActualGlobal;

			if(entradaGlobalOld == NULL){
				// si es nulo significa qe se abre el archivo por primera vez
				memcpy(entradaGlobalNew->FilePath, pathArchivo, longitudPath+1);

				//entradaGlobalNew->FilePath = pathArchivo;
				entradaGlobalNew->Open = 1;
				entradaGlobalNew->indiceGlobalTable = indiceGlobal;
				indiceGlobal++;
				indiceActualGlobal = indiceGlobal;
				list_add(lista_tabla_global,entradaGlobalNew);
			} else {
				// de lo contrario este se encuentra abierto y un proceso esta qeriendo abrirlo
				list_remove_by_condition(lista_tabla_global, (void*) _buscarPath);
				entradaGlobalOld->Open++;
				indiceActualGlobal= entradaGlobalOld->Open;
				list_add(lista_tabla_global,entradaGlobalOld);
			}

//-------------- seteo variable necesarias para la tabla de procesos ------------------------------

			t_entrada_proceso *entradaProcesoNew = malloc(sizeof(t_entrada_proceso));

			entradaProcesoNew->bandera =string_new();

			//considero el caso qe venga con + de 2 flags

			if(flags.lectura){
				entradaProcesoNew->bandera = "r";
			}
			if(flags.escritura){
				string_append(&entradaProcesoNew->bandera,"w");
			}
			if(flags.creacion){ //si lo crea entoncs puede leer y escribir
				string_append(&entradaProcesoNew->bandera,"c");
				msg_enviar_separado(CREAR_ARCHIVO, longitudPath, pathArchivo, socket_fs);//
				//crear archivo en fs
			}

			entradaProcesoNew->referenciaGlobalTable = indiceActualGlobal;

			//-------------- busco si ya tengo la tabla para este id de proceso -------------------------

			int _espid(t_tabla_proceso* a){ return a->pid == pid; }
			t_tabla_proceso* tablaProceso = list_find(lista_tabla_de_procesos, (void*) _esPid);

			// si la tabla del proceso no tiene entradas entoncs agrego una entrada nueva
			if(tablaProceso == NULL){
				entradaProcesoNew->indice = 3; // inicio indice en 3
				tablaProceso->indiceMax = 3;
				// en este momento ya estan cargados los flags, el indice y la referencia a tabla global

				list_add(tablaProceso->lista_entradas_de_proceso, entradaProcesoNew);

			} else {
				// defino indice de tabla

				entradaProcesoNew->indice = tablaProceso->indiceMax + 1;
				tablaProceso->indiceMax++;
				list_add(tablaProceso->lista_entradas_de_proceso, entradaProcesoNew);
			}

		}

		free(varCompartida);
		free(varSemaforo);
		msg_destruir(msgRecibido);
	}
}


void _sumarCantOpPriv(t_num8 pid){
	int _espidproc(t_infoProceso* a){ return a->pid == pid; }
	t_infoProceso* infP = list_remove_by_condition(infoProcs, (void*) _espidproc);
	infP->cantOpPriv++;	//fixme: no suma si no lo saco
	list_add(infoProcs, infP);
}

typedef struct {
	t_num8 pid;
	t_num size;
	char* script;
}_t_hiloEspera;

void enviarScriptAMemoria(_t_hiloEspera* aux){

	log_trace(logKernel, "Envio script a memoria");

	bool _buscarPCB(t_PCB* pcb){
		return pcb->pid == aux->pid;
	}
	bool _buscarConsola(t_infosocket* a){
		return a->pid == aux->pid;
	}

	_lockLista_PCB_consola();
	t_infosocket* a = list_find(lista_PCB_consola, (void*) _buscarConsola);
	_unlockLista_PCB_consola();
	int socketConsola = a->socket;

	sem_wait(&sem_gradoMp);
	_sacarDeCola(aux->pid, cola_New, mutex_New);

	_lockLista_PCBs();
	t_PCB* pcb = list_find(lista_PCBs, (void*) _buscarPCB);
	_unlockLista_PCBs();
	send(socket_memoria, &aux->pid, sizeof(t_num8), 0);
	msg_enviar_separado(INICIALIZAR_PROGRAMA, aux->size, aux->script, socket_memoria);
	send(socket_memoria, &pcb->cantPagsStack, sizeof(t_num8), 0);
	t_num8 respuesta;
	recv(socket_memoria, &respuesta, sizeof(t_num8), 0);
	switch(respuesta){
	case OK:
		log_trace(logKernel, "Recibi OK de memoria");
		pcb->cantPagsCodigo = (string_length(aux->script) / tamanioPag);
		pcb->cantPagsCodigo = (string_length(aux->script) % tamanioPag) == 0? pcb->cantPagsCodigo: pcb->cantPagsCodigo + 1;
		pcb->sp = pcb->cantPagsCodigo * tamanioPag;
		msg_enviar_separado(respuesta, sizeof(t_num8), &pcb->pid, socketConsola);
		t_infoProceso* infP = malloc(sizeof(t_infoProceso));
		bzero(infP, sizeof(t_infoProceso));
		infP->pid = aux->pid;
		list_add(infoProcs, infP);
		_ponerEnCola(aux->pid, cola_Ready, mutex_Ready);
		sem_post(&sem_cantColaReady);
		break;
	case MARCOS_INSUFICIENTES:
		log_trace(logKernel, "MARCOS INSUFICIENTES");
		msg_enviar_separado(MARCOS_INSUFICIENTES, 0, 0, socketConsola);
		setearExitCode(pcb->pid, RECURSOS_INSUFICIENTES);
		break;
	default:
		log_trace(logKernel, "RECIBI %d", respuesta);
	}
	free(aux->script);
	free(aux);
}

void atender_consola(int socket_consola){

	t_msg* msgRecibido = msg_recibir(socket_consola);

	void* script;	//si lo declaro adentro del switch se queja
	void _finalizarPIDs(t_infosocket* i){
		if(i->socket == socket_consola)
			finalizarPid(i->pid, DESCONEXION_CONSOLA);
	}
	switch(msgRecibido->tipoMensaje){

	case ENVIO_CODIGO:
		log_trace(logKernel, "Recibi script de consola %d", socket_consola);
		msg_recibir_data(socket_consola, msgRecibido);
		pid++;
		script = malloc(msgRecibido->longitud);
		memcpy(script, msgRecibido->data, msgRecibido->longitud);
		log_info(logKernel, "\n%s", script);
		t_num8 pidActual = crearPCB(socket_consola);
		llenarIndicesPCB(pidActual, script);

		t_infosocket* info = malloc(sizeof(t_infosocket));
		info->pid = pidActual;
		info->socket = socket_consola;

		_lockLista_PCB_consola();
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
		msg_recibir_data(socket_consola, msgRecibido);
		t_num8 pidAFinalizar;
		memcpy(&pidAFinalizar, msgRecibido->data, sizeof(t_num8));
		log_trace(logKernel, "Recibi FINALIZAR_PROGRAMA de consola %d - pid %d", socket_consola, pidAFinalizar);
		finalizarPid(pidAFinalizar, COMANDO_FINALIZAR);
		break;

	case 0: case DESCONECTAR:
		fprintf(stderr, "La consola %d se ha desconectado \n", socket_consola);
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
		char* comando = malloc(200);	//fixme: liberar?
		fgets(comando, 200, stdin);

		comando[strlen(comando)-1] = '\0';

		if(string_equals_ignore_case(comando, "listado procesos")){
			printf("Mostrar todos? (s/n) ");
			fgets(comando, 200, stdin);
			comando[strlen(comando)-1] = '\0';
			int i;

			if(string_starts_with(comando, "s") || string_starts_with(comando, "n")){
				printf("   ----  PID  ----   PC  ---- CantPags ---- ExitCode ----   Cola   ----   \n");
				_lockLista_PCBs();
				for(i = 0; i < list_size(lista_PCBs); i++){
					t_PCB* pcbA = list_get(lista_PCBs, i);
					char* cola = " -- ";
					char* exitCode;

					if((int)pcbA->exitCode > 0)
						exitCode = "-";
					else
						exitCode = string_from_format("%d", pcbA->exitCode);	//fixme: se caga en este if
					if(_estaEnCola(pcbA->pid, cola_New, mutex_New))
						cola = "NEW";
					if(_estaEnCola(pcbA->pid, cola_Ready, mutex_Ready))
						cola = "READY";
					if(_estaEnCola(pcbA->pid, cola_Exec, mutex_Exec))
						cola = "EXEC";
					if(_estaEnCola(pcbA->pid, cola_Block, mutex_Block))
						cola = "BLOCK";
					if(_estaEnCola(pcbA->pid, cola_Exit, mutex_Exit))
						cola = "EXIT";

					if(string_starts_with(comando, "s") || !string_equals_ignore_case(cola, " -- "))
						printf("   ----   %d   ----   %d   ----    %d     ----     %s    ----  %s  ----   \n",
							pcbA->pid, pcbA->pc, (pcbA->cantPagsCodigo + pcbA->cantPagsStack), exitCode, cola);
				}
				_unlockLista_PCBs();

			}else printf("Flasheaste era s/n \n");

		}else if(string_starts_with(comando, "obtener info ")){

			int pidPCB = atoi(string_substring_from(comando, 13));

			int _espid(t_infoProceso* a){ return a->pid == pidPCB; }
			t_infoProceso* infP = list_find(infoProcs, (void*) _espid);
			if(infP == NULL)
				printf( "No existe el pid %d \n", pidPCB);
			else{

				printf("Elegir opcion: \n"
						" a - Cantidad de rafagas ejecutadas \n"
						" b - Cantidad de operaciones privilegiadas que ejecutó \n"
						" c - Tabla de archivos abiertos por el proceso \n"
						" d - Cantidad de páginas de Heap utilizadas \n"
						" e - Cantidad de acciones alocar realizadas [cantidad de operaciones] \n"
						" f - Cantidad de acciones alocar realizadas [bytes] \n"
						" g - Cantidad de acciones liberar realizadas [cantidad de operaciones]  \n"
						" h - Cantidad de acciones liberar realizadas [bytes] \n");

				switch(getchar()){
				case 'a':
					printf( "Cantidad de rafagas ejecutadas: %d \n", infP->cantRafagas);	//todo: le preguto a cpu cuantas lleva?
					break;
				case 'b':
					printf( "Cantidad de operaciones privilegiadas que ejecutó: %d \n", infP->cantOpPriv);
					break;
				case 'c':
					printf( "Tabla de archivos abiertos por el proceso %d \n", infP->tablaArchivos);	//todo
					break;
				case 'd':
					printf( "Cantidad de páginas de Heap utilizadas: %d \n", infP->cantPagsHeap);
					break;
				case 'e':
					printf( "Cantidad de operaciones alocar: %d \n", infP->cantOp_alocar);
					break;
				case 'f':
					printf( "Se alocaron %d bytes \n", infP->canrBytes_alocar);
					break;
				case 'g':
					printf( "Cantidad de operaciones liberar: %d \n", infP->cantOp_liberar);
					break;
				case 'h':
					printf( "Se liberaron %d bytes \n", infP->canrBytes_liberar);
					break;
				default:
					printf( "Capo que me tiraste? \n");
				}

			}
		}else if(string_equals_ignore_case(comando, "tabla archivos")){

			//todo: implementar

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
				for(i = 0; i < valorSem - nuevoGMP; i++){	//fixme: averiguar si  gradoMultiprogramacion-valorSem
					pthread_create(&hiloEspera, &atributo,(void*) _esperarGradoMP, NULL);
				}
				pthread_attr_destroy(&atributo);

				gradoMultiprogramacion = nuevoGMP;
				fprintf(stderr, "Nuevo grado de multiprogramacion: %d\n", gradoMultiprogramacion);
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

					gradoMultiprogramacion = nuevoGMP;
					fprintf(stderr, "Nuevo grado de multiprogramacion: %d\n", gradoMultiprogramacion);
				}else{
					log_trace(logKernel, "Posteo gradoMP");
					for(i = 0; i < nuevoGMP - valorSem; i++){
						log_trace(logKernel, "Subo grado de MP");
						sem_post(&sem_gradoMp);
					}

					gradoMultiprogramacion = nuevoGMP;
					fprintf(stderr, "Nuevo grado de multiprogramacion: %d\n", gradoMultiprogramacion);
				}
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
				printf( "No existe el pid %d \n", pidKill);
			else{
				finalizarPid(pidKill, COMANDO_FINALIZAR);
				printf("Finalizo pid %d\n", pidKill);
			}

		}else if(string_equals_ignore_case(comando, "detener")){

			log_trace(logKernel, "Detengo planificacion");
			pthread_mutex_lock(&mut_detengo_plani);

		}else if(string_equals_ignore_case(comando, "reanudar")){

			log_trace(logKernel, "Reanudo planificacion");
			pthread_mutex_unlock(&mut_detengo_plani);

		}else if(string_equals_ignore_case(comando, "help")){
			printf("Comandos:\n"
					"● listado procesos: Obtener listado de procesos\n"
					"● obtener info [pid]: Obtener informacion de proceso\n"
					"● tabla archivos: Obtener informacion de proceso\n"
					"● grado mp [grado]: Setear grado de multiprogramacion\n"
					"● kill [pid]: Finalizar proceso\n"
					"● detener: Detener la planificación\n"
					"● reanudar: Reanuda la planificación\n");

		}else
			fprintf(stderr, "El comando '%s' no es valido\n", comando);

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



void finalizarPid(t_num8 pid, int exitCode){
	log_trace(logKernel, "Finalizo pid %d con exitCode %d", pid, exitCode);
	int _buscarPID(t_infosocket* i){
		return i->pid == pid;
	}
	_lockLista_PCB_cpu();
	t_infosocket* info = list_remove_by_condition(lista_PCB_cpu, (void*) _buscarPID);
	_unlockLista_PCB_cpu();
	if(info == NULL){
		log_info(logKernel, "No se encuentra cpu ejecutando pid %d", pid);
		_sacarDeCola(pid, cola_Block, mutex_Block);
		if(_sacarDeCola(pid, cola_Ready, mutex_Ready) == pid)
			sem_wait(&sem_cantColaReady);
	}
	else{
		//fixme: cuando ejecuta y finaliza explota
		_sacarDeCola(pid, cola_Exec, mutex_Exec);
		int _buscarCPU(t_cpu* c){
			return c->socket == info->socket;
		}
		_lockLista_cpus();
		t_cpu* cpu = list_find(lista_cpus, (void*) _buscarCPU);
		_unlockLista_cpus();
		if(cpu == NULL)
			log_info(logKernel, "No se encuentra cpu ejecutando pid %d", pid);
		else
			kill(cpu->pid, SIGUSR2);
		free(info);
	}

	info = list_remove_by_condition(lista_PCB_consola, (void*) _buscarPID);
	if(info != NULL)
		free(info);

	void _sacarDeSemaforos(t_VariableSemaforo* sem){
		t_num8 encontrado = _sacarDeCola(pid, sem->colaBloqueados, sem->mutex_colaBloqueados);
		if(encontrado == pid)
			sem->valorSemaforo++;
	}
	list_iterate(lista_variablesSemaforo, (void*) _sacarDeSemaforos);

	send(socket_memoria, &pid, sizeof(t_num8), 0);
	msg_enviar_separado(FINALIZAR_PROGRAMA, 0, 0, socket_memoria);
	setearExitCode(pid, exitCode);
	sem_post(&sem_gradoMp);
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

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
		sem_init(&semaforo->semaforo, 0, (int)valores[i]);
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
		if (strcmp("Hola soy la Consola", bufferEscucha) == 0)
			retorno = 1;
		else if (strcmp("Hola soy la CPU", bufferEscucha) == 0)
			retorno = 2;

		int bytesEnviados = send(socket_cliente, "Conexion Aceptada", 18, 0);
		if (bytesEnviados <= 0)
			retorno = -2;
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
	list_remove_and_destroy_by_condition(lista_PCB_cpu, (void*) _esCPU2, free);
	_sacarDeCola(pcb->pid, cola_Exec, mutex_Exec);
	sigoFIFO = 0;
	quantumRestante = 0;
	if(flag_finalizado){
		log_trace(logKernel, "Finalizo PCB");
		pcb->exitCode = 0;
		send(socket_memoria, &pcb->pid, sizeof(t_num8), 0);
		msg_enviar_separado(FINALIZAR_PROGRAMA, 0, 0, socket_memoria);
		_ponerEnCola(pcb->pid, cola_Exit, mutex_Exit);
		liberarPCB(pcb, true);
	}else{
		if(flag_bloqueo)
			_ponerEnCola(pcb->pid, cola_Block, mutex_Block);
		else{
			_ponerEnCola(pcb->pid, cola_Ready, mutex_Ready);
			sem_wait(&sem_cantColaReady);
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
	t_cpu* cpuUsada = list_find(lista_cpus, (void*) _esCPU);

	while(1){

		sem_wait(&cpuUsada->sem);
		bool flag_finalizado = false;

		t_msg* msgRecibido = msg_recibir(socket_cpu);

		t_infosocket* aux = list_find(lista_PCB_cpu, (void*) _esCPU2);
		int _es_PCB(t_PCB* p){
			return p->pid == aux->pid;
		}

		t_PCB* pcb = list_find(lista_PCBs, (void*) _es_PCB);
		bool _esPid(t_infosocket* a){ return a->pid == pcb->pid; }

		t_VariableCompartida* varCompartida = malloc(sizeof(varCompartida));
		int _buscar_VarComp(t_VariableCompartida* p){
			return string_equals_ignore_case(p->nombre, varCompartida->nombre);
		}
		t_num tamanioNombre, offset = 0;

		switch(msgRecibido->tipoMensaje){
		case OK:
			log_trace(logKernel, "Recibi OK");
			quantumRestante--;
			break;
		case FINALIZAR_PROGRAMA:
			log_trace(logKernel, "Finalizo programa");
			flag_finalizado = true;
			//no break
		case ENVIO_PCB:		// si me devuelve el PCB es porque fue la ultima instruccion
			msg_recibir_data(socket_cpu, msgRecibido);
			pcb = recibir_pcb(socket_cpu, msgRecibido, flag_finalizado, 0);
			if(flag_finalizado){
				bool _esPid(t_infosocket* a){ return a->pid == pcb->pid; }
				t_infosocket* info = list_find(lista_PCB_consola, (void*) _esPid);
				if(info == NULL)
					log_trace(logKernel, "No se ecuentra consola asociada a pid %d", pcb->pid);
				msg_enviar_separado(FINALIZAR_PROGRAMA, sizeof(t_num8), &info->pid, info->socket);
				sem_post(&sem_gradoMp);
			}
			list_remove_by_condition(lista_PCBs, (void*) _es_PCB);
			list_add(lista_PCBs, pcb);
			_sacarDeCola(pcb->pid, cola_Exec, mutex_Exec);
			cpuUsada->libre = true;
			sem_post(&sem_cantCPUs);
			break;
		case 0: case FIN_CPU:
			fprintf(stderr, "La cpu %d se ha desconectado \n", socket_cpu);
			log_trace(logKernel, "La desconecto la cpu %d", socket_cpu);
			//si la cpu se desconecto la saco de la lista
			list_remove_by_condition(lista_cpus, (void*) _esCPU);
			list_remove_and_destroy_by_condition(lista_PCB_cpu, (void*) _esCPU2, free);
			setearExitCode(pcb->pid, -20);
			_sacarDeCola(pcb->pid, cola_Exec, mutex_Exec);
			list_remove_by_condition(lista_PCBs, (void*) _es_PCB);
			list_add(lista_PCBs, pcb);
			sigoFIFO = 0;
			quantumRestante = 0;
			sem_post(&sem_gradoMp);
			pthread_exit(NULL);
			break;
		case ERROR:
			log_trace(logKernel, "Recibi ERROR");
			msg_recibir_data(socket_cpu, msgRecibido);
			pcb = recibir_pcb(socket_cpu, msgRecibido, 1, 0);
			pcb->exitCode = -11;	//-11: error de sintaxis en script
			list_remove_by_condition(lista_PCBs, (void*) _es_PCB);
			list_add(lista_PCBs, pcb);
			cpuUsada->libre = true;
			sem_post(&sem_cantCPUs);
			t_infosocket* info = list_find(lista_PCB_consola, (void*) _esPid);
			msg_enviar_separado(ERROR, sizeof(t_num8), &pcb->pid, info->socket);
			sem_post(&sem_gradoMp);
			break;
		case ESCRIBIR_FD:
			log_trace(logKernel, "Recibi ESCRIBIR_FD");
			msg_recibir_data(socket_cpu, msgRecibido);
			int size = msgRecibido->longitud - sizeof(t_puntero), fd;
			void* informacion = malloc(size);
			memcpy(&fd, msgRecibido->data, sizeof(t_puntero));
			memcpy(informacion, msgRecibido->data + sizeof(t_puntero), size);
			msg_destruir(msgRecibido);

			msgRecibido = msg_recibir(socket_cpu);
			msg_recibir_data(socket_cpu, msgRecibido);
			pcb = recibir_pcb(socket_cpu, msgRecibido, 0, 1);
			list_remove_by_condition(lista_PCBs, (void*) _es_PCB);
			list_add(lista_PCBs, pcb);
			cpuUsada->libre = true;
			sem_post(&sem_cantCPUs);
			if(fd == 1){
				log_trace(logKernel, "Imprimo por consola: %s", informacion);
				t_infosocket* info = list_find(lista_PCB_consola, (void*) _esPid);
				if(info == NULL)
					log_trace(logKernel, "No se ecuentra consola asociada a pid %d", pcb->pid);
				msg_enviar_separado(IMPRIMIR_TEXTO, size, informacion, info->socket);
			}else{
				log_trace(logKernel, "Escribo en fd %d: %s", fd, informacion);
				//todo: lo trato como si fuese del FS
			}
			sigoFIFO = 0;
			_sacarDeCola(pcb->pid, cola_Block, mutex_Block);
			_ponerEnCola(pcb->pid, cola_Ready, mutex_Ready);
			sem_post(&sem_cantColaReady);
			free(informacion);
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
			}
			free(varCompartida->nombre);
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
			}
			free(varCompartida->nombre);
			break;
		}

		free(varCompartida);
		msg_destruir(msgRecibido);
		pthread_mutex_unlock(&mut_planificacion);	//fixme: bug printear variable global
	}
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

	t_infosocket* a = list_find(lista_PCB_consola, (void*) _buscarConsola);
	int socketConsola = a->socket;

	sem_wait(&sem_gradoMp);
	_sacarDeCola(aux->pid, cola_New, mutex_New);

	t_PCB* pcb = list_find(lista_PCBs, (void*) _buscarPCB);
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
		setearExitCode(pcb->pid, -1);
		break;
	default:
		log_trace(logKernel, "RECIBI %d", respuesta);
	}
	free(aux->script);
	free(aux);
}

void atender_consola(int socket_consola){

	t_msg* msgRecibido = msg_recibir(socket_consola);
	msg_recibir_data(socket_consola, msgRecibido);

	log_trace(logKernel, "Recibi tipoMensaje %d de consola", msgRecibido->tipoMensaje);

	void* script;	//si lo declaro adentro del switch se queja
	switch(msgRecibido->tipoMensaje){

	case ENVIO_CODIGO:
		pid++;
		script = malloc(msgRecibido->longitud);
		memcpy(script, msgRecibido->data, msgRecibido->longitud);
		log_info(logKernel, "\n%s", script);
		t_num8 pidActual = crearPCB(socket_consola);
		llenarIndicesPCB(pidActual, script);

		t_infosocket* info = malloc(sizeof(t_infosocket));
		info->pid = pidActual;
		info->socket = socket_consola;

		list_add(lista_PCB_consola, info);

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

	case 0:
		fprintf(stderr, "La consola %d se ha desconectado \n", socket_consola);

		//si la consola se desconecto la saco de la lista
		bool _esConsola(int socketC){ return socketC == socket_consola; }
		list_remove_by_condition(lista_consolas, (void*) _esConsola);
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

		if(string_equals_ignore_case(comando, "listado procesos")){
			printf("Mostrar todos? (s/n) ");
			fgets(comando, 200, stdin);
			comando[strlen(comando)-1] = '\0';
			int i;

			if(string_starts_with(comando, "s") || string_starts_with(comando, "n")){
				printf("   ----  PID  ----   PC  ---- CantPags ---- ExitCode ----   Cola   ----   \n");
				for(i = 0; i < list_size(lista_PCBs); i++){
					t_PCB* pcbA = list_get(lista_PCBs, i);
					char* cola = " -- ";
					char* exitCode;

					if(pcbA->exitCode > 0)
						exitCode = "-";
					else
						exitCode = string_itoa(pcbA->exitCode);	//fixme: se caga en este if
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
						" h - Cantidad de acciones liberar realizadas [bytes] \n"
						" i - Cantidad de syscalls ejecutadas \n");

				switch(getchar()){
				case 'a':
					printf( "Cantidad de rafagas ejecutadas: %d \n", infP->cantRafagas);
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
				case 'i':
					printf( "Cantidad de syscalls ejecutadas: %d \n", infP->cantSyscalls);
					break;
				default:
					printf( "Capo que me tiraste? \n");
				}

			}
		}else if(string_equals_ignore_case(comando, "tabla archivos")){

			//todo: implementar

		}else if(string_starts_with(comando, "grado mp ")){

			int valorSem, nuevoGMP;
			nuevoGMP = atoi(string_substring_from(comando, 9));
			sem_getvalue(&sem_gradoMp, &valorSem);

			log_trace(logKernel, "Cambio grado multiprogramacion, anterior %d enEjecucion %d nuevo %d",
					gradoMultiprogramacion, gradoMultiprogramacion-valorSem, nuevoGMP);

			void _esperarGradoMP(){
				sem_wait(&sem_gradoMp);
			}

			if(nuevoGMP < gradoMultiprogramacion && nuevoGMP < valorSem){
				pthread_attr_t atributo;
				pthread_attr_init(&atributo);
				pthread_attr_setdetachstate(&atributo, PTHREAD_CREATE_DETACHED);
				pthread_t hiloEspera;
				while(valorSem > nuevoGMP){
					pthread_create(&hiloEspera, &atributo,(void*) _esperarGradoMP, NULL);
					sem_getvalue(&sem_gradoMp, &valorSem);
				}
				pthread_attr_destroy(&atributo);

				gradoMultiprogramacion = nuevoGMP;
				printf("Nuevo grado de multiprogramacion: %d\n", gradoMultiprogramacion);
			}else{
				sem_init(&sem_gradoMp, 0, nuevoGMP - valorSem);
				gradoMultiprogramacion = nuevoGMP;
				printf("Nuevo grado de multiprogramacion: %d\n", gradoMultiprogramacion);
			}

		}else if(string_starts_with(comando, "kill ")){

			int pidKill = atoi(string_substring_from(comando, 5));
			finalizarPid(pidKill);
			printf("Finalizo pid %d\n", pidKill);

		}else if(string_equals_ignore_case(comando, "detener")){

			log_trace(logKernel, "Detengo planificacion");
			pthread_mutex_lock(&mut_planificacion2);

		}else if(string_equals_ignore_case(comando, "reanudar")){

			log_trace(logKernel, "Reanudo planificacion");
			pthread_mutex_unlock(&mut_planificacion2);

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
		sem_destroy(&variable->semaforo);
		free(variable);
	}
	list_destroy_and_destroy_elements(lista_variablesSemaforo, (void*) _destruirSemaforos);

	FD_ZERO(&fdsMaestro);

	log_trace(logKernel, "Terminando Proceso");
	log_destroy(logKernel);
	exit(1);
}



void finalizarPid(int pid){
	int _buscarPID(t_infosocket* i){
		return i->pid == pid;
	}
	t_infosocket* info = list_find(lista_PCB_cpu, (void*) _buscarPID);
	if(info == NULL)
		log_info(logKernel, "No se encuentra cpu ejecutando pid %d", pid);

	int _buscarCPU(t_cpu* c){
		return c->socket == info->socket;
	}
	t_cpu* cpu = list_find(lista_cpus, (void*) _buscarCPU);
	if(cpu == NULL)
		log_info(logKernel, "No se encuentra cpu ejecutando pid %d", pid);

	kill(cpu->pid, SIGUSR2);
}






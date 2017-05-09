/*
 * libreriaKernel.c
 *
 *  Created on: 3/4/2017
 *      Author: utnso
 */


#include "libreriaKernel.h"

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
		variable->nombre = shared_vars[i];
		variable->valor = 0;
		list_add(lista_variablesSemaforo, variable);
	}
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

void escucharCPU(int socket_cpu) {

	while(1){
		void* mensajeRecibido = malloc(sizeof(uint8_t));
		int bytesRecibidos = recv(socket_cpu, mensajeRecibido, sizeof(uint8_t), 0);
		uint8_t tipoMensaje;
		memcpy(&tipoMensaje, mensajeRecibido, sizeof(uint8_t));

		if (bytesRecibidos <= 0 || tipoMensaje == FIN_CPU) {
			fprintf(stderr, "La cpu %d se ha desconectado \n", socket_cpu);

			//si la cpu se desconecto la saco de la lista
			bool _esCpu(int socketC){ return socketC == socket_cpu; }
			list_remove_by_condition(lista_cpus, (void*) _esCpu);
			pthread_exit(NULL);
		}
		free(mensajeRecibido);
	}

}

void atender_consola(int socket_consola){

	t_msg* msgRecibido = msg_recibir(socket_consola);
	msg_recibir_data(socket_consola, msgRecibido);

	if (msgRecibido->tipoMensaje == 0) {
		fprintf(stderr, "La consola %d se ha desconectado \n", socket_consola);

		//si la consola se desconecto la saco de la lista
		bool _esConsola(int socketC){ return socketC == socket_consola; }
		list_remove_by_condition(lista_consolas, (void*) _esConsola);
		close(socket_consola);
		FD_CLR(socket_consola, &fdsMaestro);
		return;
	}

	log_trace(logKernel, "Recibi tipoMensaje %d de consola", msgRecibido->tipoMensaje);

	char* script;	//si lo declaro adentro del switch se queja
	switch(msgRecibido->tipoMensaje){

	case ENVIO_CODIGO:
		pid++;
		script = (char*)msgRecibido->data;
		log_info(logKernel, script);
		int pidActual = crearPCB(socket_consola);
		if(enviarScriptAMemoria((uint32_t) pidActual, script) < 0)
			setearExitCode(pidActual, -1);
		break;

	}

}




int crearPCB(int socketConsola){

	t_PCB* pcb = malloc(sizeof(t_PCB));
	pcb->socketConsola = socketConsola;
	pcb->pid = pid;
	pcb->pc = 0;
	pcb->indiceCodigo = list_create();
	pcb->indiceStack = list_create();
	//todo: ver q pija son los indices

	list_add(lista_PCBs, pcb);
	return pid;
}



void borrarPCB(int pidActual){		//todo: hay q borrar pcb?
	bool _buscarPCB(t_PCB* pcb){
		return pcb->pid == pid;
	}

	void _liberarPCB(t_PCB* pcb){
		//free pcb->indiceCodigo;
		//free pcb->indiceEtiquetas;
		//free pcb->indiceStack;
		free(pcb);
	}

	list_remove_and_destroy_by_condition(lista_PCBs, (void*) _buscarPCB, (void*) _liberarPCB);
}

void setearExitCode(int pid, int exitCode){
	bool _buscarPCB(t_PCB* pcb){
		return pcb->pid == pid;
	}

	t_PCB* pcb = list_find(lista_PCBs, (void*) _buscarPCB);
	pcb->ec = exitCode;
}






int enviarScriptAMemoria(uint32_t pid, char* script){

	bool _buscarPCB(t_PCB* pcb){
		return pcb->pid == pid;
	}

	t_PCB* pcb = list_find(lista_PCBs, (void*) _buscarPCB);

	send(socket_memoria, &pid, sizeof(uint32_t), 0);
	msg_enviar_separado(INICIALIZAR_PROGRAMA, (uint32_t) string_length(script), script, socket_memoria);
	uint8_t respuesta;
	recv(socket_memoria, &respuesta, sizeof(uint8_t), 0);
	switch(respuesta){
	case OK:
		pcb->cantPags = (string_length(script) / tamanioPag);
		pcb->cantPags = (string_length(script) % tamanioPag) == 0? pcb->cantPags: pcb->cantPags + 1;
		send(pcb->socketConsola, &respuesta, sizeof(uint8_t), 0);
		send(pcb->socketConsola, &pcb->pid, sizeof(uint32_t), 0);
		return 1;
	case MARCOS_INSUFICIENTES:
		log_trace(logKernel, "MARCOS INSUFICIENTES");
		send(pcb->socketConsola, &respuesta, sizeof(uint8_t), 0);
		return -1;
	}
	return 0;
}



void consolaKernel(){

	/*
	 * Faltan implementar solo estos:
	1. Obtener el listado de procesos del sistema, permitiendo mostrar todos o solo los que estén
		en algún estado (cola).
	2. Obtener para un proceso dado:
		a. La cantidad de rafagas ejecutadas.
		b. La cantidad de operaciones privilegiadas que ejecutó.
		c. Obtener la tabla de archivos abiertos por el proceso.
		d. La cantidad de páginas de Heap utilizadas
		i. Cantidad de acciones alocar realizadas en cantidad de operaciones y en
		bytes
		ii. Cantidad de acciones liberar realizadas en cantidad de operaciones y en
		bytes
		e. Cantidad de syscalls ejecutadas
	3. Obtener la tabla global de archivos.
	5. Finalizar un proceso. En caso de que el proceso esté en ejecución, se deberá esperar a que la
		CPU devuelva el PCB actualizado para luego finalizarlo.
	6. Detener la planificación a fin de que no se produzcan cambios de estado de los procesos.
	*/

	while(1){
		char* comando = malloc(200);
		fgets(comando, 200, stdin);

		comando[strlen(comando)-1] = '\0';

		if(string_equals_ignore_case(comando, "listado procesos")){
			printf("Mostrar todos? (s/n) ");
			fgets(comando, 200, stdin);
			comando[strlen(comando)-1] = '\0';

			//todo: implementar
			if(string_equals_ignore_case(comando, "s")){
				printf("Muestro todos\n");
			}
			else if(string_starts_with(comando, "n")){
				printf("Muestro solo los que estén en algún estado\n");
			}else printf("Flasheaste era s/n \n");

		}else if(string_starts_with(comando, "obtener info ")){

			int pid = atoi(string_substring_from(comando, 13));

			//todo: implementar
			printf( "Cantidad de rafagas ejecutadas: %d \n"
					"Cantidad de operaciones privilegiadas que ejecutó: %d \n"
					//"Obtener la tabla de archivos abiertos por el proceso \n"
					"Cantidad de páginas de Heap utilizadas: %d \n"
					"Cantidad de acciones alocar realizadas en cantidad de operaciones y en bytes \n"
					"Cantidad de acciones liberar realizadas en cantidad de operaciones y en bytes \n"
					"Cantidad de syscalls ejecutadas \n",
					pid, pid, pid);		//esto no va pero me da paja

		}else if(string_equals_ignore_case(comando, "tabla archivos")){

			//todo: implementar

		}else if(string_starts_with(comando, "grado mp ")){

			gradoMultiprogramacion = atoi(string_substring_from(comando, 9));
			printf("Nuevo grado de multiprogramacion: %d\n", gradoMultiprogramacion);

		}else if(string_starts_with(comando, "kill ")){

			int pid = atoi(string_substring_from(comando, 5));
			//todo: implementar
			printf("Finalizo pid %d\n", pid);

		}else if(string_equals_ignore_case(comando, "detener")){

			//todo: implementar

		}else if(string_equals_ignore_case(comando, "help")){
			printf("Comandos:\n"
					"● listado procesos: Obtener listado de procesos\n"
					"● obtener info [pid]: Obtener informacion de proceso\n"
					"● tabla archivos: Obtener informacion de proceso\n"
					"● grado mp [grado]: Setear grado de multiprogramacion\n"
					"● kill [pid]: Finalizar proceso\n"
					"● detener: Detener la planificación\n");

		}else
			fprintf(stderr, "El comando '%s' no es valido\n", comando);

	}






}
void terminarKernel(){			//aca libero todos

	list_destroy(lista_cpus);
	list_destroy(lista_consolas);

	void _destruirStackMetadata(t_StackMetadata* stack){
		free(stack);
	}
	void _destruirIndiceStack(t_Stack* stack){
		list_destroy_and_destroy_elements(stack->args, (void*) _destruirStackMetadata);
		list_destroy_and_destroy_elements(stack->vars, (void*) _destruirStackMetadata);
		free(stack);
	}
	void _destruirPCBs(t_PCB* pcb){
		//todo: aca libero todos los elementos del pcb (mas q nada los indices)
		list_destroy(pcb->indiceCodigo);
		list_destroy_and_destroy_elements(pcb->indiceStack, (void*) _destruirIndiceStack);
		free(pcb);
	}
	list_destroy_and_destroy_elements(lista_PCBs, (void*) _destruirPCBs);

	void _destruirVariablesCompartidas(t_VariableCompartida* variable){
		free(variable);
	}
	list_destroy_and_destroy_elements(lista_variablesCompartidas, (void*) _destruirVariablesCompartidas);
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









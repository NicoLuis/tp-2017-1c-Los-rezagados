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

		if (bytesRecibidos <= 0 || tipoMensaje == CPU_TERMINO) {
			fprintf(stderr, "La cpu %d se ha desconectado \n", socket_cpu);

			//si la cpu se desconecto la saco de la lista
			bool _esCpu(int socketC){ return socketC == socket_cpu; }
			list_remove_by_condition(lista_cpus, (void*) _esCpu);
			pthread_exit(NULL);
		}
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

	case CONSOLA_ENVIA_PATH:
		script = (char*)msgRecibido->data;
		log_info(logKernel, script);
		crearPCB(socket_consola);
		break;

	}

}




void crearPCB(int socketConsola){

	t_PCB* pcb = malloc(sizeof(t_PCB));
	pcb->pid = pid;
	send(socketConsola, &pid, sizeof(uint32_t), 0);
	pid++;
	//todo: ver q pija son los indices

	list_add(lista_PCBs, pcb);

}


void terminarProceso(){

	list_destroy(lista_cpus);
	list_destroy(lista_consolas);
	void destruirPCBs(t_PCB* pcb){
		//todo: aca libero todos los elementos del pcb (mas q nada los indices)
		free(pcb);
	}
	list_destroy_and_destroy_elements(lista_PCBs, (void*) destruirPCBs);

	log_trace(logKernel, "Terminando Proceso");
	log_destroy(logKernel);
}









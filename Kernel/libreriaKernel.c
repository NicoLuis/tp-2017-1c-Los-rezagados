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

void escucharCPU(void* socketClienteCPU) {

	//Casteo socket_cpu
	int socket_cpu = (int) socketClienteCPU;

	list_add(lista_cpus, socketClienteCPU);

	printf("Se conecto una CPU\n");

	int bytesEnviados = send(socket_cpu, "Conexion Aceptada", 18, 0);
	if (bytesEnviados <= 0) {
		printf("Error send CPU");
		pthread_exit(NULL);
	}

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

void escucharConsola(void* socketCliente) {

	//Casteo socket_consola
	int socket_consola = (int) socketCliente;

	list_add(lista_consolas, socketCliente);

	printf("Se conecto una Consola \n");

	int bytesEnviados = send(socket_consola, "Conexion Aceptada", 18, 0);
	if (bytesEnviados <= 0) {
		printf("Error send Consola");
		pthread_exit(NULL);
	}


	// Espero por mensaje de Consola para reenviar a los demas procesos (segun pide el Checkpoint)
	while(1){

		t_msg* msgRecibido = msg_recibir(socket_consola);
		msg_recibir_data(socket_consola, msgRecibido);

		if (msgRecibido->tipoMensaje == 0) {
			fprintf(stderr, "La consola %d se ha desconectado \n", socket_consola);

			//si la consola se desconecto la saco de la lista
			bool _esConsola(int socketC){ return socketC == socket_consola; }
			list_remove_by_condition(lista_consolas, (void*) _esConsola);
			pthread_exit(NULL);
		}
		fprintf(stderr, "tipoMensaje %d\n", msgRecibido->tipoMensaje);
		fprintf(stderr, "longitud %d\n", msgRecibido->longitud);
		fprintf(stderr, "texto %s\n", (char*) msgRecibido->data);


		void _enviarACpus(int socketCpu) {
			send(socketCpu, msgRecibido->data, msgRecibido->longitud, 0);
		}
		list_iterate(lista_cpus, (void*) _enviarACpus);

	}

}

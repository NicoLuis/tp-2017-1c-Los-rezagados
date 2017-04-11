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
		void* buffer = malloc(200);		//el mensaje q recibi se guarda aca

		int bytesRecibidos = recv(socket_consola, buffer, 200, 0);

		if (bytesRecibidos <= 0) {
			fprintf(stderr, "El cliente se ha desconectado");

			//si la consola se desconecto la saco de la lista
			bool _esConsola(int socketC){ return socketC == socket_consola; }
			list_remove_by_condition(lista_consolas, (void*) _esConsola);
			pthread_exit(NULL);
		}

		//	reenvio lo q recibi

		send(socket_memoria, buffer, 200, 0);
		send(socket_fs, buffer, 200, 0);

		void _enviarACpus(int socketCpu) {
			send(socketCpu, buffer, 200, 0);
		}
		list_iterate(lista_cpus, (void*) _enviarACpus);

		//	muestro lo q recibi

		fprintf(stderr, "%s", (char*) buffer);
	}

}

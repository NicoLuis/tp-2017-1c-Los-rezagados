/*
 * libreriaMemoria.c
 *
 *  Created on: 3/4/2017
 *      Author: utnso
 */

#include "libreriaMemoria.h"

void mostrarArchivoConfig() {

	printf("El puerto de la MEMORIA es: %d\n", puertoMemoria);
	printf("La cantidad de Marcos es: %d\n", cantidadDeMarcos);
	printf("El tamaño de las Marcos es: %d\n", tamanioDeMarcos);
	printf("La cantidad de entradas de Cache habilitadas son: %d\n", cantidadaEntradasCache);
	printf("La cantidad maxima de entradas a la cache por proceso son: %d\n", cachePorProceso);
	printf("El algoritmo de reemplaco de la Cache es: %s\n", algoritmoReemplazo);
	printf("El retardo de la memoria es de: %d\n",retardoMemoria);

}


void escucharKERNEL(void* socket_kernel) {

	//Casteo socketNucleo
	int socketKernel = (int) socket_kernel;

	printf("Se conecto el Kernel\n");

	int bytesEnviados = send(socketKernel, "Conexion Aceptada", 18, 0);
	if (bytesEnviados <= 0) {
		printf("Error send Nucleo");
		pthread_exit(NULL);
	}

	// Espero por mensaje de Kernel para mostrar por pantalla (segun pide el Checkpoint)
	while(1){
		void* buffer = malloc(200);		//el mensaje q recibi se guarda aca

		int bytesRecibidos = recv(socketKernel, buffer, 200, MSG_WAITALL);
		if (bytesRecibidos <= 0) {
			printf("El cliente se ha desconectado");
			abort();
		}

		//	muestro lo q recibi
		fprintf(stderr, "%s", (char*) buffer);
	}

}

void escucharCPU(void* socket_cpu) {

	//Casteo socketCPU
	int socketCPU = (int) socket_cpu;

	printf("Se conecto un CPU\n");

	int bytesEnviados = send(socketCPU, "Conexion Aceptada", 18, 0);
	if (bytesEnviados <= 0) {
		printf("Error send CPU");
		pthread_exit(NULL);
	}

}

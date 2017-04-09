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

	printf("Se conecto una Consola \n");

	int bytesEnviados = send(socket_consola, "Conexion Aceptada", 18, 0);
	if (bytesEnviados <= 0) {
		printf("Error send Consola");
		pthread_exit(NULL);
	}
}

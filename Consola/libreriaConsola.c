/*
 * libreriaConsola.c
 *
 *  Created on: 9/4/2017
 *      Author: utnso
 */

#include "libreriaConsola.h"

void mostrarArchivoConfig() {

	printf("La IP del Kernel es: %s \n", ipKernel);
	printf("El puerto del Kernel es: %d\n", puertoKernel);
}


void finalizarPrograma(void* socket_kernel) {
	char buffer[200];
	int socket = (int) socket_kernel;
	printf("Ingrese mensaje:\n");
	scanf("%s", buffer);

	printf("EL mensaje ingresado es: %s\n",buffer);

	send(socket, buffer, 20, 0);

}

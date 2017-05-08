
#include "libreriaCPU.h"

void mostrarArchivoConfig() {

	printf("La IP del Kernel es: %s \n", ipKernel);
	printf("El puerto del Kernel es: %d\n", puertoKernel);
	printf("La IP de la Memorial es: %s \n", ipMemoria);
	printf("El puerto de la Memoria es: %d\n", puertoMemoria);
}


void terminar(){

	fprintf(stderr, "\nMe voy\n");
	uint8_t mensaje8 = FIN_CPU;
	void* mensaje = malloc(sizeof(uint8_t));
	memcpy(mensaje, &mensaje8, sizeof(uint8_t));

	if(socket_kernel != 0){
		send(socket_kernel, mensaje, sizeof(uint8_t), 0);
		close(socket_kernel);
	}

	if(socket_memoria != 0){
		send(socket_memoria, mensaje, sizeof(uint8_t), 0);
		close(socket_memoria);
	}

	exit(1);
}


#include "libreriaCPU.h"

void mostrarArchivoConfig() {

	printf("La IP del Kernel es: %s \n", ipKernel);
	printf("El puerto del Kernel es: %d\n", puertoKernel);
	printf("La IP de la Memorial es: %s \n", ipMemoria);
	printf("El puerto de la Memoria es: %d\n", puertoMemoria);
}


void terminar(){

	if(socket_kernel != 0){
		msg_enviar_separado(FIN_CPU, 1, 0, socket_kernel);
		close(socket_kernel);
	}

	if(socket_memoria != 0){
		msg_enviar_separado(FIN_CPU, 1, 0, socket_memoria);
		close(socket_memoria);
	}

	exit(1);
}

void ultimaEjec(){
	ultimaEjecucion = true;
}

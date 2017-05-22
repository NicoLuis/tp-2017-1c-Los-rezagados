
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

void ejecutar(){

}


t_puntero asignarMemoria(void* buffer, int size){

	t_puntero puntero;

	msg_enviar_separado(ASIGNACION_MEMORIA, size, buffer, socket_memoria);
	send(socket_memoria, &pcb->pid, sizeof(t_num8), 0);

	t_msg* msgRecibido = msg_recibir(socket_memoria);
	msg_recibir_data(socket_memoria, msgRecibido);

	switch(msgRecibido->tipoMensaje){
	case ASIGNACION_MEMORIA:
		puntero = (t_puntero) msgRecibido->data;
		log_error(logCPU, "Recibi %d", puntero);
		break;
	case 0:
		log_error(logCPU, "Se desconecto Memoria");
		break;
	}

	return puntero;
}
























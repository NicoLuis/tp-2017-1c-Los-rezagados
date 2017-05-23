
#include "libreriaCPU.h"
#include "pcb.h"

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


t_posicion asignarMemoria(void* buffer, int size){

	t_posicion puntero;
	int offset = 0, tmpsize;

	msg_enviar_separado(ASIGNACION_MEMORIA, size, buffer, socket_memoria);
	send(socket_memoria, &pcb->pid, sizeof(t_num8), 0);

	t_msg* msgRecibido = msg_recibir(socket_memoria);
	msg_recibir_data(socket_memoria, msgRecibido);

	switch(msgRecibido->tipoMensaje){
	case ASIGNACION_MEMORIA:
		memcpy(&puntero.pagina, msgRecibido->data + offset, tmpsize = sizeof(t_num8));
		offset += tmpsize;
		memcpy(&puntero.offset, msgRecibido->data + offset, tmpsize = sizeof(t_num8));
		offset += tmpsize;
		memcpy(&puntero.size, msgRecibido->data + offset, tmpsize = sizeof(t_num8));
		log_trace(logCPU, "Recibi %d %d %d", puntero.pagina, puntero.offset, puntero.size);
		break;
	case 0:
		log_error(logCPU, "Se desconecto Memoria");
		break;
	}
	msg_destruir(msgRecibido);
	return puntero;
}


void* _serializarEscribirValorMemoria(t_posicion puntero, t_num valor){
	int offset = 0, tmpsize;
	void* buffer = malloc(sizeof(t_num8)*4 + sizeof(t_valor_variable));

	memcpy(buffer + offset, &pcb->pid, tmpsize = sizeof(t_num8));
	offset += tmpsize;
	memcpy(buffer + offset, &puntero.pagina, tmpsize = sizeof(t_num8));
	offset += tmpsize;
	memcpy(buffer + offset, &puntero.offset, tmpsize = sizeof(t_num8));
	offset += tmpsize;
	memcpy(buffer + offset, &puntero.size, tmpsize = sizeof(t_num8));
	offset += tmpsize;
	memcpy(buffer + offset, &valor, tmpsize = sizeof(t_num));
	offset += tmpsize;

	return buffer;
}

t_posicion escribirMemoria(t_posicion puntero, t_valor_variable valor){

	void* buffer = _serializarEscribirValorMemoria(puntero, valor);
	msg_enviar_separado(ESCRITURA_PAGINA, sizeof(t_num8)*4 + sizeof(t_valor_variable), buffer, socket_memoria);
	free(buffer);

	t_num header;
	recv(socket_memoria, &header, sizeof(t_num), 0);

	switch(header){
	case ESCRITURA_PAGINA:
		log_trace(logCPU, "Asigno correctamente");
		break;
	case 0:
		log_error(logCPU, "Se desconecto Memoria");
		break;
	case FINALIZAR_PROGRAMA:
		log_error(logCPU, "Stack Overflow?");
		break;
	}

	return puntero;
}
























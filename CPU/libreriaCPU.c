
#include "libreriaCPU.h"
#include "pcb.h"
#include "ansisop.h"

AnSISOP_funciones functions = {
		.AnSISOP_asignar 				= asignar,
		.AnSISOP_asignarValorCompartida = asignarValorCompartida,
		.AnSISOP_definirVariable 		= definirVariable,
		.AnSISOP_dereferenciar 			= dereferenciar,
		.AnSISOP_finalizar 				= finalizar,
		.AnSISOP_irAlLabel 				= irAlLabel,
		.AnSISOP_llamarConRetorno 		= llamarConRetorno,
		.AnSISOP_llamarSinRetorno 		= llamarSinRetorno,
		.AnSISOP_obtenerPosicionVariable = obtenerPosicionVariable,
		.AnSISOP_obtenerValorCompartida = obtenerValorCompartida,
		.AnSISOP_retornar 				= retornar
};
AnSISOP_kernel kernel_functions = {
		.AnSISOP_abrir = abrir,
		.AnSISOP_borrar = borrar,
		.AnSISOP_cerrar = cerrar,
		.AnSISOP_escribir = escribir,
		.AnSISOP_leer = leer,
		.AnSISOP_liberar = liberar,
		.AnSISOP_moverCursor = moverCursor,
		.AnSISOP_reservar = reservar,
		.AnSISOP_signal = ansisop_signal,
		.AnSISOP_wait = ansisop_wait
};

void mostrarArchivoConfig() {

	printf("La IP del Kernel es: %s \n", ipKernel);
	printf("El puerto del Kernel es: %d\n", puertoKernel);
	printf("La IP de la Memorial es: %s \n", ipMemoria);
	printf("El puerto de la Memoria es: %d\n", puertoMemoria);
}


void finalizarCPU(){

	if(socket_kernel != 0){
		msg_enviar_separado(FIN_CPU, 1, 0, socket_kernel);
		close(socket_kernel);
	}

	if(socket_memoria != 0){
		msg_enviar_separado(FIN_CPU, 1, 0, socket_memoria);
		close(socket_memoria);
	}

	liberarPCB(pcb);
	exit(1);
}

void ultimaEjec(){
	ultimaEjecucion = true;
}


void ejecutarInstruccion(){
	log_info(logCPU, "PCB pid: %d", pcb->pid);

	char* instruccion = proximaInstruccion();

	if (instruccion != NULL) {
		log_trace(logCPU, "Instruccion recibida: %s", instruccion);
		analizadorLinea(instruccion, &functions, &kernel_functions);
		pcb->pc++;
	} else {
		log_error(logCPU, "No se pudo recibir la instruccion");
		return;
	}

	if(ultimaEjecucion){
		uint32_t size = tamanioTotalPCB(pcb);
		void* pcbSerializado = serializarPCB(pcb);
		msg_enviar_separado(ENVIO_PCB, size, pcbSerializado, socket_kernel);
		free(pcbSerializado);
	}
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


void* _serializarPuntero(t_posicion puntero){
	int offset = 0, tmpsize;
	void* buffer = malloc(sizeof(t_num8)*4);

	memcpy(buffer + offset, &pcb->pid, tmpsize = sizeof(t_num8));
	offset += tmpsize;
	memcpy(buffer + offset, &puntero.pagina, tmpsize = sizeof(t_num));
	offset += tmpsize;
	memcpy(buffer + offset, &puntero.offset, tmpsize = sizeof(t_num));
	offset += tmpsize;
	memcpy(buffer + offset, &puntero.size, tmpsize = sizeof(t_num));
	offset += tmpsize;

	return buffer;
}

t_posicion escribirMemoria(t_posicion puntero, t_valor_variable valor){

	void* buffer = _serializarPuntero(puntero);
	buffer = realloc(buffer, sizeof(t_num)*3 + sizeof(t_num8) + sizeof(t_valor_variable));
	memcpy(buffer + sizeof(t_num)*3 + sizeof(t_num8), &valor, sizeof(t_num));
	msg_enviar_separado(ESCRITURA_PAGINA, sizeof(t_num)*3 + sizeof(t_num8) + sizeof(t_valor_variable), buffer, socket_memoria);
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


char* proximaInstruccion() {

	char* proxInstruccion = NULL;
	t_intructions* instruction = malloc(sizeof(t_intructions));
	memcpy(instruction, pcb->indiceCodigo.bloqueSerializado + (pcb->pc * sizeof(t_intructions)), sizeof(t_intructions));

	int pagina = instruction->start / tamanioPagina;
	t_posicion puntero = {
			.pagina = pagina + pcb->cantPagsCodigo,
			.size = instruction->offset,									//offset -> es el size
			.offset = instruction->start - (tamanioPagina * pagina) 		//start dentro de la pagina
	};

	log_info(logCPU, "pagina %d start %d size %d", puntero.pagina, puntero.offset, puntero.size);

	void* buffer = _serializarPuntero(puntero);
	msg_enviar_separado(LECTURA_PAGINA, sizeof(t_num)*3 + sizeof(t_num8), buffer, socket_memoria);
	free(buffer);
	free(instruction);

	t_msg* msgRecibido = msg_recibir(socket_memoria);
	msg_recibir_data(socket_memoria, msgRecibido);

	switch(msgRecibido->tipoMensaje){
	case LECTURA_PAGINA:
		memcpy(proxInstruccion, msgRecibido->data, msgRecibido->longitud);
		return proxInstruccion;
		break;
	case STACKOVERFLOW:
		log_error(logCPU, "Pagina invalida"); break;
	case 0:
		log_error(logCPU, "memoria se desconecto");
	}

	return NULL;
}


















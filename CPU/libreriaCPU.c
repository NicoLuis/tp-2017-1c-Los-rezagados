
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

	printf("Nº log %d \n", process_getpid());
	printf("La IP del Kernel es: %s \n", ipKernel);
	printf("El puerto del Kernel es: %d\n", puertoKernel);
	printf("La IP de la Memorial es: %s \n", ipMemoria);
	printf("El puerto de la Memoria es: %d\n", puertoMemoria);
}


void finalizarCPU(){
	log_trace(logCPU, "  FIN CPU  ");
	log_trace(logAnsisop, "  FIN CPU  ");

	if(socket_kernel != 0){
		msg_enviar_separado(FIN_CPU, 0, 0, socket_kernel);
		close(socket_kernel);
	}

	if(socket_memoria != 0){
		msg_enviar_separado(FIN_CPU, 0, 0, socket_memoria);
		close(socket_memoria);
	}

	if(pcb != NULL)
		liberarPCB(pcb, false);

	log_destroy(logCPU);
	log_destroy(logAnsisop);
	config_destroy(configuracion);

	exit(1);
}

void ultimaEjecTotal(){
	log_info(logCPU, "Signal ultimaEjecucionTotal");
	ultimaEjec();
	flag_ultimaEjecucionTotal = 1;
}

void ultimaEjec(){
	log_info(logCPU, "Signal ultimaEjecucion");
	flag_finalizado = 1;
	flag_ultimaEjecucion = 1;
}


void ejecutar(){
	int quantumRestante = quantum;

	do{
		quantumRestante--;
		if(quantumRestante <= 0 && string_equals_ignore_case(algoritmo, "RR"))
			flag_ultimaEjecucion = 1;
		ejecutarInstruccion();
		pcb->cantRafagas++;
	} while(!flag_ultimaEjecucion);

	flag_finalizado = 0;
	flag_ultimaEjecucion = 0;
	liberarPCB(pcb, 0);
	pcb = NULL;
}


void ejecutarInstruccion(){
	log_info(logCPU, "PCB pid: %d", pcb->pid);
	char* instruccion = proximaInstruccion();

	if (instruccion != NULL) {
		log_trace(logCPU, "Instruccion recibida: %s", instruccion);
		log_trace(logAnsisop, "\t\t%s", instruccion);
		pcb->pc++;
		analizadorLinea(instruccion, &functions, &kernel_functions);
	} else {
		log_error(logCPU, "No se pudo recibir la instruccion");
		flag_error = 1;
	}

	if(flag_error){
		log_trace(logCPU, "Devuelvo ERROR");
		uint32_t size = tamanioTotalPCB(pcb);
		void* pcbSerializado = serializarPCB(pcb);
		msg_enviar_separado(ERROR, size, pcbSerializado, socket_kernel);
		free(pcbSerializado);
		flag_finalizado = 1;
		flag_ultimaEjecucion = 1;
		flag_error = 0;
	}
	else
	if(flag_ultimaEjecucion){
		log_trace(logCPU, "Ultima Instruccion");
		uint32_t size = tamanioTotalPCB(pcb);
		void* pcbSerializado = serializarPCB(pcb);
		if(flag_finalizado)
			msg_enviar_separado(FINALIZAR_PROGRAMA, size, pcbSerializado, socket_kernel);
		else
			msg_enviar_separado(ENVIO_PCB, size, pcbSerializado, socket_kernel);
		free(pcbSerializado);
	}
}

void* _serializarPuntero(t_posicion puntero){
	int offset = 0, tmpsize;
	void* buffer = malloc(sizeof(t_posicion) + sizeof(t_pid));

	memcpy(buffer + offset, &pcb->pid, tmpsize = sizeof(t_pid));
	offset += tmpsize;
	memcpy(buffer + offset, &puntero, tmpsize = sizeof(t_posicion));
	offset += tmpsize;

	return buffer;
}

t_valor_variable leerMemoria(t_posicion puntero){

	t_valor_variable valor;

	void* buffer = _serializarPuntero(puntero);
	msg_enviar_separado(LECTURA_PAGINA, sizeof(t_posicion) + sizeof(t_num8), buffer, socket_memoria);
	free(buffer);

	t_msg* msgRecibido = msg_recibir(socket_memoria);

	switch(msgRecibido->tipoMensaje){
	case LECTURA_PAGINA:
		if(msg_recibir_data(socket_memoria, msgRecibido) > 0)
			memcpy(&valor, msgRecibido->data, sizeof(t_valor_variable));
		else
			log_warning(logCPU, "No recibi nada");
		break;
	case 0:
		log_error(logCPU, "Se desconecto Memoria");
		break;
	case PAGINA_INVALIDA:
		log_error(logCPU, "Pagina %d invalida", puntero.pagina);
		break;
	}

	log_info(logCPU, "Valor %d", valor);
	msg_destruir(msgRecibido);
	return valor;
}

t_posicion escribirMemoria(t_posicion puntero, void* valor){

	int offset = 0, tmpsize;
	int sizeTotal = sizeof(t_pid) + sizeof(t_posicion) + puntero.size;
	void* buffer = malloc(sizeTotal);

	memcpy(buffer + offset, &pcb->pid, tmpsize = sizeof(t_pid));
	offset += tmpsize;
	memcpy(buffer + offset, &puntero, tmpsize = sizeof(t_posicion));
	offset += tmpsize;
	memcpy(buffer + offset, valor, tmpsize = puntero.size);
	offset += tmpsize;

	msg_enviar_separado(ESCRITURA_PAGINA, sizeTotal, buffer, socket_memoria);
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
	case PAGINA_INVALIDA:
		log_error(logCPU, "Pagina %d invalida", puntero.pagina);
		break;
	}

	return puntero;
}

t_posicion traducirSP(){
	t_posicion posicion;
	posicion.pagina = pcb->sp / tamanioPagina;
	posicion.offset = pcb->sp % tamanioPagina;
	return posicion;
}


char* proximaInstruccion() {

	char* proxInstruccion = NULL;
	t_intructions* instruction = malloc(sizeof(t_intructions));
	memcpy(instruction, pcb->indiceCodigo.bloqueSerializado + (pcb->pc * sizeof(t_intructions)), sizeof(t_intructions));

	int pagina = instruction->start / tamanioPagina;
	t_posicion puntero = {
			.pagina = pagina,
			.size = instruction->offset,									//offset -> es el size
			.offset = instruction->start - (tamanioPagina * pagina) 		//start dentro de la pagina
	};

	log_info(logCPU, "pagina %d start %d size %d", puntero.pagina, puntero.offset, puntero.size);

	void* buffer = _serializarPuntero(puntero);
	msg_enviar_separado(LECTURA_PAGINA, sizeof(t_posicion) + sizeof(t_num8), buffer, socket_memoria);
	free(buffer);
	free(instruction);

	t_msg* msgRecibido = msg_recibir(socket_memoria);

	switch(msgRecibido->tipoMensaje){
	case LECTURA_PAGINA:
		if(msg_recibir_data(socket_memoria, msgRecibido) > 0){
			log_info(logCPU, "Recibo contenido");
			proxInstruccion = malloc(msgRecibido->longitud);
			memcpy(proxInstruccion, msgRecibido->data, msgRecibido->longitud);
			proxInstruccion[msgRecibido->longitud-1] = '\0';
			msg_destruir(msgRecibido);
			return proxInstruccion;
		}else
			log_warning(logCPU, "No recibi nada");
		break;
	case STACKOVERFLOW:
		log_error(logCPU, "Pagina invalida"); break;
	case 0:
		log_error(logCPU, "memoria se desconecto"); break;
	default:
		log_error(logCPU, "cualquiera");
	}

	msg_destruir(msgRecibido);
	return NULL;
}


















/*
 * ansisop.c
 *
 *  Created on: 18/5/2017
 *      Author: utnso
 */

#include "ansisop.h"
#include "pcb.h"


t_puntero definirVariable(t_nombre_variable identificador_variable){

	log_trace(logAnsisop, "Definir variable %c", identificador_variable);

	if( pcb->sp + sizeof(t_StackMetadata) >= (pcb->cantPagsCodigo + pcb->cantPagsStack)*tamanioPagina ){
		log_error(logAnsisop, "STACKOVERFLOW");
		flag_error = true;
		return -1;
	}
	t_StackMetadata* metadata = malloc(sizeof(t_StackMetadata));
	metadata->id = identificador_variable;
	metadata->posicionMemoria = traducirSP();
	metadata->posicionMemoria.size = sizeof(t_nombre_variable);

	pcb->sp += sizeof(t_StackMetadata);

	t_Stack* stackActual = list_get(pcb->indiceStack, list_size(pcb->indiceStack)-1 );
	if(stackActual == NULL){
		stackActual = malloc(sizeof(t_Stack));
		stackActual->args = list_create();
		stackActual->vars = list_create();
		list_add(pcb->indiceStack, stackActual);
	}
	if(identificador_variable >= '0' && identificador_variable <= '9')
		list_add(stackActual->args, metadata);
	else
		list_add(stackActual->vars, metadata);

	t_puntero retorno = metadata->posicionMemoria.pagina * tamanioPagina + metadata->posicionMemoria.offset;
	log_trace(logAnsisop, "Puntero variable %c: %d", identificador_variable, retorno);
	return retorno;
}

t_valor_variable dereferenciar(t_puntero direccion_variable){

	if(direccion_variable >= 0){
		log_trace(logAnsisop, "Dereferenciar direccion variable %d", direccion_variable);

		t_posicion puntero;
		puntero.pagina = direccion_variable / tamanioPagina;
		puntero.offset = direccion_variable % tamanioPagina;
		puntero.size = sizeof(t_valor_variable);

		return leerMemoria(puntero);
	}else{
		log_error(logAnsisop, "No se puede obtener valor de %d", direccion_variable);
		flag_error = true;
		return -1;
	}
}


t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable){
	int i;

	log_trace(logAnsisop, "Obtener posicion variable %c", identificador_variable);

	t_Stack* stackActual = list_get(pcb->indiceStack, list_size(pcb->indiceStack)-1 );
	t_puntero retorno;

	if(identificador_variable >= '0' && identificador_variable <= '9')

		for(i = 0; i < list_size(stackActual->args); i++){
			t_StackMetadata* aux = list_get(stackActual->args, i);
			if(aux->id == identificador_variable){
				retorno = aux->posicionMemoria.pagina * tamanioPagina + aux->posicionMemoria.offset;
				log_trace(logAnsisop, "Puntero variable %c: %d", identificador_variable, retorno);
				return retorno;
			}
		}

	else

		for(i = 0; i < list_size(stackActual->vars); i++){
			t_StackMetadata* aux = list_get(stackActual->vars, i);
			if(aux->id == identificador_variable){
				retorno = aux->posicionMemoria.pagina * tamanioPagina + aux->posicionMemoria.offset;
				log_trace(logAnsisop, "Puntero variable %c: %d", identificador_variable, retorno);
				return retorno;
			}
		}

	log_error(logAnsisop, "No se encontro la variable %c", identificador_variable);
	flag_error = true;
	return -1;
}

void asignar(t_puntero direccion_variable, t_valor_variable valor){

	if(direccion_variable >= 0){
		log_trace(logAnsisop, "Asigno valor %d en direccion variable %d", valor, direccion_variable);

		t_posicion puntero;
		puntero.pagina = direccion_variable / tamanioPagina;
		puntero.offset = direccion_variable % tamanioPagina;
		puntero.size = sizeof(t_valor_variable);

		void* buffer = malloc(sizeof(t_valor_variable));
		memcpy(buffer, &valor, sizeof(t_valor_variable));
		escribirMemoria(puntero, buffer);
		free(buffer);
	}else{
		log_error(logAnsisop, "No se puede asignar en %d", direccion_variable);
		flag_error = true;
	}
}


void irAlLabel(t_nombre_etiqueta etiqueta){
	log_trace(logAnsisop, "Voy al label %s", etiqueta);

	t_puntero nro = metadata_buscar_etiqueta(etiqueta, pcb->indiceEtiquetas.bloqueSerializado, pcb->indiceEtiquetas.size);
	if(nro == -1){
		log_error(logAnsisop, "No encontre label %s", etiqueta);
		flag_error = true;
	}
	else
		pcb->pc = nro;
}

void llamarSinRetorno(t_nombre_etiqueta etiqueta){

	log_trace(logAnsisop, "Llamar sin retorno %s", etiqueta);

	t_Stack* stackActual = malloc(sizeof(t_Stack));
	stackActual->args = list_create();
	stackActual->vars = list_create();
	stackActual->retPos = pcb->pc;
	list_add(pcb->indiceStack, stackActual);

	t_puntero nro = metadata_buscar_etiqueta(etiqueta, pcb->indiceEtiquetas.bloqueSerializado, pcb->indiceEtiquetas.size);
	if(nro == -1){
		log_error(logAnsisop, "No encontre label %s", etiqueta);
		flag_error = true;
	}
	else
		pcb->pc = nro;
}

void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){

	log_trace(logAnsisop, "Llamar con retorno %s en %d", etiqueta, donde_retornar);

	t_Stack* stackActual = malloc(sizeof(t_Stack));
	stackActual->args = list_create();
	stackActual->vars = list_create();
	stackActual->retPos = pcb->pc;
	stackActual->retVar.pagina = donde_retornar / tamanioPagina;
	stackActual->retVar.offset = donde_retornar % tamanioPagina;
	stackActual->retVar.size = sizeof(t_valor_variable);
	pcb->sp += sizeof(t_posicion);

	list_add(pcb->indiceStack, stackActual);

	t_puntero nro = metadata_buscar_etiqueta(etiqueta, pcb->indiceEtiquetas.bloqueSerializado, pcb->indiceEtiquetas.size);
	if(nro == -1){
		log_error(logAnsisop, "No encontre label %s", etiqueta);
		flag_error = true;
	}
	else
		pcb->pc = nro;
}

void retornar(t_valor_variable retorno){

	log_trace(logAnsisop, "Retorno valor %d", retorno);

	t_Stack* stackActual = list_get(pcb->indiceStack, list_size(pcb->indiceStack) - 1);

	void* buffer = malloc(sizeof(t_valor_variable));
	memcpy(buffer, &retorno, sizeof(t_valor_variable));
	escribirMemoria(stackActual->retVar, buffer);
	free(buffer);
}

void finalizar(){

	log_trace(logAnsisop, "Finalizar contexto");

	t_Stack* stackActual = list_remove(pcb->indiceStack, list_size(pcb->indiceStack) - 1);

	if(list_size(pcb->indiceStack) == 0){
		log_trace(logCPU, "Finalizo ejecucion");
		pcb->cantRafagas++;
		flag_ultimaEjecucion = 1;
		flag_finalizado = 1;
	}else
		pcb->pc = stackActual->retPos;

	pcb->sp = pcb->sp - list_size(stackActual->args) * sizeof(t_StackMetadata);
	pcb->sp = pcb->sp - list_size(stackActual->vars) * sizeof(t_StackMetadata);

	list_destroy_and_destroy_elements(stackActual->args, free);
	list_destroy_and_destroy_elements(stackActual->vars, free);
	free(stackActual);

}


/*
 *
 *
 * KERNEL
 *
 *
 *
 */

t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
	//ariel
	log_trace(logAnsisop, "Asignar valor %d a variable %s", valor, variable);

	int longitud_buffer = 0;
	longitud_buffer = sizeof(t_num) + string_length(variable) + sizeof(t_valor_variable);
	void* buffer = malloc(longitud_buffer);
	t_num tamanioNombre = string_length(variable);

	memcpy(buffer, &tamanioNombre, sizeof(t_num));
	memcpy(buffer + sizeof(t_num), variable, tamanioNombre);
	memcpy(buffer + sizeof(t_num) + tamanioNombre, &valor, sizeof(t_valor_variable));

	if(buffer == NULL){
		log_error(logCPU, "error en la asignacion del buffer");
	}

	msg_enviar_separado(GRABAR_VARIABLE_COMPARTIDA, longitud_buffer, buffer, socket_kernel);
	free(buffer);

	t_msg* msgRecibido = msg_recibir(socket_kernel);

	if(msgRecibido->tipoMensaje == GRABAR_VARIABLE_COMPARTIDA){
		log_trace(logAnsisop, "Se asigno correctamente");
		msg_destruir(msgRecibido);
		return valor;
	}

	log_error(logAnsisop, "No se pudo asignar - se recibio %d", msgRecibido->tipoMensaje);
	flag_error = true;
	msg_destruir(msgRecibido);

	return -1;
}

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable){
	log_trace(logAnsisop, "Obtener valor de variable %s", variable);
	msg_enviar_separado(VALOR_VARIABLE_COMPARTIDA, string_length(variable), variable, socket_kernel);

	t_msg* msgRecibido = msg_recibir(socket_kernel);

	if(msgRecibido->tipoMensaje == VALOR_VARIABLE_COMPARTIDA){
		msg_recibir_data(socket_kernel, msgRecibido);
		t_valor_variable valor;
		memcpy(&valor, msgRecibido->data, sizeof(t_valor_variable));

		log_trace(logAnsisop, "Valor %d", valor);

		msg_destruir(msgRecibido);
		return valor;
	}else{
		log_error(logAnsisop, "Error al obtener variable - se recibio %d", msgRecibido->tipoMensaje);
		flag_error = true;
		msg_destruir(msgRecibido);
		return -1;
	}
}

t_descriptor_archivo abrir(t_direccion_archivo direccion, t_banderas flags){
	
	u_int fd = 0;
	int offset = 0, tmpsize;
	log_trace(logAnsisop, "abrir el siguiente archivo ubicado en: %s", direccion);
	int longitud_buffer = sizeof(t_num) + string_length(direccion) + sizeof(t_banderas) + sizeof(t_pid);
	void* buffer = malloc(longitud_buffer);
	t_num longitud_path = string_length(direccion);

	memcpy(buffer + offset, &longitud_path, tmpsize = sizeof(t_num));
	offset += tmpsize;
	memcpy(buffer + offset, direccion, tmpsize = longitud_path);
	offset += tmpsize;
	memcpy(buffer+ offset, &flags, tmpsize = sizeof(t_banderas));
	offset += tmpsize;
	memcpy(buffer+ offset, &pcb->pid, tmpsize = sizeof(t_pid));
	offset += tmpsize;

	if(buffer == NULL){
		log_trace(logCPU, "error en al cargar el buffer");
		flag_error = 1;// matar proceso CPU scaso de erro
	}
	msg_enviar_separado(ABRIR_ANSISOP, longitud_buffer, buffer, socket_kernel);

	t_msg* msgRecibido = msg_recibir(socket_kernel);

	if(msgRecibido->tipoMensaje == ABRIR_ANSISOP){
		msg_recibir_data(socket_kernel, msgRecibido);
		memcpy(&fd, msgRecibido->data, sizeof(t_descriptor_archivo));
		//fd = msgRecibido->data; //se recibe del kernel el fd que se asigno al proceso que abrio un archivo
		log_trace(logAnsisop, "Se asigno el fd correctamente");
	}else{
		log_error(logAnsisop, "Error - se recibio %d", msgRecibido->tipoMensaje);
		flag_error = 1;
	}

	msg_destruir(msgRecibido);
	free (buffer);
	return fd;
}

void mandarMsgaKernel(int tipoMensaje, t_descriptor_archivo fd){


	int longitud_buffer = sizeof(t_descriptor_archivo) + sizeof(t_pid);
	void* buffer = malloc(longitud_buffer);

	memcpy(buffer, &fd, sizeof(t_descriptor_archivo));
	memcpy(buffer + sizeof(t_descriptor_archivo), &pcb->pid, sizeof(t_pid));
	msg_enviar_separado(tipoMensaje, longitud_buffer, buffer, socket_kernel);

	free(buffer);

}

void borrar(t_descriptor_archivo direccion){

	log_trace(logAnsisop, "borrar el siguiente archivo ubicado en el siguiente file descriptor: %d", direccion);

	mandarMsgaKernel(BORRAR_ANSISOP, direccion);

	t_msg* msgRecibido = msg_recibir(socket_kernel);

	if(msgRecibido->tipoMensaje == BORRAR)
		log_trace(logAnsisop, "Borro correctamente");
	else
		log_error(logAnsisop, "Error al borrar");

	msg_destruir(msgRecibido);
}

void cerrar(t_descriptor_archivo descriptor_archivo){

	mandarMsgaKernel(CERRAR_ANSISOP, descriptor_archivo);

	//todo recibir confirmacion

}

void mandarMSGaKernel_LE(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio, int tipoMsj){
	int offset = 0, tmpsize;
	void* buf = malloc(sizeof(t_descriptor_archivo) + sizeof(t_valor_variable) + tamanio + sizeof(t_pid));

	memcpy(buf + offset, &descriptor_archivo, tmpsize = sizeof(t_descriptor_archivo));
	offset += tmpsize;
	memcpy(buf + offset, &tamanio, tmpsize = sizeof(t_valor_variable));
	offset += tmpsize;
	memcpy(buf + offset, informacion, tmpsize = tamanio);
	offset += tmpsize;
	memcpy(buf + offset, &pcb->pid, tmpsize = sizeof(t_pid));
	offset += tmpsize;

	msg_enviar_separado(tipoMsj, offset, buf, socket_kernel);

	free(buf);
}

void escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio){

	log_trace(logAnsisop, "Escribir en fd %d: %s", descriptor_archivo, informacion);

	mandarMSGaKernel_LE(descriptor_archivo, informacion, tamanio, ESCRIBIR_FD);

	t_msg* msgRecibido = msg_recibir(socket_kernel);

	if(msgRecibido->tipoMensaje == ESCRIBIR_FD)
		log_trace(logAnsisop, "Escribio bien");
	else{
		log_error(logAnsisop, "Error al escribir");
		flag_error = 1;
	}

	msg_destruir(msgRecibido);


	/*log_trace(logAnsisop, "Escribir en fd %d: %s", descriptor_archivo, informacion);
	int tmpsize = sizeof(t_descriptor_archivo);
	void* buf = malloc(tamanio + tmpsize);
	memcpy(buf, &descriptor_archivo, tmpsize);
	memcpy(buf + tmpsize, informacion, tamanio);
	msg_enviar_separado(ESCRIBIR_FD, tamanio + tmpsize, buf, socket_kernel);

	*/
}

void leer(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio){


	log_trace(logAnsisop, "Leer fd %d en %d size %d", descriptor_archivo, informacion, tamanio);
	int offset = 0, tmpsize;
	void* buff = malloc(sizeof(t_descriptor_archivo) + sizeof(t_valor_variable) + sizeof(t_pid));

	memcpy(buff + offset, &descriptor_archivo, tmpsize = sizeof(t_descriptor_archivo));
	offset += tmpsize;
	memcpy(buff + offset, &tamanio, tmpsize = sizeof(t_valor_variable));
	offset += tmpsize;
	memcpy(buff + offset, &pcb->pid, tmpsize = sizeof(t_pid));
	offset += tmpsize;

	msg_enviar_separado(LEER_ANSISOP, offset, buff, socket_kernel);

	free(buff);

	t_msg* msgDatosObtenidos = msg_recibir(socket_kernel);

	if(msgDatosObtenidos->tipoMensaje == OBTENER_DATOS){
		msg_recibir_data(socket_kernel, msgDatosObtenidos);

		log_trace(logCPU, "lei bien");

		if(informacion >= 0){
			//log_trace(logAnsisop, "Asigno valor %d en direccion variable %d", valor, direccion_variable);
			t_posicion puntero;
			puntero.pagina = informacion / tamanioPagina;
			puntero.offset = (informacion % tamanioPagina);
			//puntero.offset = (informacion % tamanioPagina) + sizeof(t_HeapMetadata);
			puntero.size = msgDatosObtenidos->longitud;

			escribirMemoria(puntero, msgDatosObtenidos->data);
		}
	}else{
		log_error(logCPU, "Error al leer");
		flag_error = 1;
	}
	msg_destruir(msgDatosObtenidos);

}

void moverCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion){

	log_trace(logAnsisop, "Mover Cursor fd %d posicion %d", descriptor_archivo,posicion );

	void* buffer = malloc(sizeof(t_descriptor_archivo) + sizeof(t_valor_variable));

	memcpy(buffer, &descriptor_archivo, sizeof(t_descriptor_archivo));
	memcpy(buffer + sizeof(t_descriptor_archivo), &posicion, sizeof(t_valor_variable));
	memcpy(buffer + sizeof(t_descriptor_archivo) + sizeof(t_valor_variable), &pcb->pid, sizeof(t_pid));

	msg_enviar_separado(MOVER_ANSISOP, sizeof(t_descriptor_archivo) + sizeof(t_descriptor_archivo), buffer, socket_kernel);
	free(buffer);




	/*
	* MOVER CURSOR DE ARCHIVO
	*
	* Informa al Kernel que el proceso requiere que se mueva el cursor a la posicion indicada.
	*
	* @syntax TEXT_SEEK_FILE (buscar)
	* @param descriptor_archivo Descriptor de archivo del archivo abierto
	* @param posicion Posicion a donde mover el cursor
	* @return void
	*/



}



t_puntero reservar(t_valor_variable espacio){
	log_trace(logAnsisop, "Reservo %d bytes", espacio);
	/*	no necesitaba pasarle el pcb
	int tmpsize = tamanioTotalPCB(pcb);
	void* buffer = malloc(tmpsize + sizeof(t_valor_variable));
	memcpy(buffer, &espacio, sizeof(t_valor_variable));
	void* tmpbuffer = serializarPCB(pcb);
	memcpy(buffer + sizeof(t_valor_variable), tmpbuffer, tmpsize);
	*/
	msg_enviar_separado(ASIGNACION_MEMORIA, sizeof(t_valor_variable), &espacio, socket_kernel);

	t_puntero direccion = -1;
	bool flag_se_agrego_pag;
	t_msg* msgRecibido = msg_recibir(socket_kernel);

	if(msgRecibido->tipoMensaje == ASIGNACION_MEMORIA){
		msg_recibir_data(socket_kernel, msgRecibido);
		memcpy(&direccion, msgRecibido->data, sizeof(t_puntero) );
		memcpy(&flag_se_agrego_pag, msgRecibido->data + sizeof(t_puntero), sizeof(bool));
		if(flag_se_agrego_pag){
			log_info(logAnsisop, "Se asigno una nueva pagina al heap");
			pcb->cantPagsHeap++;
		}
	}
	else{
		log_error(logAnsisop, "Error al reservar memoria - se recibio %d", msgRecibido->tipoMensaje);
		flag_error = true;
	}
	msg_destruir(msgRecibido);
	return direccion;
}

void liberar(t_puntero puntero){
	log_trace(logAnsisop, "Libero posicion %d", puntero);
	msg_enviar_separado(LIBERAR, sizeof(t_puntero), &puntero, socket_kernel);
	t_msg* msgRecibido = msg_recibir(socket_kernel);

	if(msgRecibido->tipoMensaje == LIBERAR){
		log_info(logAnsisop, "Se libero correctamente");
		if(msgRecibido->longitud == 1){
			log_info(logAnsisop, "Se libero una pagina al heap");
			pcb->cantPagsHeap--;
		}
	}
	else{
		log_error(logAnsisop, "Error al liberar memoria - se recibio %d", msgRecibido->tipoMensaje);
		flag_error = true;
	}
	msg_destruir(msgRecibido);
}

void ansisop_signal(t_nombre_semaforo identificador_semaforo){
	log_trace(logAnsisop, "Signal semaforo %s", identificador_semaforo);
	msg_enviar_separado(SIGNAL, string_length(identificador_semaforo), identificador_semaforo, socket_kernel);

	t_msg* msgRecibido = msg_recibir(socket_kernel);

	if(msgRecibido->tipoMensaje != SIGNAL){
		log_error(logAnsisop, "Error al hacer el signal - se recibio %d", msgRecibido->tipoMensaje);
		flag_error = true;
	}
	msg_destruir(msgRecibido);
}

void ansisop_wait(t_nombre_semaforo identificador_semaforo){
	log_trace(logAnsisop, "Wait semaforo %s", identificador_semaforo);
	msg_enviar_separado(WAIT, string_length(identificador_semaforo), identificador_semaforo, socket_kernel);

	t_msg* msgRecibido = msg_recibir(socket_kernel);

	if(msgRecibido->tipoMensaje == WAIT){
		msg_recibir_data(socket_kernel, msgRecibido);
		t_num valorSemaforoRecibido;
		memcpy(&valorSemaforoRecibido, msgRecibido->data, sizeof(t_num));
		log_trace(logAnsisop, "valorSemaforoRecibido %d", valorSemaforoRecibido);
		if((int)valorSemaforoRecibido < 0)
			flag_ultimaEjecucion = 1;
		msg_destruir(msgRecibido);
	}else{
		log_error(logAnsisop, "Error al hacer el wait - se recibio %d", msgRecibido->tipoMensaje);
		flag_error = true;
		msg_destruir(msgRecibido);
	}
}


















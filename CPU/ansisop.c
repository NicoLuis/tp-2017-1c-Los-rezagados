/*
 * ansisop.c
 *
 *  Created on: 18/5/2017
 *      Author: utnso
 */

#include "ansisop.h"
#include "pcb.h"


#define INICIOSTACK (pcb->cantPagsCodigo * tamanioPagina)

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

	t_puntero retorno = metadata->posicionMemoria.pagina * tamanioPagina + metadata->posicionMemoria.offset - INICIOSTACK;
	log_trace(logAnsisop, "Puntero variable %c: %d", identificador_variable, retorno);
	return retorno;
}

t_valor_variable dereferenciar(t_puntero direccion_variable){

	if(direccion_variable >= 0){
		log_trace(logAnsisop, "Dereferenciar direccion variable %d", direccion_variable);

		int offsetTotal = INICIOSTACK + direccion_variable;
		t_posicion puntero;
		puntero.pagina = offsetTotal / tamanioPagina;
		puntero.offset = offsetTotal % tamanioPagina;
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
				retorno = aux->posicionMemoria.pagina * tamanioPagina + aux->posicionMemoria.offset - INICIOSTACK;
				log_trace(logAnsisop, "Puntero variable %c: %d", identificador_variable, retorno);
				return retorno;
			}
		}

	else

		for(i = 0; i < list_size(stackActual->vars); i++){
			t_StackMetadata* aux = list_get(stackActual->vars, i);
			if(aux->id == identificador_variable){
				retorno = aux->posicionMemoria.pagina * tamanioPagina + aux->posicionMemoria.offset - INICIOSTACK;
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

		int offsetTotal = INICIOSTACK + direccion_variable;
		t_posicion puntero;
		puntero.pagina = offsetTotal / tamanioPagina;
		puntero.offset = offsetTotal % tamanioPagina;
		puntero.size = sizeof(t_valor_variable);

		escribirMemoria(puntero, valor);
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

	donde_retornar += INICIOSTACK;

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

	escribirMemoria(stackActual->retVar, retorno);
}

void finalizar(){

	log_trace(logAnsisop, "Finalizar contexto");

	t_Stack* stackActual = list_remove(pcb->indiceStack, list_size(pcb->indiceStack) - 1);

	if(list_size(pcb->indiceStack) == 0){
		log_trace(logCPU, "Finalizo ejecucion");
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
	msg_recibir_data(socket_kernel, msgRecibido);

	if(msgRecibido->tipoMensaje == VALOR_VARIABLE_COMPARTIDA){
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
	
	log_trace(logAnsisop, "abrir el siguiente archivo ubicado en: %s", direccion);
	int longitud_buffer = sizeof(t_num) + string_length(direccion) + sizeof(t_banderas) + sizeof(t_num8);
	void* buffer = malloc(longitud_buffer);
	t_num longitud_path = string_length(direccion);

	memcpy(buffer, &longitud_path, sizeof(t_num));
	memcpy(buffer + sizeof(t_num), direccion, longitud_path);
	memcpy(buffer+ string_length(direccion) + longitud_path, &flags, sizeof(t_banderas));
	memcpy(buffer+ string_length(direccion) + longitud_path + sizeof(t_banderas),&pcb->pid,sizeof(t_num8));
	if(buffer == NULL){
			log_trace(logCPU, "error en al cargar el buffer");
			flag_error=1;// matar proceso CPU scaso de erro
		}
	msg_enviar_separado(ABRIR_ANSISOP, longitud_buffer, buffer, socket_kernel);





	return 0;
}

void borrar(t_descriptor_archivo direccion){

}

void cerrar(t_descriptor_archivo descriptor_archivo){

}

void escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio){
	log_trace(logAnsisop, "Escribir en fd %d: %s", descriptor_archivo, informacion);
	int tmpsize = sizeof(t_descriptor_archivo);
	void* buf = malloc(tamanio + tmpsize);
	memcpy(buf, &descriptor_archivo, tmpsize);
	memcpy(buf + tmpsize, informacion, tamanio);
	msg_enviar_separado(ESCRIBIR_FD, tamanio + tmpsize, buf, socket_kernel);

	t_msg* msgRecibido = msg_recibir(socket_kernel);

	if(msgRecibido->tipoMensaje == ESCRIBIR_FD)
		log_trace(logAnsisop, "Escribio bien");
	else
		log_error(logAnsisop, "Error al escribir");

	msg_destruir(msgRecibido);
}

void leer(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio){

}

void liberar(t_puntero puntero){

}

void moverCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion){

}

t_puntero reservar(t_valor_variable espacio){

	return 0;
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
		int auxAsqueroso = valorSemaforoRecibido;
		if(auxAsqueroso < 0)
			flag_ultimaEjecucion = 1;
		msg_destruir(msgRecibido);
	}else{
		log_error(logAnsisop, "Error al hacer el wait - se recibio %d", msgRecibido->tipoMensaje);
		flag_error = true;
		msg_destruir(msgRecibido);
	}
}


















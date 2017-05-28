/*
 * ansisop.c
 *
 *  Created on: 18/5/2017
 *      Author: utnso
 */

#include "ansisop.h"
#include "pcb.h"

#define INICIOSTACK (pcb->cantPagsCodigo * tamanioPagina)


//fixme:
/*
este script no detecta la instruccion end -> toma la instruccion "prints n a" como "prints n a end"

begin
variables a
a = 20
prints n a
end
 */



t_puntero definirVariable(t_nombre_variable identificador_variable){

	log_trace(logAnsisop, "Definir variable %c", identificador_variable);

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
	}else
		log_error(logAnsisop, "No se puede obtener valor de %d", direccion_variable);
		return -1;
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
	}else
		log_error(logAnsisop, "No se puede asignar en %d", direccion_variable);
}


void irAlLabel(t_nombre_etiqueta etiqueta){
	log_trace(logAnsisop, "Voy al label %s", etiqueta);

	t_puntero nro = metadata_buscar_etiqueta(etiqueta, pcb->indiceEtiquetas.bloqueSerializado, pcb->indiceEtiquetas.size);
	if(nro == -1)
		log_error(logAnsisop, "No encontre label %s", etiqueta);
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
	if(nro == -1)
		log_error(logAnsisop, "No encontre label %s", etiqueta);
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
	if(nro == -1)
		log_error(logAnsisop, "No encontre label %s", etiqueta);
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

	pcb->pc = stackActual->retPos;
	pcb->sp = pcb->sp - list_size(stackActual->args) * sizeof(t_StackMetadata);
	pcb->sp = pcb->sp - list_size(stackActual->vars) * sizeof(t_StackMetadata);

	list_destroy_and_destroy_elements(stackActual->args, free);
	list_destroy_and_destroy_elements(stackActual->vars, free);
	free(stackActual);

	if(list_size(pcb->indiceStack) == 0){
		log_trace(logCPU, "Finalizo ejecucion");
		ultimaEjecucion = true;
		finalizado = true;
	}
}

t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){

	return 0;
}

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable){

	return 0;
}

/*
 *
 *
 * KERNEL
 *
 *
 *
 */


t_descriptor_archivo abrir(t_direccion_archivo direccion, t_banderas flags){

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
	ultimaEjecucion = true;
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

}
void ansisop_wait(t_nombre_semaforo identificador_semaforo){

}


















/*
 * ansisop.c
 *
 *  Created on: 18/5/2017
 *      Author: utnso
 */

#include "ansisop.h"

#define INICIOSTACK pcb->cantPagsCodigo * tamanioPagina


t_puntero definirVariable(t_nombre_variable identificador_variable){

	log_trace(logAnsisop, "Definir variable %c", identificador_variable);

	t_StackMetadata* metadata = malloc(sizeof(t_StackMetadata));
	metadata->id = identificador_variable;

	t_posicion puntero = asignarMemoria(&identificador_variable, sizeof(t_nombre_variable));
	metadata->posicionMemoria = puntero;

	t_Stack* stackActual = list_get(pcb->indiceStack, list_size(pcb->indiceStack)-1 );
	list_add(stackActual->vars, metadata);

	return puntero.pagina * tamanioPagina + puntero.offset - INICIOSTACK ;
}

t_valor_variable dereferenciar(t_puntero direccion_variable){

	return 0;
}


t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable){
	int i;

	log_trace(logAnsisop, "Obtener posicion variable %c", identificador_variable);

	t_Stack* stackActual = list_get(pcb->indiceStack, list_size(pcb->indiceStack)-1 );

	for(i = 0; i < list_size(stackActual->vars); i++){
		t_StackMetadata* aux = list_get(stackActual->vars, i);
		if(aux->id == identificador_variable){
			return aux->posicionMemoria.pagina * tamanioPagina + aux->posicionMemoria.offset - INICIOSTACK ;
		}
	}

	for(i = 0; i < list_size(stackActual->args); i++){
		t_StackMetadata* aux = list_get(stackActual->args, i);
		if(aux->id == identificador_variable){
			return aux->posicionMemoria.pagina * tamanioPagina + aux->posicionMemoria.offset - INICIOSTACK ;
		}
	}

	return -1;
}

void asignar(t_puntero direccion_variable, t_valor_variable valor){

	log_trace(logAnsisop, "Asigno valor %d en direccion variable %d", valor, direccion_variable);

	int offsetTotal = INICIOSTACK + direccion_variable;
	t_posicion puntero;
	puntero.pagina = offsetTotal / tamanioPagina;
	puntero.offset = offsetTotal % tamanioPagina;
	puntero.size = sizeof(t_valor_variable);

	escribirMemoria(puntero, valor);
}

void irAlLabel(t_nombre_etiqueta t_nombre_etiqueta){

}

void llamarSinRetorno(t_nombre_etiqueta etiqueta){

}

void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){

}

t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){

	return 0;
}

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable){

	return 0;
}

void retornar(t_valor_variable retorno){

}

void finalizar(){
	log_debug(logCPU, "Finalizo ejecucion");
	ultimaEjec();
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


















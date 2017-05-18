/*
 * ansisop.c
 *
 *  Created on: 18/5/2017
 *      Author: utnso
 */

#include "libreriaCPU.h"
#include <parser/parser.h>
#include <parser/metadata_program.h>

void asignar(t_puntero direccion_variable, t_valor_variable valor);
t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor);
t_puntero definirVariable(t_nombre_variable identificador_variable);
t_valor_variable dereferenciar(t_puntero direccion_variable);
void finalizar();
void irAlLabel(t_nombre_etiqueta t_nombre_etiqueta);
void llamarSinRetorno(t_nombre_etiqueta etiqueta);
void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);
t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable);
t_valor_variable obtenerValorCompartida(t_nombre_compartida variable);
void retornar(t_valor_variable retorno);

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

void asignar(t_puntero direccion_variable, t_valor_variable valor){

}

t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){

	return 0;
}

t_puntero definirVariable(t_nombre_variable identificador_variable){

	return 0;
}

t_valor_variable dereferenciar(t_puntero direccion_variable){

	return 0;
}

void finalizar(){

}

void irAlLabel(t_nombre_etiqueta t_nombre_etiqueta){

}

void llamarSinRetorno(t_nombre_etiqueta etiqueta){

}

void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){

}

t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable){


	return 0;
}

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable){

	return 0;
}

void retornar(t_valor_variable retorno){

}

/*
 * ansisop.h
 *
 *  Created on: 23/5/2017
 *      Author: utnso
 */

#ifndef ANSISOP_H_
#define ANSISOP_H_

#include "libreriaCPU.h"
#include "pcb.h"
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
t_descriptor_archivo abrir(t_direccion_archivo direccion, t_banderas flags);
void borrar(t_descriptor_archivo direccion);
void cerrar(t_descriptor_archivo descriptor_archivo);
void escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio);
void leer(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio);
void liberar(t_puntero puntero);
void moverCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion);
t_puntero reservar(t_valor_variable espacio);
void ansisop_signal(t_nombre_semaforo identificador_semaforo);
void ansisop_wait(t_nombre_semaforo identificador_semaforo);

#endif /* ANSISOP_H_ */

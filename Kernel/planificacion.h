/*
 * planificacion.h
 *
 *  Created on: 15/5/2017
 *      Author: utnso
 */

#ifndef PLANIFICACION_H_
#define PLANIFICACION_H_

#include "libreriaKernel.h"

int sigoFIFO;
int quantumRestante;

pthread_mutex_t mut_planificacion;

void planificar();
void planificar_FIFO();
void planificar_RR();


#endif /* PLANIFICACION_H_ */

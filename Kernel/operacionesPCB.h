/*
 * operacionesPCB.h
 *
 *  Created on: 15/5/2017
 *      Author: utnso
 */

#ifndef OPERACIONESPCB_H_
#define OPERACIONESPCB_H_

#include "libreriaKernel.h"

int crearPCB(int);
void llenarIndicesPCB(int, char*);
void setearExitCode(int, int);
bool _sacarDeCola(int pid, t_queue* cola);

#endif /* OPERACIONESPCB_H_ */

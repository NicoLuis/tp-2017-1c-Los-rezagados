/*
 * operacionesPCB.h
 *
 *  Created on: 15/5/2017
 *      Author: utnso
 */

#ifndef OPERACIONESPCBKERNEL_H_
#define OPERACIONESPCBKERNEL_H_

#include "libreriaKernel.h"
#include "pcb.h"

t_num8 crearPCB(int);
void llenarIndicesPCB(int, char*);
void setearExitCode(int, int);
bool _sacarDeCola(t_num8 pid, t_queue* cola);
bool _estaEnCola(t_num8 pid, t_queue* cola);

#endif /* OPERACIONESPCBKERNEL_H_ */

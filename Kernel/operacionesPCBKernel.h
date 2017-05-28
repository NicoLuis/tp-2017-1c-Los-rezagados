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
t_num8 _sacarDeCola(t_num8 pid, t_queue* cola, pthread_mutex_t mutex);
void _ponerEnCola(t_num8 pid, t_queue* cola, pthread_mutex_t mutex);
bool _estaEnCola(t_num8 pid, t_queue* cola, pthread_mutex_t mutex);

#endif /* OPERACIONESPCBKERNEL_H_ */

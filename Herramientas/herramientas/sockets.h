/*
 * sockets.h
 *
 *  Created on: 15/4/2017
 *      Author: utnso
 */

#ifndef HERRAMIENTAS_SOCKETS_H_
#define HERRAMIENTAS_SOCKETS_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>


typedef struct {
	uint8_t tipoMensaje;
	uint32_t longitud;
	void *data;
}__attribute__  ((packed)) t_msg;


int conectarAServidor(char* ipServidor, int puertoServidor);

int crearSocketDeEscucha(int puerto);

int aceptarCliente(int socketEscucha);


t_msg* msg_crear(uint8_t tipoMensaje);
void msg_destruir(t_msg* msg);
void msg_enviar_separado(uint8_t tipoMensaje, uint32_t longitud, void* data, int socketTo);
void msg_enviar(t_msg* msg, int socketTo);
t_msg* msg_recibir(int socketFrom);
void msg_recibir_data(int socketFrom, t_msg* msg);



#endif /* HERRAMIENTAS_SOCKETS_H_ */

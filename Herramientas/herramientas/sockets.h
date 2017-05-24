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
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#include <parser/parser.h>
#include <parser/metadata_program.h>

typedef uint32_t t_num;
typedef uint8_t t_num8;

typedef struct {
	uint8_t tipoMensaje;
	t_num longitud;
	void *data;
}__attribute__  ((packed)) t_msg;

typedef struct{
	t_num pagina;
	t_num offset;
	t_num size;
}t_posicion;

int conectarAServidor(char* ipServidor, int puertoServidor);

int crearSocketDeEscucha(char* puerto, t_log* conectar_select_log);

int aceptarCliente(int socketEscucha);


t_msg* msg_crear(uint8_t tipoMensaje);
void msg_destruir(t_msg* msg);
void msg_enviar_separado(uint8_t tipoMensaje, t_num longitud, void* data, int socketTo);
void msg_enviar(t_msg* msg, int socketTo);
t_msg* msg_recibir(int socketFrom);
void msg_recibir_data(int socketFrom, t_msg* msg);



#endif /* HERRAMIENTAS_SOCKETS_H_ */

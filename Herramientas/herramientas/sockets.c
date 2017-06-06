#include "sockets.h"

int conectarAServidor(char* ipServidor, int puertoServidor) {

	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr(ipServidor);
	direccionServidor.sin_port = htons(puertoServidor);

	int socketServidor = socket(AF_INET, SOCK_STREAM, 0);

	if (connect(socketServidor, (void*) &direccionServidor,sizeof(direccionServidor)) != 0) {
		perror("No se pudo conectar");
		return -1;
	}

	return socketServidor;
}

int crearSocketDeEscucha(char* puerto, t_log* log) {

	struct addrinfo hints, *ai, *p;
	int socketEscucha, activado = 1, rv;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; //netdb

	if ((rv = getaddrinfo(NULL, puerto, &hints, &ai)) != 0) {
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		log_error(log, "selectserver: %s\n", gai_strerror(rv));
	}

	for(p = ai; p != NULL; p = p->ai_next) {
		socketEscucha = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (socketEscucha < 0) continue;
		setsockopt(socketEscucha, SOL_SOCKET, SO_REUSEADDR, &activado,sizeof(int));

		if (bind(socketEscucha, p->ai_addr, p->ai_addrlen) < 0) {
			close(socketEscucha);
			continue;
		}
		break;
	}

	if (p == NULL){
		log_error(log, "fallo el bind");
		fprintf(stderr, "selectserver: failed to bind \n");
	}
	freeaddrinfo(ai);

	if (listen(socketEscucha, 10) < 0){
		log_error(log, "listen");
		perror("listen");
	}

	return socketEscucha;

}

int aceptarCliente(int socketEscucha) {

	struct sockaddr_in direccionCliente;
	unsigned int tamanioDireccion = sizeof(struct sockaddr_in);
	int socketCliente = accept(socketEscucha, (void*) &direccionCliente,&tamanioDireccion);
	return socketCliente;

}


/************************************/
/*			ENVIO 	MENSAJES		*/
/************************************/

t_msg* msg_crear(uint8_t tipoMensaje) {

	t_msg *msg = (t_msg *) malloc(sizeof(t_msg));
	msg->tipoMensaje = tipoMensaje;
	msg->longitud = 0;
	msg->data = NULL;

	return msg;
}

void msg_destruir(t_msg *msg) {
	if(msg->data != NULL)
		free(msg->data);
	free(msg);
}

void msg_enviar_separado(t_num8 tipoMensaje, t_num longitud, void* data, int socketTo){

	send(socketTo, &tipoMensaje, sizeof(t_num8), 0);
	send(socketTo, &longitud, sizeof(t_num), 0);
	send(socketTo, data, longitud, 0);

}

void msg_enviar(t_msg* msg, int socketTo){

	send(socketTo, &msg->tipoMensaje, sizeof(t_num8), 0);
	send(socketTo, &msg->longitud, sizeof(t_num), 0);
	send(socketTo, msg->data, msg->longitud, 0);

}

void msg_recibir_data(int socketFrom, t_msg* msg){

	msg->data = malloc(msg->longitud);
	bzero(msg->data, msg->longitud);
	recv(socketFrom, msg->data, msg->longitud, MSG_WAITALL);

}

t_msg* msg_recibir(int socketFrom){

	t_msg* msg = msg_crear(0);

	recv(socketFrom, &msg->tipoMensaje, sizeof(t_num8), MSG_WAITALL);
	recv(socketFrom, &msg->longitud, sizeof(t_num), MSG_WAITALL);

	return msg;
}



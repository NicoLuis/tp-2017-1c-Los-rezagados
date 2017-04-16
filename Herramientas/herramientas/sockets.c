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

int crearSocketDeEscucha(int puerto) {

	struct sockaddr_in direccion;
	direccion.sin_family = AF_INET;
	direccion.sin_addr.s_addr = INADDR_ANY;
	direccion.sin_port = htons(puerto);

	int socketEscucha = socket(AF_INET, SOCK_STREAM, 0);

	int activado = 1;
	setsockopt(socketEscucha, SOL_SOCKET, SO_REUSEADDR, &activado,sizeof(activado));

	if (bind(socketEscucha, (void*) &direccion, sizeof(direccion)) != 0) {
		perror("Falló el bind");
		return -1;
	}

	printf("Estoy escuchando\n");
	listen(socketEscucha, 100);

	return socketEscucha;

}

int aceptarCliente(int socketEscucha) {

	struct sockaddr_in direccionCliente;
	unsigned int tamanioDireccion = sizeof(struct sockaddr_in);
	int socketCliente = accept(socketEscucha, (void*) &direccionCliente,&tamanioDireccion);

	printf("Recibí una conexión en %d!!\n", socketCliente);

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
	free(msg->data);
	free(msg);
}

void msg_enviar_separado(uint8_t tipoMensaje, uint32_t longitud, void* data, int socketTo){

	send(socketTo, &tipoMensaje, sizeof(uint8_t), 0);
	send(socketTo, &longitud, sizeof(uint32_t), 0);
	send(socketTo, data, longitud, 0);

}

void msg_enviar(t_msg* msg, int socketTo){

	send(socketTo, &msg->tipoMensaje, sizeof(uint8_t), 0);
	send(socketTo, &msg->longitud, sizeof(uint32_t), 0);
	send(socketTo, msg->data, msg->longitud, 0);

}

void msg_recibir_data(int socketFrom, t_msg* msg){

	msg->data = malloc(msg->longitud);
	recv(socketFrom, msg->data, msg->longitud, MSG_WAITALL);

}

t_msg* msg_recibir(int socketFrom){

	t_msg* msg = msg_crear(0);

	recv(socketFrom, &msg->tipoMensaje, sizeof(uint8_t), 0);
	recv(socketFrom, &msg->longitud, sizeof(uint32_t), 0);

	return msg;
}



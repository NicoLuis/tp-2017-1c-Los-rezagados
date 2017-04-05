#include "../herramientas/sockets.h"

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


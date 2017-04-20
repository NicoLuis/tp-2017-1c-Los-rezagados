/*
 * libreriaKernel.c
 *
 *  Created on: 3/4/2017
 *      Author: utnso
 */


#include "libreriaKernel.h"

void mostrarArchivoConfig() {

	printf("El puerto del Programa es: %d\n", puertoPrograma);
	printf("El puerto de la CPU es: %d\n", puertoCPU);
	printf("La IP de la Memoria es: %s\n", ipMemoria);
	printf("El puerto de la Memoria es: %d\n", puertoMemoria);
	printf("La IP del File System es: %s\n", ipFileSystem);
	printf("El puerto del File System es: %d\n", puertoFS);
	printf("El Quantum es de: %d\n", quantum);
	printf("El Quantum de Sleep es de: %d\n", quantumSleep);
	printf("EL algoritmo de planificacion es: %s\n", algoritmo);
	printf("El grado de Multiprogramacion es: %d\n", gradoMultiprogramacion);
	//
	//
	//
	printf("El tamaño del Stack es de: %d\n", stackSize);
}



int handshake(int socket_cliente){

	char* bufferEscucha = malloc(200);
	int retorno = 0;

	send(socket_cliente, "Hola quien sos?", 16, 0);

	int bytesRecibidos = recv(socket_cliente, bufferEscucha, 50, 0);
	if (bytesRecibidos <= 0) {
		printf("El cliente se ha desconectado");
		return -1;
	}

	bufferEscucha[bytesRecibidos] = '\0';

	if (strcmp("Hola soy la Consola", bufferEscucha) == 0)
		retorno = 1;
	else if (strcmp("Hola soy la CPU", bufferEscucha) == 0)
		retorno = 2;

	int bytesEnviados = send(socket_cliente, "Conexion Aceptada", 18, 0);
	if (bytesEnviados <= 0)
		return -2;


	return retorno;
}

void escucharCPU(int socket_cpu) {

	while(1){
		void* mensajeRecibido = malloc(sizeof(uint8_t));
		int bytesRecibidos = recv(socket_cpu, mensajeRecibido, sizeof(uint8_t), 0);
		uint8_t tipoMensaje;
		memcpy(&tipoMensaje, mensajeRecibido, sizeof(uint8_t));

		if (bytesRecibidos <= 0 || tipoMensaje == CPU_TERMINO) {
			fprintf(stderr, "La cpu %d se ha desconectado \n", socket_cpu);

			//si la cpu se desconecto la saco de la lista
			bool _esCpu(int socketC){ return socketC == socket_cpu; }
			list_remove_by_condition(lista_cpus, (void*) _esCpu);
			pthread_exit(NULL);
		}
	}

}

void atender_consola(int socket_consola){

	t_msg* msgRecibido = msg_recibir(socket_consola);
	msg_recibir_data(socket_consola, msgRecibido);

	if (msgRecibido->tipoMensaje == 0) {
		fprintf(stderr, "La consola %d se ha desconectado \n", socket_consola);

		//si la consola se desconecto la saco de la lista
		bool _esConsola(int socketC){ return socketC == socket_consola; }
		list_remove_by_condition(lista_consolas, (void*) _esConsola);
		close(socket_consola);
		FD_CLR(socket_consola, &fdsMaestro);
	}else{

		fprintf(stderr, "tipoMensaje %d\n", msgRecibido->tipoMensaje);
		fprintf(stderr, "longitud %d\n", msgRecibido->longitud);
		fprintf(stderr, "texto %s\n", (char*) msgRecibido->data);
	}
}

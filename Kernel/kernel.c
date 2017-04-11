/*
 * kernel.c
 *
 *  Created on: 3/4/2017
 *      Author: utnso
 */


#include <herramientas/sockets.c>
#include "libreriaKernel.h"




int main(int argc, char* argv[]) {


	//Archivo de configuracion

	if (argc == 1) {
		printf("Debe ingresar la ruta del archivo en LC \n");
		return -1;
	}

	if (argc != 2) {
		printf("Numero incorrecto de argumentos \n");
		return -2;
	}


	//Inicializo listas
	lista_cpus = list_create();
	lista_consolas = list_create();

	//Cargo archivo de configuracion

	t_config* configuracion = config_create(argv[1]);
	puertoPrograma = config_get_int_value(configuracion, "PUERTO_PROG");
	puertoCPU = config_get_int_value(configuracion, "PUERTO_CPU");
	ipMemoria = config_get_string_value(configuracion, "IP_MEMORIA");
	puertoMemoria = config_get_int_value(configuracion, "PUERTO_MEMORIA");
	ipFileSystem = config_get_string_value(configuracion, "IP_FS");
	puertoFS = config_get_int_value(configuracion, "PUERTO_FS");
	quantum = config_get_int_value(configuracion, "QUANTUM");
	quantumSleep = config_get_int_value(configuracion, "QUANTUM_SLEEP");
	algoritmo = config_get_string_value(configuracion, "ALGORITMO");
	gradoMultiprogramacion = config_get_int_value(configuracion, "GRADO_MULTIPROG");
	stackSize = config_get_int_value(configuracion, "STACK_SIZE");

	//Muestro archivo de configuracion

	mostrarArchivoConfig();

	//-------------------------------CONEXION AL LA MEMORIA-------------------------------------

	printf("Me conecto a la Memoria\n");

	socket_memoria = conectarAServidor(ipMemoria, puertoMemoria);

	char* bufferMemoria = malloc(200);

	int bytesRecibidos = recv(socket_memoria, bufferMemoria, 50, 0);
	if (bytesRecibidos <= 0) {
		printf("La Memoria se ha desconectado\n");
	}

	bufferMemoria[bytesRecibidos] = '\0';

	printf("Recibi %d bytes con el siguiente mensaje: %s\n",bytesRecibidos, bufferMemoria);

	send(socket_memoria, "Hola soy el KERNEL", 19, 0);

	bytesRecibidos = recv(socket_memoria, bufferMemoria, 50, 0);

	if (bytesRecibidos <= 0) {
		printf("La Memoria se ha desconectado\n");
	}

	bufferMemoria[bytesRecibidos] = '\0';

	printf("Respuesta: %s\n", bufferMemoria);

	if (strcmp("Conexion aceptada", bufferMemoria) == 0) {
		printf("Me conecte correctamente a la Memoria\n");
	}



	//-------------------------------CONEXION AL FILE SYSTEM-------------------------------------

	printf("Me conecto al File System\n");

	socket_fs = conectarAServidor(ipFileSystem, puertoFS);

	char* bufferFS = malloc(200);

	bytesRecibidos = recv(socket_fs, bufferFS, 50, 0);
	if (bytesRecibidos <= 0) {
		printf("El File System se ha desconectado\n");
	}

	bufferFS[bytesRecibidos] = '\0';

	printf("Recibi %d bytes con el siguiente mensaje: %s\n",bytesRecibidos, bufferFS);

	send(socket_fs, "Hola soy el KERNEL", 19, 0);

	bytesRecibidos = recv(socket_fs, bufferFS, 50, 0);

	if (bytesRecibidos <= 0) {
		printf("El FIle System se ha desconectado\n");
	}

	bufferFS[bytesRecibidos] = '\0';

	printf("Respuesta: %s\n", bufferFS);

	if (strcmp("Conexion aceptada", bufferFS) == 0) {
		printf("Me conecte correctamente al File System\n");
	}



	//-----------------------------CREO HILO PARA CONEXION DE CPU'S Y CONSOLA-------------------


	//Creo un hilo para comunicarme con el nucleo
	pthread_t hilo_conexionConsola;

	//Atributo Detached
	pthread_attr_t atributo;
	pthread_attr_init(&atributo);
	pthread_attr_setdetachstate(&atributo, PTHREAD_CREATE_DETACHED);

	int socket_kernel = crearSocketDeEscucha(puertoPrograma);
		char* bufferEscucha = malloc(200);

		int falloP_thread;

		//CADA VEZ QUE ESCUCHA UNA NUEVA CONEXION CREA UN HILO, PREGUNTA SI ES UNA CPU O UNA CONSOLA
		//SEGUN QUIEN SEA EJECUTA LA FUNCION CORRESPONDIENTE:
		// - escucharCPU(int socket_cliente);
		// - escucharConsola(int socket_cliente);

		while (1) {
			int socket_cliente = aceptarCliente(socket_kernel);
			if ((socket_cliente) == -1) {

				printf("Error en el accept()");

				abort();
			}

			send(socket_cliente, "Hola quien sos?", 16, 0);

			int bytesRecibidos = recv(socket_cliente, bufferEscucha, 50, 0);
			if (bytesRecibidos <= 0) {

				printf("El cliente se ha desconectado");

				abort();
			}

			bufferEscucha[bytesRecibidos] = '\0';

			if (strcmp("Hola soy la Consola", bufferEscucha) == 0) {

				pthread_t* hiloCPU = malloc(sizeof(pthread_t));

				falloP_thread = pthread_create(hiloCPU, &atributo,(void*) escucharConsola, (void*) socket_cliente);
				if (falloP_thread < 0) {

					printf("Error Hilo CPU");

					abort();
				}

			} else if (strcmp("Hola soy la CPU", bufferEscucha) == 0) {

				falloP_thread = pthread_create(&hilo_conexionConsola, &atributo,(void*) escucharCPU, (void*) socket_cliente);
				if (falloP_thread < 0) {

					printf("Error Hilo Esucha Consola");

					abort();
				}
			}

		}



	return 0;

}

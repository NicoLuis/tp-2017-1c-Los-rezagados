/*
 * kernel.c
 *
 *  Created on: 3/4/2017
 *      Author: utnso
 */


#include "libreriaKernel.h"




int main(int argc, char* argv[]) {


	//Archivo de configuracion

	if (argc == 1) {
		printf("Debe ingresar la ruta del archivo en LC");
		return -1;
	}

	if (argc != 2) {
		printf("Numero incorrecto de argumentos");
		return -2;
	}


	//Cargo archivo de configuracion

	t_config* configuracion = config_create(argv[1]);
	puertoPrograma = config_get_int_value(configuracion, "PUERTO_PROG");
	puertoCPU = config_get_int_value(configuracion, "PUERTO_CPU");
	ipMemoria = config_get_string_value(configuracion, "IP_MEMORIA");
	puertoMemoria = config_get_int_value(configuracion, "PUERTO_MEMORIA");
	ipFileSystem = config_get_string_value(configuracion, "IP_FS");
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

	socket_fs = conectarAServidor(ipFileSystem, puertoFileSystem);

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


	return 0;

}

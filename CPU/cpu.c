#include <herramientas/sockets.c>
#include "libreriaCPU.h"



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
	ipKernel = config_get_string_value(configuracion, "IP_KERNEL");
	puertoKernel = config_get_int_value(configuracion, "PUERTO_KERNEL");
	ipMemoria = config_get_string_value(configuracion, "IP_MEMORIA");
	puertoMemoria = config_get_int_value(configuracion, "PUERTO_MEMORIA");

	//Muestro archivo de configuracion

	mostrarArchivoConfig();

	//-------------------------------CONEXION AL KERNEL-------------------------------------

	printf("Me conecto al Kernel\n");

	socket_kernel = conectarAServidor(ipKernel, puertoKernel);

	char* bufferKernel = malloc(200);

	int bytesRecibidos = recv(socket_kernel, bufferKernel, 50, 0);
	if (bytesRecibidos <= 0) {
		printf("El Kernel se ha desconectado\n");
	}

	bufferKernel[bytesRecibidos] = '\0';

	printf("Recibi %d bytes con el siguiente mensaje: %s\n",bytesRecibidos, bufferKernel);

	send(socket_kernel, "Hola soy la CPU", 16, 0);

	bytesRecibidos = recv(socket_kernel, bufferKernel, 50, 0);

	if (bytesRecibidos <= 0) {
		printf("El Kernel se ha desconectado\n");
	}

	bufferKernel[bytesRecibidos] = '\0';

	printf("Respuesta: %s\n", bufferKernel);

	if (strcmp("Conexion aceptada", bufferKernel) == 0) {
		printf("Me conecte correctamente al Kernel\n");
	}

	free(bufferKernel);

	//-------------------------------CONEXION AL LA MEMORIA-------------------------------------

	printf("Me conecto a la Memoria\n");

	socket_memoria = conectarAServidor(ipMemoria, puertoMemoria);

	char* bufferMemoria = malloc(200);

	bytesRecibidos = recv(socket_memoria, bufferMemoria, 50, 0);
	if (bytesRecibidos <= 0) {
		printf("La Memoria se ha desconectado\n");
	}

	bufferMemoria[bytesRecibidos] = '\0';

	printf("Recibi %d bytes con el siguiente mensaje: %s\n",bytesRecibidos, bufferMemoria);

	send(socket_memoria, "Hola soy la CPU", 16, 0);

	bytesRecibidos = recv(socket_memoria, bufferMemoria, 50, 0);

	if (bytesRecibidos <= 0) {
		printf("La Memoria se ha desconectado\n");
	}

	bufferMemoria[bytesRecibidos] = '\0';

	printf("Respuesta: %s\n", bufferMemoria);

	if (strcmp("Conexion aceptada", bufferMemoria) == 0) {
		printf("Me conecte correctamente a la Memoria\n");
	}

	free(bufferMemoria);


	// Espero por mensaje de Kernel para mostrar por pantalla (segun pide el Checkpoint)
	while(1){
		void* buffer = malloc(200);		//el mensaje q recibi se guarda aca

		int bytesRecibidos = recv(socket_kernel, buffer, 200, MSG_WAITALL);
		if (bytesRecibidos <= 0) {
			printf("El cliente se ha desconectado");
			abort();
		}

		//	muestro lo q recibi
		fprintf(stderr, "%s", (char*) buffer);
	}

	return 0;

}

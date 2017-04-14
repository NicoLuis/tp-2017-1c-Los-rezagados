#include <herramientas/sockets.c>
#include "libreriaConsola.h"



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


	//Muestro archivo de configuracion

	mostrarArchivoConfig();

	//-------------------------------CONEXION AL LA MEMORIA-------------------------------------

	printf("Me conecto al Kernel\n");

	socket_kernel = conectarAServidor(ipKernel, puertoKernel);

	char* bufferKernel = malloc(200);

	int bytesRecibidos = recv(socket_kernel, bufferKernel, 50, 0);
	if (bytesRecibidos <= 0) {
		printf("El Kernel se ha desconectado\n");
	}

	bufferKernel[bytesRecibidos] = '\0';

	printf("Recibi %d bytes con el siguiente mensaje: %s\n",bytesRecibidos, bufferKernel);

	send(socket_kernel, "Hola soy la Consola", 20, 0);

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


	// Creo el hilo que permite que la consola quede a la espera de comandos que se ingresen por linea de comandos
	// fixme: el hilo hay q crearlo cuando detecte el comando -> sino el main sigue y termina el proceso sin hacer nada

/*
	pthread_attr_t atributo;
	pthread_t hiloInterpreteDeComandos;
	pthread_attr_init(&atributo);
	pthread_attr_setdetachstate(&atributo, PTHREAD_CREATE_DETACHED);
	pthread_create(&hiloInterpreteDeComandos, &atributo,(void*) finalizarPrograma, (void *) socket_kernel);
	pthread_attr_destroy(&atributo);
*/

	while(1){
		void* mensaje = malloc(200);
		int socket = (int) socket_kernel;
		printf("Ingrese mensaje:\n");
		fgets(mensaje, 200, stdin);

		//todo: cuando lea el comando hay q comparar cual es y ahi creo el hilo q haga lo q tnga q hacer

		printf("El mensaje ingresado es: %s\n", (char*) mensaje);
		send(socket, mensaje, 200, 0);
	}

	return 0;
}

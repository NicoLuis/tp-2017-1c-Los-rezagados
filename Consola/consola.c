#include <sockets.c>
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

	signal (SIGINT, finalizarConsola);
	flag_desconexion = 0;

	//Creo archivo log
	logConsola = log_create(string_from_format("consola_%d.log", process_getpid()), "CONSOLA", false, LOG_LEVEL_TRACE);
	log_trace(logConsola, "  -----------  INICIO CONSOLA  -----------  ");

	//Cargo archivo de configuracion

	t_config* configuracion = config_create(argv[1]);
	ipKernel = config_get_string_value(configuracion, "IP_KERNEL");
	puertoKernel = config_get_int_value(configuracion, "PUERTO_KERNEL");

	lista_programas = list_create();

	//Muestro archivo de configuracion

	mostrarArchivoConfig();

	//-------------------------------CONEXION AL KERNEL-------------------------------------

	log_trace(logConsola, "Me conecto al Kernel");

	socket_kernel = conectarAServidor(ipKernel, puertoKernel);

	char* bufferKernel = malloc(200);

	int bytesRecibidos = recv(socket_kernel, bufferKernel, 50, 0);
	if (bytesRecibidos <= 0) {
		log_trace(logConsola, "El Kernel se ha desconectado");
	}

	bufferKernel[bytesRecibidos] = '\0';

	log_trace(logConsola, "Recibi %d bytes con el siguiente mensaje: %s",bytesRecibidos, bufferKernel);

	send(socket_kernel, "Hola soy la Consola", 20, 0);

	bytesRecibidos = recv(socket_kernel, bufferKernel, 50, 0);

	if (bytesRecibidos <= 0) {
		log_trace(logConsola, "El Kernel se ha desconectado");
	}

	bufferKernel[bytesRecibidos] = '\0';

	log_trace(logConsola, "Respuesta: %s", bufferKernel);

	if (strcmp("Conexion Aceptada", bufferKernel) == 0) {
		log_trace(logConsola, "Me conecte correctamente al Kernel");
	}

	free(bufferKernel);

	pthread_attr_t atributo;
	pthread_t hiloKernel;
	pthread_attr_init(&atributo);
	pthread_attr_setdetachstate(&atributo, PTHREAD_CREATE_DETACHED);
	pthread_create(&hiloKernel, &atributo,(void*) escucharKernel, NULL);
	pthread_attr_destroy(&atributo);

	while(1){
		void* mensaje = malloc(200);
		//printf("Ingrese mensaje:\n");
		fgets(mensaje, 200, stdin);

		leerComando(mensaje);
		free(mensaje);
	}

	return 0;
}


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

	pthread_attr_t atributo;
	pthread_t hiloInterpreteDeComandos;
	pthread_attr_init(&atributo);
	pthread_attr_setdetachstate(&atributo, PTHREAD_CREATE_DETACHED);
	pthread_create(&hiloInterpreteDeComandos, &atributo,(void*) finalizarPrograma, (void *) socket_kernel);
	pthread_attr_destroy(&atributo);



}

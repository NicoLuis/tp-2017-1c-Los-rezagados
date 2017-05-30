
#include <herramientas/sockets.c>
#include "libreriaFS.h"



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
	puertoFS = config_get_int_value(configuracion, "PUERTO");
	puntoMontaje = config_get_string_value(configuracion, "PUNTO_MONTAJE");
	if(puntoMontaje[string_length(puntoMontaje)-1] == '/')
		puntoMontaje[string_length(puntoMontaje)-1] = '\0';

	//Creo archivo log
	logFS = log_create("FS.log", "FS", false, LOG_LEVEL_TRACE);
	log_trace(logFS, "  -----------  INICIO FILE SYSTEM  -----------  ");

	//Muestro archivo de configuracion
	mostrarArchivoConfig();
	leerMetadataArchivo();
	leerBitMap();

	//Creo un hilo para comunicarme con el Kernel
		pthread_t hilo_conexionKERNEL;

		//Atributo Detached
		pthread_attr_t atributo;
		pthread_attr_init(&atributo);
		pthread_attr_setdetachstate(&atributo, PTHREAD_CREATE_DETACHED);

		//-------------CREAR UN SOCKET DE ESCUCHA PARA LAS CPU's Y EL KERNEL-------------------------
		int socket_fs = crearSocketDeEscucha(string_itoa(puertoFS), logFS);

 		char* bufferEscucha = malloc(200);

		int falloP_thread;

		while (1) {
			int socket_cliente = aceptarCliente(socket_fs);
			if ((socket_cliente) == -1) {

				fprintf(stderr, "Error en el accept()\n");

				abort();
			}

			send(socket_cliente, "Hola quien sos?", 16, 0);

			int bytesRecibidos = recv(socket_cliente, bufferEscucha, 50, 0);
			if (bytesRecibidos <= 0) {

				fprintf(stderr, "El cliente se ha desconectado\n");

				abort();
			}

			bufferEscucha[bytesRecibidos] = '\0';

			if (strcmp("Hola soy el KERNEL", bufferEscucha) == 0) {

				falloP_thread = pthread_create(&hilo_conexionKERNEL, &atributo,(void*) escucharKERNEL, (void*) socket_cliente);
				if (falloP_thread < 0) {

					fprintf(stderr, "Error Hilo Esucha Kernel\n");

					abort();
				}
			}

		}

		free(bufferEscucha);

		pthread_attr_destroy(&atributo);



	return 0;

}


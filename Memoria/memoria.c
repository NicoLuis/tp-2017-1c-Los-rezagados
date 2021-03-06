/*
 * memoria.c
 *
 *  Created on: 3/4/2017
 *      Author: utnso
 */


#include <sockets.c>
#include "libreriaMemoria.h"



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

	//Creo archivo de log

	log_memoria = log_create("memoria.log", "memoria_principal", false, LOG_LEVEL_TRACE);
	log_trace(log_memoria, "  -----------  INICIO MEMORIA  -----------  ");


	//Cargo archivo de configuracion

	t_config* configuracion = config_create(argv[1]);
	puertoMemoria = config_get_int_value(configuracion, "PUERTO");
	cantidadDeMarcos = config_get_int_value(configuracion, "MARCOS");
	tamanioDeMarcos = config_get_int_value(configuracion, "MARCO_SIZE");
	cantidadEntradasCache = config_get_int_value(configuracion, "ENTRADAS_CACHE");
	cachePorProceso = config_get_int_value(configuracion, "CACHE_X_PROC");
	algoritmoReemplazo = config_get_string_value(configuracion, "REEMPLAZO_CACHE");
	retardoMemoria = config_get_int_value(configuracion, "RETARDO_MEMORIA");

	//Muestro archivo de configuracion
	mostrarArchivoConfig();

	//reservo memoria para la memoria real
	memoria_real = calloc(cantidadDeMarcos, tamanioDeMarcos);

	// inicializo el resto
	lista_cpus = list_create();
	listaProcesos = list_create();
	inicializarFrames();

	//Verifico si la Cache esta habilitada
	if (Cache_Activada()) {
		Cache = crearCache();
		log_info(log_memoria, "La Cache esta Habilitada");
	}

	//Defino señales
	signal (SIGINT, terminarMemoria);


	//Creo un hilo para comunicarme con el Kernel
	pthread_t hilo_conexionKERNEL;
	//Atributo Detached
	pthread_attr_t atributo;
	pthread_attr_init(&atributo);
	pthread_attr_setdetachstate(&atributo, PTHREAD_CREATE_DETACHED);

	//Creo hilo de la consola
	pthread_t hiloConsola;

	pthread_create(&hiloConsola, &atributo, (void *) ejecutarComandos,NULL);

	//-------------CREAR UN SOCKET DE ESCUCHA PARA LAS CPU's Y EL NUCLEO-------------------------
	int socketMemoria = crearSocketDeEscucha(string_itoa(puertoMemoria), log_memoria);
	char* bufferEscucha = malloc(200);

	int falloP_thread;

	//CADA VEZ QUE ESCUCHA UNA NUEVA CONEXION CREA UN HILO, PREGUNTA SI ES UNA CPU O EL KERNEL
	//SEGUN QUIEN SEA EJECUTA LA FUNCION CORRESPONDIENTE:
	// - escucharCPU(int socket_cliente);
	// - escucharKERNEL(int socket_cliente);

	while (1) {
		int socket_cliente = aceptarCliente(socketMemoria);
		if ((socket_cliente) == -1) {

			log_error(log_memoria,"Error en el accept()");

			abort();
		}

		send(socket_cliente, "Hola quien sos?", 16, 0);

		int bytesRecibidos = recv(socket_cliente, bufferEscucha, 50, 0);
		if (bytesRecibidos <= 0) {

			log_info(log_memoria,"El cliente se ha desconectado");

			abort();
		}

		bufferEscucha[bytesRecibidos] = '\0';

		if (strcmp("Hola soy la CPU", bufferEscucha) == 0) {

			pthread_t* hiloCPU = malloc(sizeof(pthread_t));

			falloP_thread = pthread_create(hiloCPU,&atributo,(void*) escucharCPU, (void*) socket_cliente);
			if (falloP_thread < 0) {

				log_error(log_memoria,"Error Hilo CPU");

				abort();
			}

		} else if (strcmp("Hola soy el KERNEL", bufferEscucha) == 0) {

			falloP_thread = pthread_create(&hilo_conexionKERNEL, &atributo,(void*) escucharKERNEL, (void*) socket_cliente);
			if (falloP_thread < 0) {

				log_error(log_memoria,"Error Hilo Escucha Nucleo");

				abort();
			}
		}

	}

	pthread_attr_destroy(&atributo);

	return 0;

}

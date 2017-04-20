/*
 * kernel.c
 *
 *  Created on: 3/4/2017
 *      Author: utnso
 */


#include <herramientas/sockets.c>
#include "libreriaKernel.h"

int conectar_select(char*);


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



	conectar_select(string_itoa(puertoPrograma));




	return 0;

}

int conectar_select(char* puerto_escucha) {

	fd_set descriptoresLectura;
	int socket_cliente_aceptado, current_fd;
	t_log* conectar_select_log = log_create("kernel.log", "conectar_select", false, LOG_LEVEL_TRACE);
	log_trace(conectar_select_log, "Iniciando select");
	FD_ZERO(&fdsMaestro);
	FD_ZERO(&descriptoresLectura);

	int servidor = crearSocketDeEscucha(puerto_escucha, conectar_select_log);
	FD_SET(servidor, &fdsMaestro);
	int max_fds = servidor;

	pthread_t hilo_conexionCPU;
	//Atributo Detached
	pthread_attr_t atributo;
	pthread_attr_init(&atributo);
	pthread_attr_setdetachstate(&atributo, PTHREAD_CREATE_DETACHED);

	while (1) {
		descriptoresLectura = fdsMaestro;

		if (select(max_fds + 1, &descriptoresLectura, NULL, NULL, NULL) == -1) {
			if(errno==EINTR)
				select(max_fds + 1, &descriptoresLectura, NULL, NULL, NULL);
			else
				log_error(conectar_select_log, "Error en el select");
		}

		for (current_fd  = 0; current_fd  <= max_fds; current_fd++) {
			if (FD_ISSET(current_fd , &descriptoresLectura)) {
				if (current_fd  == servidor) {

					if ((socket_cliente_aceptado = aceptarCliente(servidor)) == -1){
						log_error(conectar_select_log, "Error al aceptar conexion");
						perror("Error al aceptar conexion");
					} else {
// Aca entro cuando entro una nueva conexion		-----------------------------------------------------------------

						if (socket_cliente_aceptado > max_fds) max_fds = socket_cliente_aceptado;
						log_trace(conectar_select_log, "El select recibio una conexion en el socket %d", socket_cliente_aceptado);

						int tipo = handshake(socket_cliente_aceptado);

						switch(tipo){


							case 1:
// Aca entro cuando entro una nueva consola		-----------------------------------------------------------------
								FD_SET(socket_cliente_aceptado, &fdsMaestro);
								list_add(lista_consolas, &socket_cliente_aceptado);

								break;
							case 2:
// Aca entro cuando entro una nueva cpu			-----------------------------------------------------------------

								if (pthread_create(&hilo_conexionCPU, &atributo,(void*) escucharCPU, (void*) socket_cliente_aceptado) < 0) {
									log_error(conectar_select_log, "Error Hilo Escucha Consola");
									abort();
								}

								list_add(lista_cpus, &socket_cliente_aceptado);

								break;
							case -1:
								log_error(conectar_select_log, "Error en el handshake: el cliente se desconecto"); break;

							case -2:
								log_error(conectar_select_log, "Error en el handshake: send"); break;


						}

					}

				} else
					atender_consola(current_fd);		//todo: ver q pasa con las conexiones de CPU
			}
		}
	}
	return 0;
}


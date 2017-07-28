/*
 * kernel.c
 *
 *  Created on: 3/4/2017
 *      Author: utnso
 */


#include <sockets.c>
#include "libreriaKernel.h"
#include "planificacion.h"

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

	//Inicializo listas pid y colas
	lista_cpus = list_create();
	lista_consolas = list_create();
	lista_PCBs = list_create();
	lista_PCB_consola = list_create();
	lista_PCB_cpu = list_create();
	infoProcs = list_create();
	tabla_heap = list_create();
	pid = 0;
	indiceGlobal = 0;
	cola_New = queue_create();
	cola_Ready = queue_create();
	cola_Exec = queue_create();
	cola_Block = queue_create();
	cola_Exit = queue_create();
	flag_logCancer = 1;

	lista_tabla_de_procesos =  list_create(); //capa FS creo lista de estructuras de procesos
	lista_tabla_global = list_create(); //capa FS creo lista de estructuras globales

	//Creo archivo log
	logKernel = log_create("kernel.log", "KERNEL", false, LOG_LEVEL_TRACE);
	log_trace(logKernel, "  -----------  INICIO KERNEL  -----------  ");

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
	stackSize = (t_num8) config_get_int_value(configuracion, "STACK_SIZE");
	char** ids = config_get_array_value(configuracion, "SEM_IDS");
	char** valores = config_get_array_value(configuracion, "SEM_INIT");
	char** shared_vars = config_get_array_value(configuracion, "SHARED_VARS");
	inicializarSemaforosYVariables(ids, valores, shared_vars);

	//Muestro archivo de configuracion

	mostrarArchivoConfig();


	//Defino señales
	signal (SIGINT, terminarKernel);




	//-------------------------------CONEXION AL LA MEMORIA-------------------------------------

	log_trace(logKernel, "Me conecto a Memoria");
	socket_memoria = conectarAServidor(ipMemoria, puertoMemoria);

	int retornoHandshake = handshake(socket_memoria, 1);
	if(retornoHandshake == -1){
		log_trace(logKernel, "La Memoria se ha desconectado");
		exit(-1);
	}
	else if(retornoHandshake == -2){
		log_trace(logKernel, "Error en handshake, no es memoria");
		exit(-1);
	}
	log_trace(logKernel, "Me conecte bien a Memoria");

	recv(socket_memoria, &tamanioPag, sizeof(t_num), 0);
	log_trace(logKernel, "Tamaño de paginas: %d", tamanioPag);


	//-------------------------------CONEXION AL FILE SYSTEM-------------------------------------
	log_trace(logKernel, "Me conecto al File System");
	socket_fs = conectarAServidor(ipFileSystem, puertoFS);

	retornoHandshake = handshake(socket_fs, 1);
	if(retornoHandshake == -1){
		log_trace(logKernel, "El File System se ha desconectado");
		exit(-1);
	}
	else if(retornoHandshake == -2){
		log_trace(logKernel, "Error en handshake, no es file system");
		exit(-1);
	}
	log_trace(logKernel, "Me conecte bien al File System");


	//-------------------------------HILO CONSOLA DE KERNEL-------------------------------------

	pthread_attr_t atributo;
	pthread_attr_init(&atributo);
	pthread_attr_setdetachstate(&atributo, PTHREAD_CREATE_DETACHED);
	pthread_t hiloInterpreteDeComandos;
	pthread_create(&hiloInterpreteDeComandos, &atributo,(void*) consolaKernel, NULL);
	pthread_attr_destroy(&atributo);

	//-------------------------------HILO PLANIFICACION-----------------------------------------
	pthread_attr_init(&atributo);
	pthread_attr_setdetachstate(&atributo, PTHREAD_CREATE_DETACHED);
	pthread_create(&hiloInterpreteDeComandos, &atributo,(void*) planificar, NULL);
	pthread_attr_destroy(&atributo);

	//-------------------------------CONEXIONES CPU Y CONSOLA-----------------------------------
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
	t_cpu* cpuNueva;

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

						int tipo = handshake(socket_cliente_aceptado, 0);

						switch(tipo){


							case 1:
// Aca entro cuando entro una nueva consola		-----------------------------------------------------------------
								FD_SET(socket_cliente_aceptado, &fdsMaestro);
								_lockLista_Consolas();
								list_add(lista_consolas, &socket_cliente_aceptado);
								_unlockLista_Consolas();
								log_trace(conectar_select_log, "Nueva Consola");

								break;
							case 2:
// Aca entro cuando entro una nueva cpu			-----------------------------------------------------------------

								// todo: imprimir nueva CPU / consola
								cpuNueva = malloc(sizeof(t_cpu));
								cpuNueva->socket = socket_cliente_aceptado;
								recv(socket_cliente_aceptado, &cpuNueva->pidProcesoCPU, sizeof(t_num), 0);
								cpuNueva->libre = true;
								//sem_init(&cpuNueva->semaforo, 1, 0);
								_lockLista_cpus();
								list_add(lista_cpus, cpuNueva);
								_unlockLista_cpus();

								if (pthread_create(&hilo_conexionCPU, &atributo,(void*) escucharCPU, (void*) socket_cliente_aceptado) < 0) {
									log_error(conectar_select_log, "Error Hilo Escucha Consola");
									abort();
								}
								log_trace(conectar_select_log, "Nueva CPU");
								sem_post(&sem_cantCPUs);

								break;
							case -1:
								log_error(conectar_select_log, "Error en el handshake: el cliente se desconecto"); break;

							case -2:
								log_error(conectar_select_log, "Error en el handshake: send"); break;


						}

					}

				} else
					atender_consola(current_fd);
			}
		}
	}
	return 0;
}


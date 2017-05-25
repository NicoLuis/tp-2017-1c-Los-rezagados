#include <herramientas/sockets.c>
#include "libreriaCPU.h"
#include "pcb.h"



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

	//Creo archivo log
	logAnsisop = log_create("ansisop.log", "Ansisop", false, LOG_LEVEL_TRACE);
	logCPU = log_create("cpu.log", "CPU", false, LOG_LEVEL_TRACE);
	log_trace(logCPU, "  -----------  INICIO CPU  -----------  ");


	signal (SIGINT, finalizarCPU);
	signal (SIGUSR1, ultimaEjec);
	ultimaEjecucion = false;

	//Cargo archivo de configuracion

	t_config* configuracion = config_create(argv[1]);
	ipKernel = config_get_string_value(configuracion, "IP_KERNEL");
	puertoKernel = config_get_int_value(configuracion, "PUERTO_KERNEL");
	ipMemoria = config_get_string_value(configuracion, "IP_MEMORIA");
	puertoMemoria = config_get_int_value(configuracion, "PUERTO_MEMORIA");

	//Muestro archivo de configuracion

	mostrarArchivoConfig();

	//-------------------------------CONEXION AL KERNEL-------------------------------------

	log_info(logCPU, "Me conecto al Kernel");

	socket_kernel = conectarAServidor(ipKernel, puertoKernel);

	char* bufferKernel = malloc(200);

	int bytesRecibidos = recv(socket_kernel, bufferKernel, 50, 0);
	if (bytesRecibidos <= 0) {
		log_info(logCPU, "El Kernel se ha desconectado");
	}

	bufferKernel[bytesRecibidos] = '\0';

	log_info(logCPU, "Recibi %d bytes con el siguiente mensaje: %s",bytesRecibidos, bufferKernel);

	send(socket_kernel, "Hola soy la CPU", 16, 0);

	bytesRecibidos = recv(socket_kernel, bufferKernel, 50, 0);

	if (bytesRecibidos <= 0) {
		log_info(logCPU, "El Kernel se ha desconectado");
	}

	bufferKernel[bytesRecibidos] = '\0';
	if (strcmp("Conexion aceptada", bufferKernel) == 0) {
		log_trace(logCPU, "Me conecte correctamente al Kernel");
	}

	free(bufferKernel);

	//-------------------------------CONEXION AL LA MEMORIA-------------------------------------

	log_trace(logCPU, "Me conecto a la Memoria");

	socket_memoria = conectarAServidor(ipMemoria, puertoMemoria);
	char* bufferMemoria = malloc(200);

	bytesRecibidos = recv(socket_memoria, bufferMemoria, 50, 0);
	if (bytesRecibidos <= 0)
		log_info(logCPU, "La Memoria se ha desconectado");
	bufferMemoria[bytesRecibidos] = '\0';

	log_info(logCPU, "Recibi %d bytes con el siguiente mensaje: %s",bytesRecibidos, bufferMemoria);

	send(socket_memoria, "Hola soy la CPU", 16, 0);

	bytesRecibidos = recv(socket_memoria, bufferMemoria, 50, 0);
	if (bytesRecibidos <= 0)
		log_info(logCPU, "La Memoria se ha desconectado");
	bufferMemoria[bytesRecibidos] = '\0';

	if (!strcmp("Conexion aceptada", bufferMemoria))
		log_trace(logCPU, "Me conecte correctamente a la Memoria");

	if(recv(socket_memoria, &tamanioPagina, sizeof(t_num), 0) <= 0)
		log_info(logCPU, "La Memoria se ha desconectado");
	log_info(logCPU, "tamanio de pagina %d", tamanioPagina);
	free(bufferMemoria);


	while(1){

		t_msg* msgRecibido = msg_recibir(socket_kernel);
		if(msgRecibido->longitud != 0)
			msg_recibir_data(socket_kernel, msgRecibido);

		switch(msgRecibido->tipoMensaje){
		case ENVIO_PCB:

			log_trace(logCPU, "Recibi ENVIO_PCB");
			pcb = desserealizarPCB(msgRecibido->data);

			log_trace(logCPU, "pcb->pid %d", pcb->pid);
			log_trace(logCPU, "pcb->pc %d", pcb->pc);
			log_trace(logCPU, "pcb->cantPagsCodigo %d", pcb->cantPagsCodigo);
			log_trace(logCPU, "pcb->exitCode %d", pcb->exitCode);
			log_trace(logCPU, "pcb->indiceCodigo.size %d", pcb->indiceCodigo.size);
			log_trace(logCPU, "pcb->indiceEtiquetas.size %d", pcb->indiceEtiquetas.size);

			break;
		case EJECUTAR_INSTRUCCION:
			log_trace(logCPU, "Recibi EJECUTAR_INSTRUCCION");
			ejecutarInstruccion();
			break;
		case EJECUTAR_ULTIMA_INSTRUCCION:
			log_trace(logCPU, "Recibi EJECUTAR_ULTIMA_INSTRUCCION");
			ultimaEjec();
			ejecutarInstruccion();
			break;
		case 0:
			fprintf(stderr, "El Kernel %d se ha desconectado \n", socket_kernel);
			finalizarCPU();
			break;
		default:
			log_trace(logCPU, "Recibi verdura %d", msgRecibido->tipoMensaje);
		}

	}

	return 0;

}

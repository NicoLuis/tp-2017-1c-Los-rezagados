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
	logAnsisop = log_create(string_from_format("ansisop_%d.log", process_getpid()), "Ansisop", false, LOG_LEVEL_TRACE);
	logCPU = log_create(string_from_format("cpu_%d.log", process_getpid()), "CPU", false, LOG_LEVEL_TRACE);
	log_trace(logAnsisop, "  -----------  INICIO CPU  -----------  ");
	log_trace(logCPU, "  -----------  INICIO CPU  -----------  ");


	signal (SIGINT, finalizarCPU);
	signal (SIGUSR1, ultimaEjecTotal);
	signal (SIGUSR2, ultimaEjec);
	flag_ultimaEjecucionTotal = false;
	flag_ultimaEjecucion = false;
	flag_finalizado = false;

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
	if (bytesRecibidos <= 0)
		log_info(logCPU, "El Kernel se ha desconectado");
	bufferKernel[bytesRecibidos] = '\0';
	log_info(logCPU, "Recibi %d bytes con el siguiente mensaje: %s",bytesRecibidos, bufferKernel);

	send(socket_kernel, "Hola soy la CPU", 16, 0);

	bytesRecibidos = recv(socket_kernel, bufferKernel, 50, 0);
	if (bytesRecibidos <= 0)
		log_trace(logCPU, "El Kernel se ha desconectado");
	bufferKernel[bytesRecibidos] = '\0';

	algoritmo = string_new();
	memcpy(algoritmo, bufferKernel + 18, 5);
	memcpy(&quantum, bufferKernel + 18 + 5, sizeof(t_num));
	memcpy(&quantumSleep, bufferKernel + 18 + 5 + sizeof(t_num), sizeof(t_num));

	t_num pid = process_getpid();
	send(socket_kernel, &pid, sizeof(t_num), 0);

	if (string_equals_ignore_case("Conexion Aceptada", string_substring(bufferKernel, 0, 18))) {
		log_trace(logCPU, "Me conecte correctamente al Kernel");
		log_info(logCPU, "Algoritmo %s quantum %d quantumSleep %d", algoritmo, quantum, quantumSleep);
	}else{
		log_info(logCPU, "bytesRecibidos %s", bufferKernel);
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

			log_trace(logCPU, "\t\t ---- EJECUTO PCB ---- ");
			pcb = desserealizarPCB(msgRecibido->data);

			log_trace(logCPU, "pcb->pid %d", pcb->pid);
			log_trace(logCPU, "pcb->pc %d", pcb->pc);
			log_trace(logCPU, "pcb->cantPagsCodigo %d", pcb->cantPagsCodigo);
			log_trace(logCPU, "pcb->indiceCodigo.size %d", pcb->indiceCodigo.size);
			log_trace(logCPU, "pcb->indiceEtiquetas.size %d", pcb->indiceEtiquetas.size);
			ejecutar();

			break;
		case EJECUTAR_ULTIMA_INSTRUCCION:
			log_trace(logCPU, "Recibi EJECUTAR_ULTIMA_INSTRUCCION");
			flag_finalizado = true;
			flag_ultimaEjecucion = true;
			ejecutarInstruccion();
			break;
		case 0:
			fprintf(stderr, "El Kernel %d se ha desconectado \n", socket_kernel);
			close(socket_kernel);
			socket_kernel = 0;
			finalizarCPU();
			break;
		default:
			log_trace(logCPU, "Recibi verdura %d", msgRecibido->tipoMensaje);
		}

		msg_destruir(msgRecibido);

		if(flag_ultimaEjecucionTotal)
			finalizarCPU();

	}

	return 0;

}

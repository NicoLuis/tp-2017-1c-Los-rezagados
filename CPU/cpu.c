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
	logAnsisop = log_create("Ansisop.log", "KERNEL", false, LOG_LEVEL_TRACE);
	logCPU = log_create("kernel.log", "KERNEL", false, LOG_LEVEL_TRACE);
	log_trace(logCPU, "  -----------  INICIO KERNEL  -----------  ");


	signal (SIGINT, finalizarCPU);
	signal (SIGUSR1, ultimaEjec);
	ultimaEjecucion = true;

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


	while(1){

		t_msg* msgRecibido = msg_recibir(socket_kernel);
		msg_recibir_data(socket_kernel, msgRecibido);

		switch(msgRecibido->tipoMensaje){
		case ENVIO_PCB:

			log_trace(logCPU, "Recibi ENVIO_PCB");
			pcb = desserealizarPCB(msgRecibido->data);

			break;
		case EJECUTAR_INSTRUCCION:
			log_trace(logCPU, "Recibi EJECUTAR_INSTRUCCION");
			ejecutarInstruccion();
			break;
		case 0:
			fprintf(stderr, "El Kernel %d se ha desconectado \n", socket_kernel);
			finalizarCPU();
			break;
		}
	}

	return 0;

}

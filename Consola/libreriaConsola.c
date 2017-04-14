/*
 * libreriaConsola.c
 *
 *  Created on: 9/4/2017
 *      Author: utnso
 */

#include "libreriaConsola.h"

void mostrarArchivoConfig() {

	printf("La IP del Kernel es: %s \n", ipKernel);
	printf("El puerto del Kernel es: %d\n", puertoKernel);
}


void finalizarPrograma(void* socket_kernel) {
	char buffer[200];
	int socket = (int) socket_kernel;
	printf("Ingrese mensaje:\n");
	scanf("%s", buffer);

	printf("EL mensaje ingresado es: %s\n",buffer);

	send(socket, buffer, 20, 0);

}


// los comandos a ingresar son: (utilizando paths absolutos)
// (los nombres se pueden cambiar)

// ● start [path]: Iniciar Programa
// ● kill [pid]: Finalizar Programa
// ● disconnect: Desconectar Consola
// ● clear: Limpiar Mensajes
// ● help: muestra los comandos posibles


void leerComando(char* comando){
	comando[string_length(comando)-1] = '\0';

	if(string_starts_with(comando, "start")){

		char* pathAIniciar = string_substring_from(comando, 6);
		fprintf(stderr, "El path es %s\n", pathAIniciar);

		//inicia un hilo para este programa

		pthread_attr_t atributo;
		pthread_attr_init(&atributo);
		pthread_attr_setdetachstate(&atributo, PTHREAD_CREATE_DETACHED);
		pthread_t hiloInterpreteDeComandos;
		pthread_create(&hiloInterpreteDeComandos, &atributo,(void*) iniciarPrograma, (void *) pathAIniciar);
		pthread_attr_destroy(&atributo);

	}else{

	if(string_starts_with(comando, "kill")){

		int pidAFinalizar = atoi(string_substring_from(comando, 5));
		fprintf(stderr, "El pid es %d\n", pidAFinalizar);

		//todo: mando mensaje a kernel para que mate el pid

	}else{

	if(string_equals_ignore_case(comando, "disconnect")){

		//todo: mando mensaje a kernel para matar a todos

	}else{

	if(string_equals_ignore_case(comando, "clear")){
		system("clear");
	}else{

	if(string_equals_ignore_case(comando, "help")){
		printf("Comandos:\n"
				"● start [path]: Iniciar Programa\n"
				"● kill [pid]: Finalizar Programa\n"
				"● disconnect: Desconectar Consola\n"
				"● clear: Limpiar Mensajes\n");
	}else{
		fprintf(stderr, "El comando '%s' no es valido\n", comando);

	}}}}}

}




void iniciarPrograma(char* pathAIniciar){


	char* scriptCompleto = cargarScript(pathAIniciar);		//fixme: preguntar si envio a kernel el script completo o solo el path


	uint8_t tipoMensaje = CONSOLA_ENVIA_PATH;
	send((int) socket_kernel, &tipoMensaje, sizeof(uint8_t), 0);

	uint32_t tamanioScript = string_length(scriptCompleto);
	send((int) socket_kernel, &tamanioScript, sizeof(uint32_t), 0);

	send((int) socket_kernel, scriptCompleto, tamanioScript, 0);

}



char* cargarScript(void* pathScript){
	int fd;
	char *data;
	struct stat sbuf;

	//todo: considerar errores

	fd = open(pathScript, O_RDONLY);

	stat(pathScript, &sbuf);

	data = mmap((caddr_t)0, sbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);

	close(fd);

	return data;
}

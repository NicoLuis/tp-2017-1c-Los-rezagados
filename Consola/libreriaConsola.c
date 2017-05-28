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


// los comandos a ingresar son: (utilizando paths absolutos)
// (los nombres se pueden cambiar)

// ● start [path]: Iniciar Programa
// ● kill [pid]: Finalizar Programa
// ● disconnect: Desconectar Consola
// ● clear: Limpiar Mensajes
// ● help: muestra los comandos posibles


void leerComando(char* comando){


	comando[strlen(comando)-1] = '\0';

	log_trace(logConsola, "Comando ingresado: %s", comando);

	if(string_starts_with(comando, "start")){

		char* pathAIniciar = string_substring_from(comando, 6);
		//fprintf(stderr, "El path es %s\n", pathAIniciar);

		t_programa* programaNuevo = malloc(sizeof(t_programa));
		programaNuevo->horaInicio = temporal_get_string_time();
		programaNuevo->pid = 0;
		programaNuevo->cantImpresionesPantalla = 0;
		list_add(lista_programas, programaNuevo);

		char* scriptCompleto = cargarScript(pathAIniciar);
		log_trace(logConsola, "\n%s", scriptCompleto);
		msg_enviar_separado(ENVIO_CODIGO, string_length(scriptCompleto) + 1, scriptCompleto, socket_kernel);

	}else if(string_starts_with(comando, "kill")){

		int pidAFinalizar = atoi(string_substring_from(comando, 5));
		//fprintf(stderr, "El pid es %d\n", pidAFinalizar);

		//todo: mando mensaje a kernel para que mate el pid

	}else if(string_equals_ignore_case(comando, "disconnect")){

		//todo: mando mensaje a kernel para matar a todos

	}else if(string_equals_ignore_case(comando, "clear")){
		system("clear");

	}else if(string_equals_ignore_case(comando, "help")){
		printf("Comandos:\n"
				"● start [path]: Iniciar Programa\n"
				"● kill [pid]: Finalizar Programa\n"
				"● disconnect: Desconectar Consola\n"
				"● clear: Limpiar Mensajes\n");
	}else
		fprintf(stderr, "El comando '%s' no es valido\n", comando);

}



void escucharKernel(){

	t_num8 pidProg;
	t_programa* programa = malloc(sizeof(t_programa));
	int _programaSinPid(t_programa* p){
		return p->pid == 0;
	}
	int _esPrograma(t_programa* p){
		return p->pid == pidProg;
	}

	while(1){

		t_msg* msgRecibido = msg_recibir(socket_kernel);

		switch(msgRecibido->tipoMensaje){

		case OK:
			log_trace(logConsola, "Recibi OK");
			msg_recibir_data(socket_kernel, msgRecibido);
			memcpy(&pidProg, msgRecibido->data, sizeof(t_num8));
			log_trace(logConsola, "El pid es %d", pidProg);

			programa = list_find(lista_programas, (void*) _programaSinPid);
			if(programa == NULL)
				log_trace(logConsola, "No se encuentra programa sin pid");
			programa->pid = pidProg;

			fprintf(stderr, "El pid es %d\n", pidProg);
			break;
		case MARCOS_INSUFICIENTES:
			log_trace(logConsola, "Recibi MARCOS_INSUFICIENTES");
			fprintf(stderr, "No hay espacio suficiente en memoria \n");
			finalizarPrograma(programa);
			break;
		case FINALIZAR_PROGRAMA:
			log_trace(logConsola, "Recibi FINALIZAR_PROGRAMA");
			msg_recibir_data(socket_kernel, msgRecibido);
			memcpy(&pidProg, msgRecibido->data, sizeof(t_num8));
			log_trace(logConsola, "El pid a finalizar es %d", pidProg);

			programa = list_find(lista_programas, (void*) _esPrograma);
			programa->horaFin = temporal_get_string_time();
			finalizarPrograma(programa);
			break;
		case 0:
			fprintf(stderr, "El kernel se ha desconectado \n");
			log_trace(logConsola, "La desconecto el kernel");
			pthread_exit(NULL);
			break;
		case IMPRIMIR_TEXTO:
			log_trace(logConsola, "Recibi IMPRIMIR_TEXTO");
			msg_recibir_data(socket_kernel, msgRecibido);
			char* texto = malloc(msgRecibido->longitud);
			memcpy(texto, msgRecibido->data, msgRecibido->longitud);
			fprintf(stderr, "%s\n", texto);
			programa = list_find(lista_programas, (void*) _esPrograma);
			programa->cantImpresionesPantalla++;
			break;
		}

	}

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



char* _contarTiempo(char* tiempo1, char* tiempo2){
	char* tiempoTranscurrido = string_new();
	int horas = 0, minutos = 0, segundos = 0, milisegundos;
	milisegundos = atoi(string_substring(tiempo1, 9, 3)) - atoi(string_substring(tiempo2, 9, 3));
	if (milisegundos < 0){
		milisegundos = 1000 + milisegundos;
		segundos = -1;
	}
	segundos = segundos + atoi(string_substring(tiempo1, 6, 2)) - atoi(string_substring(tiempo2, 6, 2));
	if (segundos < 0){
		segundos = 60 + segundos;
		minutos = -1;
	}
	minutos = minutos + atoi(string_substring(tiempo1, 3, 2)) - atoi(string_substring(tiempo2, 3, 2));
	if (minutos < 0){
		minutos = 60 + minutos;
		horas = -1;
	}
	horas = horas + atoi(string_substring(tiempo1, 0, 2)) - atoi(string_substring(tiempo2, 0, 2));

	if(horas < 10) string_append_with_format(&tiempoTranscurrido, "0%d:", horas);
	else string_append_with_format(&tiempoTranscurrido, "%d:", horas);
	if(minutos < 10) string_append_with_format(&tiempoTranscurrido, "0%d:", minutos);
	else string_append_with_format(&tiempoTranscurrido, "%d:", minutos);
	if(segundos < 10) string_append_with_format(&tiempoTranscurrido, "0%d:", segundos);
	else string_append_with_format(&tiempoTranscurrido, "%d:", segundos);
	if(milisegundos < 10) string_append_with_format(&tiempoTranscurrido, "00%d", milisegundos);
	else if(milisegundos < 100) string_append_with_format(&tiempoTranscurrido, "0%d", milisegundos);
	else string_append_with_format(&tiempoTranscurrido, "%d", milisegundos);


	return tiempoTranscurrido;
}

void finalizarPrograma(t_programa* prog){

	/*todo: detallar
	● Fecha y hora de inicio de ejecución
	● Fecha y hora de fin de ejecución
	● Cantidad de impresiones por pantalla
	● Tiempo total de ejecución (diferencia entre tiempo de inicio y tiempo de fin)
	 */

	fprintf(stderr, "Fecha y hora de inicio %s \n",  prog->horaInicio);
	fprintf(stderr, "Fecha y hora de fin %s \n",  prog->horaFin);
	fprintf(stderr, "Impresiones por pantalla %d \n",  prog->cantImpresionesPantalla);
	fprintf(stderr, "Tiempo total %s \n",  _contarTiempo(prog->horaFin, prog->horaInicio));

	//todo: sacar de lista

}

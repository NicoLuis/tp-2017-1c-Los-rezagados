/*
 * libreriaConsola.c
 *
 *  Created on: 9/4/2017
 *      Author: utnso
 */

#include "libreriaConsola.h"

void mostrarArchivoConfig() {

	system("clear");
	printf("Nº log %d \n", process_getpid());
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
		if (scriptCompleto != NULL){
			log_trace(logConsola, "\n%s", scriptCompleto);
			msg_enviar_separado(ENVIO_CODIGO, string_length(scriptCompleto) + 1, scriptCompleto, socket_kernel);
		}else{
			log_error(logConsola, "Error al abrir el script en %s", pathAIniciar);
		}

	}else if(string_starts_with(comando, "kill")){

		t_pid pidAFinalizar = atoi(string_substring_from(comando, 5));
		//fprintf(stderr, "El pid es %d\n", pidAFinalizar);

		int _esPid(t_programa* p){
			return p->pid == pidAFinalizar;
		}
		t_programa* prog = list_find(lista_programas, (void*) _esPid);
		if(prog != NULL){
			msg_enviar_separado(FINALIZAR_PROGRAMA, sizeof(t_pid), &pidAFinalizar, socket_kernel);
			//finalizarPrograma(prog, 0);
			fprintf(stderr, PRINT_COLOR_BLUE "Finalizo PID %d" PRINT_COLOR_RESET "\n", pidAFinalizar);
		}
		else
			fprintf(stderr, "El pid %d no pertenece a esta consola\n", pidAFinalizar);


	}else if(string_equals_ignore_case(comando, "disconnect")){

		msg_enviar_separado(DESCONECTAR, 0, 0, socket_kernel);

	}else if(string_equals_ignore_case(comando, "clear")){
		system("clear");

	}else if(string_equals_ignore_case(comando, "help")){
		printf("Comandos:\n"
				PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "start [path]: " PRINT_COLOR_RESET "Iniciar Programa\n"
				PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "kill [pid]: " PRINT_COLOR_RESET "Finalizar Programa\n"
				PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "disconnect: " PRINT_COLOR_RESET "Desconectar Consola\n"
				PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "clear: " PRINT_COLOR_RESET "Limpiar Mensajes\n");
	}else
		fprintf(stderr, PRINT_COLOR_YELLOW "El comando '%s' no es valido" PRINT_COLOR_RESET "\n", comando);

}



void escucharKernel(){

	t_pid pidProg;
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
			if(msg_recibir_data(socket_kernel, msgRecibido) > 0){
				memcpy(&pidProg, msgRecibido->data, sizeof(t_pid));
				log_trace(logConsola, "PID %d", pidProg);

				programa = list_find(lista_programas, (void*) _programaSinPid);
				if(programa == NULL)
					log_trace(logConsola, "No se encuentra programa sin pid");
				programa->pid = pidProg;

				fprintf(stderr, PRINT_COLOR_CYAN "PID %d" PRINT_COLOR_RESET "\n", pidProg);
			}else
				log_warning(logConsola, "No recibi nada");
			break;
		case MARCOS_INSUFICIENTES:
			log_trace(logConsola, "Recibi MARCOS_INSUFICIENTES");
			fprintf(stderr, PRINT_COLOR_YELLOW "No hay espacio suficiente en memoria" PRINT_COLOR_RESET "\n");
			finalizarPrograma(programa, 0);
			break;
		case FINALIZAR_PROGRAMA:
			log_trace(logConsola, "Recibi FINALIZAR_PROGRAMA");
			if(msg_recibir_data(socket_kernel, msgRecibido) > 0){
				memcpy(&pidProg, msgRecibido->data, sizeof(t_pid));
				log_trace(logConsola, "El pid a finalizar es %d", pidProg);

				programa = list_find(lista_programas, (void*) _esPrograma);
				programa->horaFin = temporal_get_string_time();
				finalizarPrograma(programa, 1);
			}else
				log_warning(logConsola, "No recibi nada");
			break;
		case 0:
			fprintf(stderr, PRINT_COLOR_RED "El kernel se ha desconectado" PRINT_COLOR_RESET "\n");
			log_trace(logConsola, "La desconecto el kernel");
			close(socket_kernel);
			socket_kernel = 0;
			finalizarConsola();
			pthread_exit(NULL);
			break;
		case ERROR:
			log_trace(logConsola, "Recibi ERROR");
			if(msg_recibir_data(socket_kernel, msgRecibido) > 0){
				memcpy(&pidProg, msgRecibido->data, sizeof(t_pid));
				log_trace(logConsola, "El pid %d no pudo terminar su ejecucion", pidProg);
				fprintf(stderr, PRINT_COLOR_YELLOW "El PID %d no pudo terminar su ejecucion" PRINT_COLOR_RESET "\n", pidProg);

				programa = list_find(lista_programas, (void*) _esPrograma);
				programa->horaFin = temporal_get_string_time();
				finalizarPrograma(programa, 0);
			}else
				log_warning(logConsola, "No recibi nada");
			break;
		case IMPRIMIR_TEXTO:
			log_trace(logConsola, "Recibi IMPRIMIR_TEXTO");
			if(msg_recibir_data(socket_kernel, msgRecibido) > 0){
				char* texto = malloc(msgRecibido->longitud);
				memcpy(texto, msgRecibido->data, msgRecibido->longitud);
				fprintf(stderr, "%s\n", texto);
				programa = list_find(lista_programas, (void*) _esPrograma);
				programa->cantImpresionesPantalla++;
			}else
				log_warning(logConsola, "No recibi nada");
			break;
		}

	}

}



char* cargarScript(void* pathScript){
	int fd;
	char *data;
	struct stat sbuf;

	fd = open(pathScript, O_RDONLY);
	if (fd == -1) {
		perror("error al abrir el archivo");
		return NULL;
	}
	if (stat(pathScript, &sbuf) == -1) {
		perror("Stat");
		return NULL;
	}
	data = mmap((caddr_t)0, sbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED) {
		perror("Fallo el mmap");
		return NULL;
	}
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

void finalizarPrograma(t_programa* prog, bool flag_print){

	if(flag_print){
		fprintf(stderr, "● Fecha y hora de inicio %s \n",  prog->horaInicio);
		fprintf(stderr, "● Fecha y hora de fin %s \n",  prog->horaFin);
		fprintf(stderr, "● Impresiones por pantalla %d \n",  prog->cantImpresionesPantalla);
		fprintf(stderr, "● Tiempo total %s \n",  _contarTiempo(prog->horaFin, prog->horaInicio));
	}

	int _esPid(t_programa* p){
		return p->pid == prog->pid;
	}
	list_remove_and_destroy_by_condition(lista_programas, (void*) _esPid, free);

}


void finalizarConsola(){
	log_trace(logConsola, "  FIN CONSOLA  ");

	if(socket_kernel != 0){
		msg_enviar_separado(DESCONECTAR, 0, 0, socket_kernel);
		close(socket_kernel);
	}

	log_destroy(logConsola);

	exit(1);
}

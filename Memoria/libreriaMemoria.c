/*
 * libreriaMemoria.c
 *
 *  Created on: 3/4/2017
 *      Author: utnso
 */

#include "libreriaMemoria.h"

void mostrarArchivoConfig() {

	printf("El puerto de la MEMORIA es: %d\n", puertoMemoria);
	log_info(log_memoria,"El puerto de la MEMORIA es: %d", puertoMemoria);
	printf("La cantidad de Marcos es: %d\n", cantidadDeMarcos);
	log_info(log_memoria,"La cantidad de Marcos es: %d", cantidadDeMarcos);
	printf("El tamaño de las Marcos es: %d\n", tamanioDeMarcos);
	log_info(log_memoria,"El tamaño de las Marcos es: %d", tamanioDeMarcos);
	printf("La cantidad de entradas de Cache habilitadas son: %d\n", cantidadaEntradasCache);
	log_info(log_memoria,"La cantidad de entradas de Cache habilitadas son: %d", cantidadaEntradasCache);
	printf("La cantidad maxima de entradas a la cache por proceso son: %d\n", cachePorProceso);
	log_info(log_memoria,"La cantidad maxima de entradas a la cache por proceso son: %d", cachePorProceso);
	printf("El algoritmo de reemplaco de la Cache es: %s\n", algoritmoReemplazo);
	log_info(log_memoria,"El algoritmo de reemplaco de la Cache es: %s", algoritmoReemplazo);
	printf("El retardo de la memoria es de: %d\n",retardoMemoria);
	log_info(log_memoria,"El retardo de la memoria es de: %d",retardoMemoria);

}


void escucharKERNEL(void* socket_kernel) {

	//Casteo socketKernel
	int socketKernel = (int) socket_kernel;

	uint32_t pid;
	uint32_t cantidadDePaginas;

	log_info(log_memoria,"Se conecto el Kernel");

	int bytesEnviados = send(socketKernel, "Conexion Aceptada", 18, 0);
	if (bytesEnviados <= 0) {
		log_error(log_memoria,"Error send Kernel");
		pthread_exit(NULL);
	}

	//envio tamaño pags a kernel
	send(socketKernel, &tamanioDeMarcos, sizeof(uint32_t), 0);

	uint32_t header;
	while (1) {

		if (recv(socketKernel, &pid, sizeof(uint32_t), 0) <= 0) {
			log_info(log_memoria,"El Kernel se ha desconectado");
			pthread_exit(NULL);
		}
		t_msg* msg = msg_recibir(socketKernel);
		msg_recibir_data(socketKernel, msg);

		header = msg->tipoMensaje;

		switch (header) {

		case INICIALIZAR_PROGRAMA:

			cantidadDePaginas = msg->longitud / tamanioDeMarcos;
			cantidadDePaginas = (msg->longitud % tamanioDeMarcos) == 0? cantidadDePaginas: cantidadDePaginas + 1;

			log_info(log_memoria, "Solicitud de inicializar proceso %d con %d paginas", pid, cantidadDePaginas);

			crearProcesoYAgregarAListaDeProcesos(pid, cantidadDePaginas);

			//guardo codigo en memoria

			if(hayFramesLibres(cantidadDePaginas)){
				int i = 0;
				t_frame* marcoLibre;
				for(; i < cantidadDePaginas; i++){
					marcoLibre = buscarFrameLibre(pid);
					escribirContenido(marcoLibre->nroFrame, 0, tamanioDeMarcos, msg->data + i*tamanioDeMarcos);
					free(marcoLibre);
				}
				log_info(log_memoria, "Asigne correctamente");
		 		header = 29;//OK
				send(socketKernel, &header, sizeof(uint8_t), 0);
			}else{
				log_warning(log_memoria, "No hay libres");
				header = 30;//MARCOS_INSUFICIENTES
				send(socketKernel, &header, sizeof(uint8_t), 0);
			}
			break;


		case FINALIZAR_PROGRAMA:
			if (recv(socketKernel, &pid, sizeof(uint32_t), 0) <= 0) {
				log_info(log_memoria,"El Kernel se ha desconectado");
				pthread_exit(NULL);
			}

			log_info(log_memoria,"Finalizando proceso %d\n", pid);

			lockFramesYProcesos();

			liberarFramesDeProceso(pid);
			eliminarProcesoDeListaDeProcesos(pid);

			unlockFramesYProcesos();

			break;
		}

		msg_destruir(msg);

	}
}

void escucharCPU(void* socket_cpu) {

	//Casteo socketCPU
	int socketCPU = (int) socket_cpu;

	uint32_t PID = 0;
	t_proceso* proceso;
	list_add(lista_cpus, socket_cpu);

	log_info(log_memoria,"Se conecto un CPU\n");

	int bytesEnviados = send(socketCPU, "Conexion Aceptada", 18, 0);
	if (bytesEnviados <= 0) {
		log_error(log_memoria,"Error send CPU");
		pthread_exit(NULL);
	}

	uint32_t header;
	int bytesRecibidos;
	while (1) {
/*
			bytesRecibidos = recv(socketCPU, &header, sizeof(uint32_t), 0);
			if (bytesRecibidos <= 0) {
				log_error(log_memoria,"Error recv CPU");
				pthread_exit(NULL);
			}

			if (header != CPU) {
				log_info(log_memoria,"La CPU se ha desconectado o etc");
				pthread_exit(NULL);
			}

			bytesRecibidos = recv(socketCPU, &header, sizeof(uint32_t), 0);
			if (bytesRecibidos <= 0) {
				log_error(log_memoria,"Error recv CPU");
				pthread_exit(NULL);
			}
*/
			t_msg* msg = msg_recibir(socketCPU);
			msg_recibir_data(socketCPU, msg);

			header = msg->tipoMensaje;

			switch (header) {

			case FIN_CPU: {
				log_info(log_memoria,"La CPU %d ha finalizado\n", socketCPU);
				pthread_exit(NULL);
			}
			break;
			case LECTURA_PAGINA: {
				pthread_mutex_lock(&mutexCantAccesosMemoria);
				cantAccesosMemoria++;
				pthread_mutex_unlock(&mutexCantAccesosMemoria);

				uint8_t numero_pagina;
				uint32_t offset;
				uint32_t tamanio_leer;

				bytesRecibidos = recv(socketCPU, &numero_pagina, sizeof(uint8_t),0);
				if (bytesRecibidos <= 0) {
					log_error(log_memoria,"Error recv CPU");
					pthread_exit(NULL);
				}

				bytesRecibidos = recv(socketCPU, &offset, sizeof(uint32_t), 0);
				if (bytesRecibidos <= 0) {
					log_error(log_memoria,"Error recv CPU");
					pthread_exit(NULL);
				}

				bytesRecibidos = recv(socketCPU, &tamanio_leer, sizeof(uint32_t),0);
				if (bytesRecibidos <= 0) {
					log_error(log_memoria,"Error recv CPU");
					pthread_exit(NULL);
				}

				log_info(log_memoria,"Solicitud de lectura. Pag:%d Offset:%d Size:%d",numero_pagina, offset, tamanio_leer);

				lockProcesos();
				ponerBitUsoEn1(PID, numero_pagina);
				unlockProcesos();

				if (paginaInvalida(PID, numero_pagina)) {

					log_info(log_memoria,"Stack Overflow proceso %d\n", PID);

					//AVISO FINALIZACION PROGRAMA A LA CPU
					header = FINALIZAR_PROGRAMA;

					bytesEnviados = send(socketCPU, &header, sizeof(uint32_t), 0);
					if (bytesEnviados <= 0) {
						log_error(log_memoria,"Error send CPU");
						pthread_exit(NULL);
					}

					//-----Retardo
					pthread_mutex_lock(&mutexRetardo);
					usleep(retardoMemoria * 1000);
					pthread_mutex_unlock(&mutexRetardo);
					//------------

					lockProcesos();
					t_pag* pagina = buscarPaginaEnListaDePaginas(PID,numero_pagina);
					unlockProcesos();

					void* contenido_leido = obtenerContenido(pagina->nroFrame,offset, tamanio_leer);

					enviarContenidoCPU(contenido_leido, tamanio_leer, socketCPU);


				} else {

					lockFrames();
					bool hayFrameLibre = hayFramesLibres(1);
					unlockFrames();

					if ((!hayFrameLibre) && (proceso->cantFramesAsignados == 0)) {

						log_info(log_memoria,"Finalizando programa: %d. No hay frames disponibles\n",PID);

						//AVISO FINALIZACION PROGRAMA A LA CPU
						header = FINALIZAR_PROGRAMA;

						bytesEnviados = send(socketCPU, &header, sizeof(uint32_t), 0);
						if (bytesEnviados <= 0) {
							log_error(log_memoria,"Error send CPU");
							pthread_exit(NULL);
						}

						/*

					} else {

						log_info(log_memoria,"La pagina %d NO esta en Memoria Real. Page Fault\n",numero_pagina);

						//Envio solicitud de lectura a FS

						void* paginaLeidaDeFS = pedirPaginaAFS(PID,	numero_pagina);

						//ahora que tengo la pagina la debo cargar en memoria y leer la parte que me pidio la cpu
						//obtener contenido a enviar segun pag y offset

						lockFramesYProcesos();

						cargarPaginaAMemoria(PID, numero_pagina, paginaLeidaDeFS,LECTURA_PAGINA);

						t_pag* pagina = buscarPaginaEnListaDePaginas(PID,numero_pagina);

						unlockFramesYProcesos();

						void* contenido_leido = obtenerContenido(pagina->nroFrame,offset, tamanio_leer);

						enviarContenidoCPU(contenido_leido, tamanio_leer,socketCPU);

					}
					*/
					}

				}
			}
				break;

			case ESCRITURA_PAGINA: {



				pthread_mutex_lock(&mutexCantAccesosMemoria);
				cantAccesosMemoria++;
				pthread_mutex_unlock(&mutexCantAccesosMemoria);

				uint8_t numero_pagina;
				uint32_t offset;
				uint32_t tamanio_escritura;

				bytesRecibidos = recv(socketCPU, &numero_pagina, sizeof(uint8_t),0);
				if (bytesRecibidos <= 0) {
					log_error(log_memoria, "Error recv CPU");
					pthread_exit(NULL);
				}

				bytesRecibidos = recv(socketCPU, &offset, sizeof(uint32_t), 0);
				if (bytesRecibidos <= 0) {
					log_error(log_memoria, "Error recv CPU");
					pthread_exit(NULL);
					}

				bytesRecibidos = recv(socketCPU, &tamanio_escritura,sizeof(uint32_t), 0);
				if (bytesRecibidos <= 0) {
					log_error(log_memoria, "Error recv CPU");
					pthread_exit(NULL);
					}

				log_info(log_memoria, "Solicitud de escritura. Pag:%d Offset:%d Size:%d",numero_pagina, offset, tamanio_escritura);

				void* contenido_escribir = calloc(1, tamanio_escritura);

				bytesRecibidos = recv(socketCPU, contenido_escribir,tamanio_escritura, 0);
				if (bytesRecibidos <= 0) {
					log_error(log_memoria, "Error recv CPU");
					pthread_exit(NULL);
					}

				if (tamanio_escritura == sizeof(int)) {
					int contenido;
					memcpy(&contenido, contenido_escribir, sizeof(int));
					log_info(log_memoria, "Contenido: %d", contenido);
					}

				lockProcesos();
				ponerBitUsoEn1(PID, numero_pagina);
				unlockProcesos();

				if (paginaInvalida(PID, numero_pagina)) {

				log_error(log_memoria, "Stack Overflow proceso %d\n", PID);

				//AVISO FINALIZACION PROGRAMA A LA CPU

				header = FINALIZAR_PROGRAMA;

				bytesEnviados = send(socketCPU, &header, sizeof(uint32_t), 0);
				if (bytesEnviados <= 0) {
					log_error(log_memoria, "Error send CPU");
					pthread_exit(NULL);
					}

				} else if (estaEnMemoriaReal(PID, numero_pagina)) {

						log_info(log_memoria, "La pagina %d esta en Memoria Real\n",numero_pagina);

						//-----Retardo
						pthread_mutex_lock(&mutexRetardo);
						usleep(retardoMemoria * 1000);
						pthread_mutex_unlock(&mutexRetardo);
						//------------

						lockProcesos();
						t_pag* pag = buscarPaginaEnListaDePaginas(PID, numero_pagina);
						unlockProcesos();

						escribirContenido(pag->nroFrame, offset, tamanio_escritura,contenido_escribir);

						lockFrames();
						ponerBitModificadoEn1(pag->nroFrame);
						unlockFrames();

						//ENVIO ESCRITURA OK
						header = ESCRITURA_PAGINA;

						bytesEnviados = send(socketCPU, &header, sizeof(uint32_t), 0);
						if (bytesEnviados <= 0) {
							log_error(log_memoria, "Error send CPU");
							pthread_exit(NULL);
							}

						//MODIFICAR
						int cantidadDePaginas = tamanio_escritura / tamanioDeMarcos;
						lockFrames();
						int hayFrameLibre = hayFramesLibres(cantidadDePaginas);
						unlockFrames();

						if ((!hayFrameLibre) && (proceso->cantFramesAsignados == 0)) {

							log_info(log_memoria,"Finalizando programa: %d. No hay frames disponibles\n",PID);

							//AVISO FINALIZACION PROGRAMA A LA CPU
							header = FINALIZAR_PROGRAMA;

							bytesEnviados = send(socketCPU, &header, sizeof(uint32_t),0);
							if (bytesEnviados <= 0) {
								log_error(log_memoria, "Error send CPU");
								pthread_exit(NULL);
								}

						}

					}
				}

			break;
			default:
				log_error(log_memoria,"Error");
			}

	}
}

void* reservarMemoria(int cantidadMarcos, int capacidadMarco) {
	// La creo con calloc para que me la llene de \0
	void * memoria = calloc(cantidadMarcos, capacidadMarco);
	log_info(log_memoria,"Memoria reservada\n");
	return memoria;
}

void lockProcesos() {

	pthread_mutex_lock(&mutexListaProcesos);

}
void unlockProcesos() {

	pthread_mutex_unlock(&mutexListaProcesos);

}

void lockFrames() {

	pthread_mutex_lock(&mutexListaFrames);

}

void unlockFrames() {

	pthread_mutex_unlock(&mutexListaFrames);

}

void lockFramesYProcesos() {

	pthread_mutex_lock(&mutexListaFrames);
	pthread_mutex_lock(&mutexListaProcesos);

}

void unlockFramesYProcesos() {

	pthread_mutex_unlock(&mutexListaProcesos);
	pthread_mutex_unlock(&mutexListaFrames);

}

void crearProcesoYAgregarAListaDeProcesos(uint32_t pid,	uint32_t cantidadDePaginas) {

	t_proceso* procesoNuevo = malloc(sizeof(t_proceso));
	procesoNuevo->PID = pid;
	procesoNuevo->cantFramesAsignados = 0;
	procesoNuevo->cantPaginas = cantidadDePaginas;
	procesoNuevo->listaPaginas = crearEInicializarListaDePaginas(cantidadDePaginas);

	list_add(listaProcesos, procesoNuevo);

}

void escribirPaginaEnFS(uint32_t pid, uint8_t nroPag, void* contenido_pagina) {

	void* bufferFS = malloc(100 + tamanioDeMarcos);
	uint32_t tamanioValidoBufferFS = 0;

	uint32_t header = MEMORIA;
	memcpy(bufferFS, &header, sizeof(uint32_t));
	tamanioValidoBufferFS += sizeof(uint32_t);

	header = ESCRITURA_PAGINA;
	memcpy(bufferFS + tamanioValidoBufferFS, &header, sizeof(uint32_t));
	tamanioValidoBufferFS += sizeof(uint32_t);

	memcpy(bufferFS + tamanioValidoBufferFS, &pid, sizeof(uint32_t));
	tamanioValidoBufferFS += sizeof(uint32_t);

//Numero de pagina
	memcpy(bufferFS + tamanioValidoBufferFS, &nroPag, sizeof(uint8_t));
	tamanioValidoBufferFS += sizeof(uint8_t);

	memcpy(bufferFS + tamanioValidoBufferFS, contenido_pagina,tamanioDeMarcos);
	tamanioValidoBufferFS += tamanioDeMarcos;

	pthread_mutex_lock(&mutexFS);

	int bytesEnviados = send(socket_fs, bufferFS, tamanioValidoBufferFS,0);

	if (bytesEnviados <= 0) {
		log_error(log_memoria,"Error escritura en FS");
		pthread_mutex_unlock(&mutexFS);
		pthread_exit(NULL);
	}

	log_info(log_memoria,"Escribi en FS pagina %d del proceso %d\n", nroPag, pid);

	pthread_mutex_unlock(&mutexFS);

	free(bufferFS);
}

void liberarFramesDeProceso(uint32_t unPid) {

	int _soy_el_frame_buscado(t_frame* frame) {
		return (frame->pid == unPid);
	}

	int framesAsignadosAlProceso = list_count_satisfying(listaFrames,(void *) _soy_el_frame_buscado);

	int i = 0;
	while (i < framesAsignadosAlProceso) {
		t_frame* frame = list_find(listaFrames, (void *) _soy_el_frame_buscado);
		frame->pid = 0;

		log_info(log_memoria,"Frame %d liberado", frame->nroFrame);

		i++;
	}

}

void eliminarProcesoDeListaDeProcesos(uint32_t unPid) {
	int posicion = -1;

	int _soy_el_proceso_buscado(t_proceso* proceso) {
		posicion++;
		return (proceso->PID == unPid);
	}

	list_find(listaProcesos, (void*) _soy_el_proceso_buscado);
	t_proceso* proceso = list_remove(listaProcesos, posicion);

	//-----Retardo
	pthread_mutex_lock(&mutexRetardo);
	usleep(retardoMemoria * 1000);
	pthread_mutex_unlock(&mutexRetardo);
	//------------

	void _destruir_pagina(t_pag* pagina) {
		free(pagina);
	}

	list_destroy_and_destroy_elements(proceso->listaPaginas,(void*) _destruir_pagina);

	free(proceso);
}

t_list* crearEInicializarListaDePaginas(uint32_t cantidadDePaginas) {

	//-----Retardo
	pthread_mutex_lock(&mutexRetardo);
	usleep(retardoMemoria * 1000);
	pthread_mutex_unlock(&mutexRetardo);
	//------------

	t_list* listaDePaginas = list_create();

	int i = 0;
	while (i < cantidadDePaginas) {

		t_pag* paginaAInicializar = malloc(sizeof(t_pag));
		paginaAInicializar->nroPag = i;
		paginaAInicializar->bit_pres = 0;
		paginaAInicializar->nroFrame = -1;
		list_add(listaDePaginas, paginaAInicializar);

		i++;

	}
	return listaDePaginas;
}

void ponerBitUsoEn1(uint32_t pid, uint8_t numero_pagina) {

	t_pag* pagina = buscarPaginaEnListaDePaginas(pid, numero_pagina);

	if (pagina != NULL) {
		pagina->bit_uso = 1;
	}
}

int paginaInvalida(uint32_t pid, uint8_t numero_pagina) {

		lockProcesos();

		t_proceso* proceso = buscarProcesoEnListaProcesos(pid);

		int esPagInvalida = (numero_pagina >= proceso->cantPaginas);

		unlockProcesos();

		return esPagInvalida;

}

t_pag* buscarPaginaEnListaDePaginas(uint32_t pid, uint8_t numero_pagina) {

	t_proceso* proceso = buscarProcesoEnListaProcesos(pid);

	int _soy_la_pagina_buscada(t_pag* pag) {
		return (pag->nroPag == numero_pagina);
	}

	return list_find(proceso->listaPaginas, (void*) _soy_la_pagina_buscada);

}

void* obtenerContenido(int frame, int offset, int tamanio_leer) {

	//-----Retardo
	pthread_mutex_lock(&mutexRetardo);
	usleep(retardoMemoria * 1000);
	pthread_mutex_unlock(&mutexRetardo);
	//------------
	void* contenido = calloc(1, tamanio_leer);
	int desplazamiento = (frame * tamanioDeMarcos) + offset;

	pthread_mutex_lock(&mutexMemoriaReal);
	memcpy(contenido, memoria_real + desplazamiento, tamanio_leer);
	pthread_mutex_unlock(&mutexMemoriaReal);

	return contenido;
}

void enviarContenidoCPU(void* contenido_leido, uint32_t tamanioContenido,int socketCPU) {

	uint32_t header;

	void* bufferCPU = malloc(tamanioContenido + 20);
	uint32_t tamanioValidoBuffer = 0;

	header = MEMORIA;
	memcpy(bufferCPU, &header, sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	header = ENVIO_PARTE_PAGINA;
	memcpy(bufferCPU + tamanioValidoBuffer, &header, sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	memcpy(bufferCPU + tamanioValidoBuffer, &tamanioContenido,sizeof(uint32_t));
	tamanioValidoBuffer += sizeof(uint32_t);

	memcpy(bufferCPU + tamanioValidoBuffer, contenido_leido, tamanioContenido);
	tamanioValidoBuffer += tamanioContenido;

	int bytesEnviados = send(socketCPU, bufferCPU, tamanioValidoBuffer, 0);
	if (bytesEnviados <= 0) {
		log_error(log_memoria,"Error send CPU");
		pthread_exit(NULL);
	}

	free(bufferCPU);
	free(contenido_leido);
}

t_proceso* buscarProcesoEnListaProcesos(uint32_t pid) {

	int _soy_el_pid_buscado(t_proceso* proceso) {
		return (proceso->PID == pid);
	}

	t_proceso* proceso = list_find(listaProcesos, (void*) _soy_el_pid_buscado);

	if (proceso != NULL) {
		return proceso;
	}

	else {
		pthread_mutex_trylock(&mutexListaProcesos);
		pthread_mutex_unlock(&mutexListaProcesos);

		pthread_mutex_trylock(&mutexListaFrames);
		pthread_mutex_unlock(&mutexListaFrames);

		pthread_exit(NULL);

		return NULL;
	}
}

bool hayFramesLibres(int cantidadDeFrames) {

	int _soy_frame_libre(t_frame* frame) {
		return (frame->pid == 0);
	}

	return list_count_satisfying(listaFrames, (void*) _soy_frame_libre) >= cantidadDeFrames;
}

void* pedirPaginaAFS(uint32_t pid, uint8_t numero_pagina) {

	int bytesRecibidos;

	void* bufferFS = malloc(20);
	uint32_t tamanioValidoBufferFS = 0;

	uint32_t header = MEMORIA;
	memcpy(bufferFS, &header, sizeof(uint32_t));
	tamanioValidoBufferFS += sizeof(uint32_t);

	header = LECTURA_PAGINA;
	memcpy(bufferFS + tamanioValidoBufferFS, &header, sizeof(uint32_t));
	tamanioValidoBufferFS += sizeof(uint32_t);

	memcpy(bufferFS + tamanioValidoBufferFS, &pid, sizeof(uint32_t));
	tamanioValidoBufferFS += sizeof(uint32_t);

	memcpy(bufferFS + tamanioValidoBufferFS, &numero_pagina,
			sizeof(uint8_t));
	tamanioValidoBufferFS += sizeof(uint8_t);

	pthread_mutex_lock(&mutexFS);

	log_info(log_memoria,"Pido a FS pagina %d del proceso %d", numero_pagina,pid);

	int bytesEnviados = send(socket_fs, bufferFS, tamanioValidoBufferFS,0);
	if (bytesEnviados <= 0) {
		log_error(log_memoria,"Error send FS");
		pthread_mutex_unlock(&mutexFS);
		pthread_exit(NULL);
	}

	free(bufferFS);

	bytesRecibidos = recv(socket_fs, &header, sizeof(uint32_t), 0);
	if (bytesRecibidos <= 0) {
		log_info(log_memoria,"El FS se ha desconectado");
		pthread_mutex_unlock(&mutexFS);
		pthread_exit(NULL);
	}

	if (header != FS) {
		log_info(log_memoria,"El FS se desconecto\n");
		pthread_mutex_unlock(&mutexFS);
		pthread_exit(NULL);
	}

	bytesRecibidos = recv(socket_fs, &header, sizeof(uint32_t), 0);
	if (bytesRecibidos <= 0) {
		log_info(log_memoria, "El FS se ha desconectado");
		pthread_mutex_unlock(&mutexFS);
		pthread_exit(NULL);
	}

	if (header != ENVIO_PAGINA) {
		log_info(log_memoria, "Error en recv accion fs");
		pthread_mutex_unlock(&mutexFS);
		pthread_exit(NULL);
	}

	void* paginaLeida = calloc(1, tamanioDeMarcos);

	bytesRecibidos = recv(socket_fs, &pid, sizeof(uint32_t), 0);
	if (bytesRecibidos <= 0) {
		log_info(log_memoria,"El FS se ha desconectado");
		pthread_mutex_unlock(&mutexFS);
		pthread_exit(NULL);
	}

	bytesRecibidos = recv(socket_fs, &numero_pagina, sizeof(uint8_t), 0);
	if (bytesRecibidos <= 0) {
		log_info(log_memoria,"El FS se ha desconectado");
		pthread_mutex_unlock(&mutexFS);
		pthread_exit(NULL);
	}

	bytesRecibidos = recv(socket_fs, paginaLeida, tamanioDeMarcos, 0);
	if (bytesRecibidos <= 0) {
		log_info(log_memoria,"El FS se ha desconectado");
		pthread_mutex_unlock(&mutexFS);
		pthread_exit(NULL);
	}

	log_info(log_memoria,"Recibi de FS pagina %d del proceso %d\n", numero_pagina,pid);

	pthread_mutex_unlock(&mutexFS);

	return paginaLeida;
}

void cargarPaginaAMemoria(uint32_t pid, uint8_t numero_pagina,void* paginaLeida, int accion) {

	t_frame* frame = buscarFrameLibre(pid);

	if (frame != NULL) {

		//-----Retardo
		pthread_mutex_lock(&mutexRetardo);
		usleep(retardoMemoria * 1000);
		pthread_mutex_unlock(&mutexRetardo);
		//------------

		t_pag* pag = buscarPaginaEnListaDePaginas(pid, numero_pagina);

		pag->bit_pres = 1;
		pag->nroFrame = frame->nroFrame;

		if (accion == ESCRITURA_PAGINA)
			frame->bit_modif = 1;
		else
			frame->bit_modif = 0;

		escribirContenido(frame->nroFrame, 0, tamanioDeMarcos, paginaLeida);

		log_info(log_memoria,"Pagina %d del proceso %d cargada en frame %d\n",numero_pagina, pid, frame->nroFrame);

	}

}

t_frame* buscarFrameLibre(uint32_t pid) {

	int _soy_frame_libre(t_frame* frame) {
		return (frame->pid == 0);
	}

	t_proceso* proceso = buscarProcesoEnListaProcesos(pid);

	t_frame* frameLibre;

	if (hayFramesLibres(1)) {

		//SI HAY FRAME LIBRES Y EL PROCESO NO TIENE OCUPADOS TODOS LOS MARCOS POR PROCESO ELIJO CUALQUIER FRAME

		frameLibre = list_find(listaFrames, (void*) _soy_frame_libre);

	}
/*
	else {
		//SI NO HAY FRAMES LIBRES O EL PROCESO TIENE OCUPADOS TODOS LOS MARCOS POR PROCESO
		//REEMPLAZO UN FRAME DE ESE PROCESO

		frameLibre = t_frame*
	}
	*/
	frameLibre->pid = pid;

	proceso->cantFramesAsignados++;

	return frameLibre;

}

void escribirContenido(int frame, int offset, int tamanio_escribir,	void* contenido) {

	//-----Retardo
	pthread_mutex_lock(&mutexRetardo);
	usleep(retardoMemoria * 1000);
	pthread_mutex_unlock(&mutexRetardo);
	//------------

	int desplazamiento = (frame * tamanioDeMarcos) + offset;

	pthread_mutex_lock(&mutexMemoriaReal);
	memcpy(memoria_real + desplazamiento, contenido, tamanio_escribir);
	pthread_mutex_unlock(&mutexMemoriaReal);

	//free(contenido); fixme:liberar en otro momento
}

void inicializarFrames(){
	listaFrames = list_create();

	int i=0;
	for(; i < cantidadDeMarcos ; i++){
		t_frame* frame = malloc(sizeof(t_frame));
		frame->nroFrame = i;
		frame->bit_modif = 0;
		frame->pid = 0;
		list_add(listaFrames, frame);
	}

}

void terminarProceso(){			//aca libero todos

	list_destroy(lista_cpus);
	void destruirFrames(t_frame* frame){
		free(frame);
		fixme(frame);
	}
	list_destroy_and_destroy_elements(listaFrames, (void*) destruirFrames);
	void destruirProcesos(t_proceso* proc){
		free(proc);
	}
	list_destroy_and_destroy_elements(listaProcesos, (void*) destruirProcesos);

	free(memoria_real);

	log_trace(log_memoria, "Termino Proceso");
	log_destroy(log_memoria);
	exit(1);
}

void ponerBitModificadoEn1(int nroFrame) {

	t_frame* frame = buscarFrame(nroFrame);

	frame->bit_modif = 1;

}

t_frame* buscarFrame(int numeroFrame) {

	int _soy_el_frame_buscado(t_frame* frame) {
		return (frame->nroFrame == numeroFrame);
	}

	return list_find(listaFrames, (void*) _soy_el_frame_buscado);
}

int estaEnMemoriaReal(uint32_t pid, uint8_t numero_pagina) {

	lockProcesos();
	t_pag* pagina = buscarPaginaEnListaDePaginas(pid, numero_pagina);
	unlockProcesos();

	if (pagina != NULL) {

		return (pagina->bit_pres);

	} else {

		return 0;

	}
}

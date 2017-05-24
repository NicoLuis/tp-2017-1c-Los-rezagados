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
		void* tmpBuffer;

		header = msg->tipoMensaje;
		t_num8 aux;

		switch (header) {

		case INICIALIZAR_PROGRAMA:

			recv(socketKernel, &aux, sizeof(t_num8), 0);

			cantidadDePaginas = msg->longitud / tamanioDeMarcos;
			cantidadDePaginas = (msg->longitud % tamanioDeMarcos) == 0? cantidadDePaginas: cantidadDePaginas + 1;

			cantidadDePaginas += aux;

			log_info(log_memoria, "Solicitud de inicializar proceso %d con %d paginas", pid, cantidadDePaginas);

			crearProcesoYAgregarAListaDeProcesos(pid, cantidadDePaginas);

			//guardo codigo en memoria

			if(hayFramesLibres(cantidadDePaginas)){
				int i = 0;
				t_frame* marcoLibre;
				for(; i < cantidadDePaginas; i++){
					marcoLibre = buscarFrameLibre(pid);
					tmpBuffer = malloc(tamanioDeMarcos);
					memcpy(tmpBuffer, msg->data + i*tamanioDeMarcos, tamanioDeMarcos);
					escribirContenido(marcoLibre->nroFrame, 0, tamanioDeMarcos, tmpBuffer);
				}
				free(marcoLibre);
				log_info(log_memoria, "Asigne correctamente");
		 		header = 29;//OK
				send(socketKernel, &header, sizeof(uint8_t), 0);
			}else{
				log_warning(log_memoria, "No hay frames libres");
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

	int pidPeticion= 0;
	t_posicion puntero;
	int offset = 0, tmpsize;

	//uint32_t pidAnterior;
	t_proceso* proceso;	//fixme: q es este proceso?
	//list_add(listaCPUs, socketCPU);

	log_info(log_memoria,"Se conecto un CPU\n");

	int bytesEnviados = send(socketCPU, "Conexion Aceptada", 18, 0);
	if (bytesEnviados <= 0) {
		log_error(log_memoria,"Error send CPU");
		pthread_exit(NULL);
	}
	send(socketCPU, &tamanioDeMarcos, sizeof(t_num), 0);

	uint32_t header;
	while (1) {
			t_msg* msg = msg_recibir(socketCPU);
			msg_recibir_data(socketCPU, msg);

			header = msg->tipoMensaje;

			switch (header) {

			case FIN_CPU: {
				//MATO ES HILO
				log_info(log_memoria,"La CPU %d ha finalizado\n", socketCPU);
				pthread_exit(NULL);
			}
			break;
			case LECTURA_PAGINA: {
				pthread_mutex_lock(&mutexCantAccesosMemoria);
				cantAccesosMemoria++;
				pthread_mutex_unlock(&mutexCantAccesosMemoria);

				offset = 0;

				// desserializacion
				memcpy(&pidPeticion, msg->data + offset, tmpsize = sizeof(t_num8));
				offset += tmpsize;
				memcpy(&puntero.pagina, msg->data + offset, tmpsize = sizeof(t_num));
				offset += tmpsize;
				memcpy(&puntero.offset, msg->data + offset, tmpsize = sizeof(t_num));
				offset += tmpsize;
				memcpy(&puntero.size, msg->data + offset, tmpsize = sizeof(t_num));
				offset += tmpsize;

				log_info(log_memoria,"Solicitud de lectura. Pag:%d Offset:%d Size:%d",puntero.pagina, puntero.offset, puntero.size);

				lockProcesos();
				ponerBitUsoEn1(pidPeticion, puntero.pagina);
				unlockProcesos();

				if (paginaInvalida(pidPeticion, puntero.pagina)) {

					log_error(log_memoria, "Stack Overflow proceso %d\n", pidPeticion);

					//AVISO FINALIZACION PROGRAMA A LA CPU
					msg_enviar_separado(STACKOVERFLOW, 1, 0, socketCPU);

				} else if ((Cache_Activada()) && (estaEnCache(pidPeticion, puntero.pagina))) {

					void* contenido_leido = obtenerContenidoSegunCache(pidPeticion,puntero.pagina, offset, puntero.size);
					msg_enviar_separado(LECTURA_PAGINA, puntero.size, contenido_leido, socketCPU);
					free(contenido_leido);

				} else if (estaEnMemoriaReal(pidPeticion, puntero.pagina)) {

					log_info(log_memoria, "La pagina %d esta en Memoria Real\n",puntero.pagina);

					//-----Retardo
					pthread_mutex_lock(&mutexRetardo);
					usleep(retardoMemoria * 1000);
					pthread_mutex_unlock(&mutexRetardo);
					//------------

					lockProcesos();
					t_pag* pagina = buscarPaginaEnListaDePaginas(pidPeticion,puntero.pagina);
					unlockProcesos();

					void* contenido_leido = obtenerContenido(pagina->nroFrame,offset, puntero.size);
					msg_enviar_separado(LECTURA_PAGINA, puntero.size, contenido_leido, socketCPU);
					free(contenido_leido);

					if (Cache_Activada()) {
						agregarEntradaCache(pidPeticion, puntero.pagina, pagina->nroFrame);
					}

				} else {

					lockFrames();
					int hayFrameLibre = hayFramesLibres();
					unlockFrames();

					if ((!hayFrameLibre) && (proceso->cantFramesAsignados == 0)) {

						log_info(log_memoria,"Finalizando programa: %d. No hay frames disponibles\n",pidPeticion);

						//AVISO FINALIZACION PROGRAMA A LA CPU
						msg_enviar_separado(MARCOS_INSUFICIENTES, 1, 0, socketCPU);

					}
				}

			}
			break;


			case ESCRITURA_PAGINA: {

				pthread_mutex_lock(&mutexCantAccesosMemoria);
				cantAccesosMemoria++;
				pthread_mutex_unlock(&mutexCantAccesosMemoria);

				offset = 0;

				// desserializacion
				memcpy(&pidPeticion, msg->data + offset, tmpsize = sizeof(t_num8));
				offset += tmpsize;
				memcpy(&puntero.pagina, msg->data + offset, tmpsize = sizeof(t_num));
				offset += tmpsize;
				memcpy(&puntero.offset, msg->data + offset, tmpsize = sizeof(t_num));
				offset += tmpsize;
				memcpy(&puntero.size, msg->data + offset, tmpsize = sizeof(t_num));
				offset += tmpsize;

				void* contenido_escribir = calloc(1, puntero.size);
				memcpy(&contenido_escribir, msg->data + offset, tmpsize = sizeof(t_num));
				offset += tmpsize;

				log_info(log_memoria, "Solicitud de escritura. Pag:%d Offset:%d Size:%d", puntero.pagina, puntero.offset, puntero.size);

				if (puntero.size == sizeof(t_num)) {
					int contenido;
					memcpy(&contenido, contenido_escribir, sizeof(t_num));
					log_info(log_memoria, "Contenido: %d", contenido);
				}

				lockProcesos();
				ponerBitUsoEn1(pidPeticion, puntero.pagina);
				unlockProcesos();

				if (paginaInvalida(pidPeticion, puntero.pagina)) {

					log_error(log_memoria, "Stack Overflow proceso %d\n", pidPeticion);
					free(contenido_escribir);

					//AVISO FINALIZACION PROGRAMA A LA CPU

					header = FINALIZAR_PROGRAMA;

					bytesEnviados = send(socketCPU, &header, sizeof(uint32_t), 0);
					if (bytesEnviados <= 0) {
						log_error(log_memoria, "Error send CPU");
						pthread_exit(NULL);
					}

				} else if (estaEnMemoriaReal(pidPeticion, puntero.pagina)) {

						log_info(log_memoria, "La pagina %d esta en Memoria Real\n",puntero.pagina);

						//-----Retardo
						pthread_mutex_lock(&mutexRetardo);
						usleep(retardoMemoria * 1000);
						pthread_mutex_unlock(&mutexRetardo);
						//------------

						lockProcesos();
						t_pag* pag = buscarPaginaEnListaDePaginas(pidPeticion, puntero.pagina);
						unlockProcesos();

						escribirContenido(pag->nroFrame, puntero.offset, puntero.size, contenido_escribir);
						free(contenido_escribir);

						lockFrames();
						ponerBitModificadoEn1(pag->nroFrame);
						unlockFrames();

						//ENVIO ESCRITURA OK
						header = ESCRITURA_PAGINA;//OK

						bytesEnviados = send(socketCPU, &header, sizeof(uint32_t), 0);
						if (bytesEnviados <= 0) {
							log_error(log_memoria, "Error send CPU");
							pthread_exit(NULL);
						}

						/*	fixme: necesario? La cant de pags en stack es fija
						//MODIFICAR
						int cantidadDePaginas = puntero.size / tamanioDeMarcos;
						lockFrames();
						int hayFrameLibre = hayFramesLibres(cantidadDePaginas);
						unlockFrames();

						if ((!hayFrameLibre) && (proceso->cantFramesAsignados == 0)) {

							log_info(log_memoria,"Finalizando programa: %d. No hay frames disponibles\n",pidPeticion);

							//AVISO FINALIZACION PROGRAMA A LA CPU
							header = MARCOS_INSUFICIENTES;

							bytesEnviados = send(socketCPU, &header, sizeof(uint32_t),0);
							if (bytesEnviados <= 0) {
								log_error(log_memoria, "Error send CPU");
								pthread_exit(NULL);
							}
						}
						*/

					}
				}

			break;

			case 0:
				log_trace(log_memoria, "Se desconecto cpu %d", socketCPU);
				pthread_exit(NULL);
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

int hayFramesLibres() {

	int _soy_frame_libre(t_frame* frame) {
		return (frame->pid == 0);
	}

	t_frame* frameLibre = list_find(listaFrames, (void*) _soy_frame_libre);

	if (frameLibre != NULL) {
		return 1;
	} else {
		return 0;
	}
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

	free(contenido); //fixme:liberar en otro momento
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

	//list_destroy(listaCPUs);
	void destruirFrames(t_frame* frame){
		free(frame);
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

t_list* crearCache() {
	t_list* cache = list_create();
	return cache;
}

t_cache* buscarEntradaCache(uint32_t pid, uint8_t numero_pagina) {

	int _soy_la_entrada_cache_buscada(t_cache* entradaCache) {
		return ((entradaCache->pid == pid) && (entradaCache->numPag == numero_pagina));
	}

	return list_find(Cache, (void*) _soy_la_entrada_cache_buscada);

}

int estaEnCache(uint32_t pid, uint8_t numero_pagina) {

	pthread_mutex_lock(&mutexCache);
	t_cache* entradaCache = buscarEntradaCache(pid, numero_pagina);

	if (entradaCache != NULL) {
		log_info(log_memoria, "La pagina %d esta en la Cache\n", numero_pagina);
		return 1;
	} else {
		log_info(log_memoria, "Cache Miss\n");
		pthread_mutex_unlock(&mutexCache);
		return 0;
	}
}

t_cache* crearRegistroCache(uint32_t pid, uint8_t numPag, int numFrame) {

	t_cache* unaEntradaCache = malloc(sizeof(t_cache));
	unaEntradaCache->pid = pid;
	unaEntradaCache->numPag = numPag;
	unaEntradaCache->numFrame = numFrame;

	pthread_mutex_lock(&mutexCantAccesosMemoria);
	unaEntradaCache->ultimoAcceso = cantAccesosMemoria;
	pthread_mutex_unlock(&mutexCantAccesosMemoria);

	return unaEntradaCache;
}

void eliminarCache(t_list* unaCache) {
	list_destroy(unaCache);
}

int hayEspacioEnCache() {

	int tamanioCache = list_size(Cache);

	return tamanioCache < cantidadEntradasCache;
}

void agregarEntradaCache(uint32_t pid, uint8_t numero_pagina, int nroFrame) {

	t_cache* nuevaEntradaCache = crearRegistroCache(pid, numero_pagina, nroFrame);

	pthread_mutex_lock(&mutexCache);
	if (hayEspacioEnCache()) {
		list_add(Cache, nuevaEntradaCache);
	}

	else {
		algoritmoLRU();
		list_add(Cache, nuevaEntradaCache);
	}

	log_info(log_memoria, "Pagina %d del proceso %d agregada a Cache\n", numero_pagina,pid);

	pthread_mutex_unlock(&mutexCache);
}

void* obtenerContenidoSegunCache(uint32_t pid, uint8_t numero_pagina,uint32_t offset, uint32_t tamanio_leer) {

	t_cache* entradaCache = buscarEntradaCache(pid, numero_pagina);

	pthread_mutex_lock(&mutexCantAccesosMemoria);
	entradaCache->ultimoAcceso = cantAccesosMemoria;
	pthread_mutex_unlock(&mutexCantAccesosMemoria);

	void* contenido = obtenerContenido(entradaCache->numFrame, offset,tamanio_leer);

	pthread_mutex_unlock(&mutexCache);

	return contenido;

}

void escribirContenidoSegunCache(uint32_t pid, uint8_t numero_pagina,uint32_t offset, uint32_t tamanio_escritura, void* contenido_escribir) {

	t_cache* entradaCache = buscarEntradaCache(pid, numero_pagina);

	pthread_mutex_lock(&mutexCantAccesosMemoria);
	entradaCache->ultimoAcceso = cantAccesosMemoria;
	pthread_mutex_unlock(&mutexCantAccesosMemoria);

	escribirContenido(entradaCache->numFrame, offset, tamanio_escritura,contenido_escribir);

	ponerBitModificadoEn1(entradaCache->numFrame);

	pthread_mutex_unlock(&mutexCache);

}

void vaciarCache() {

	if (Cache_Activada()) {
		pthread_mutex_lock(&mutexCache);
		eliminarCache(Cache);
		Cache = crearCache();
		pthread_mutex_unlock(&mutexCache);

		printf("Flush Cache realizado\n");
		log_info(log_memoria, "Flush Cache realizado\n");

	} else
		printf("Se intento hacer flush cache pero no esta activada\n");
		log_error(log_memoria, "Se intento hacer flush cache pero no esta activada\n");

}

void borrarEntradasCacheSegunPID(uint32_t pid) {

	int _no_es_entrada_Cache_de_PID(t_cache* entradaCache) {
		return (entradaCache->pid != pid);
	}

	pthread_mutex_lock(&mutexCache);

	t_list* nuevaCache = list_filter(Cache, (void*) _no_es_entrada_Cache_de_PID);

	list_destroy(Cache);

	Cache = nuevaCache;

	pthread_mutex_unlock(&mutexCache);
}

void borrarEntradaCacheSegunFrame(int nroFrame) {

	int index = -1;

	int _soy_la_entrada_Cache_de_ese_frame(t_cache* entradaCache) {

		index++;
		return (entradaCache->numFrame == nroFrame);
	}

	pthread_mutex_lock(&mutexCache);

	list_find(Cache, (void*) _soy_la_entrada_Cache_de_ese_frame);

	t_cache* entrada_Cache_borrada = list_remove(Cache, index);

	free(entrada_Cache_borrada);

	pthread_mutex_unlock(&mutexCache);

}

int Cache_Activada() {

	return (cantidadaEntradasCache > 0);

}

void algoritmoLRU() {

	int _menor_ultimo_acceso(t_cache* unRegistroTLB, t_cache* otroRegistroTLB) {
		return (unRegistroTLB->ultimoAcceso < otroRegistroTLB->ultimoAcceso);
	}

	log_info(log_memoria, "Algoritmo LRU");

	list_sort(Cache, (void*) _menor_ultimo_acceso);

	t_cache* entradaTLBreemplazada = list_remove(Cache, 0);

	log_info(log_memoria, "En TLB se reemplazo la pagina %d del proceso %d",entradaTLBreemplazada->numPag, entradaTLBreemplazada->pid);

	free(entradaTLBreemplazada);
}


void ejecutarComandos() {
	char buffer[1000];

	while (1) {
		printf("Ingrese comando:\n");
		scanf("%s", buffer);

		if (!strcmp(buffer, "retardo")) {

			int retardoNuevo;

			printf("\nIngrese nuevo valor de retardo:\n");
			scanf("%d", &retardoNuevo);

			pthread_mutex_lock(&mutexRetardo);
			retardoMemoria = retardoNuevo;
			pthread_mutex_unlock(&mutexRetardo);

			printf("Retardo modificado correctamente a %d ms\n\n", retardoMemoria);

			log_info(log_memoria, "Retardo modificado correctamente a %d ms\n\n",retardoMemoria);

		}
	else if (!strcmp(buffer, "dump")) {

			int pid;

			printf("Ingrese 0 si quiere un reporte de todos los procesos o ingrese el pid del proceso para un reporte particular\n");
			scanf("%d", &pid);

			lockFramesYProcesos();

			if (pid == 0) {
				dumpTodosLosProcesos();

			} else {
				dumpProcesoParticular(pid);
			}

			unlockFramesYProcesos();

			}
	else if (!strcmp(buffer, "flush")) {

			printf("Escriba cache o memory\n");
			scanf("%s", buffer);

			if (!strcmp(buffer, "cache")) {

				vaciarCache();

			} else if (!strcmp(buffer, "memory")) {

				lockFrames();

				flushMemoria();

				unlockFrames();

				printf("Flush memory realizado\n\n");

				log_info(log_memoria, "Flush memory realizado\n\n");

			} else {
				printf("Comando Incorrecto\n");
			}

		} else if(!strcmp(buffer,"size")) {
			printf("Ingrese memoria o pid\n\n");
			scanf("%s",buffer);
			if(!strcmp(buffer,"memoria")){
				printf("La cantidad de frames en memoria es de : %d\n\n",cantidadDeMarcos);

				int cantidadDeFrames = list_size(listaFrames);
				printf("La cantidad de frames segun lista de frames es de : %d\n\n",cantidadDeFrames);

				int frameUsado(t_frame* frame) {
						return (frame->bit_modif != 0);
					}

				int cantidadDeFramesLlenos = list_size(list_filter(listaFrames,(void*) frameUsado));
				printf("La cantidad de frames usados es de : %d\n\n",cantidadDeFramesLlenos);

				int cantidadDeFramesVacios = cantidadDeFrames - cantidadDeFramesLlenos;
				printf("La cantidad de frames vacios es de %d\n\n",cantidadDeFramesVacios);

			}
			else if(!strcmp(buffer,"pid")){

				int pid;
				printf("Ingrese el numero de PID:\n\n");
				scanf("%d",&pid);

				t_proceso* proceso = buscarProcesoEnListaProcesosParaDump(pid);

				if (proceso == NULL) {
					printf("No existe ese proceso\n");

				}
				else{
					printf("El tamaño es de %d paginas\n\n",proceso->cantPaginas);
					printf("El tamaño es de %d frames\n\n",proceso->cantFramesAsignados);
				}
			}
			else{
				printf("Comando Incorrecto\n");
			}
		}
		else{
			printf("Comando Incorrecto\n\n");
		}

	}
}

void dumpTablaDePaginasDeProceso(t_proceso* proceso, FILE* archivoDump) {

	int sizeListaPaginas = list_size(proceso->listaPaginas);
	int i = 0;
	fprintf(archivoDump, "Tabla de paginas:\n\n");
	printf("Tabla de paginas:\n\n");

	fprintf(archivoDump,
			"Nro de pagina\tNro de frame\tBit de presencia\tBit de modificado\n");
	printf(
			"Nro de pagina\tNro de frame\tBit de presencia\tBit de modificado\n");

	while (i < sizeListaPaginas) {
		t_pag* pagina = buscarPaginaEnListaDePaginas(proceso->PID, i);

		if (pagina->bit_pres == 1) {
			t_frame* frame = buscarFrame(pagina->nroFrame);

			fprintf(archivoDump,
					"      %d             %d                  %d                       %d\n",
					pagina->nroPag, pagina->nroFrame, pagina->bit_pres,
					frame->bit_modif);
			printf(
					"      %d             %d                  %d                       %d\n",
					pagina->nroPag, pagina->nroFrame, pagina->bit_pres,
					frame->bit_modif);
		} else {
			fprintf(archivoDump,
					"      %d             %d                  %d                       %d\n",
					pagina->nroPag, pagina->nroFrame, pagina->bit_pres, 0);
			printf(
					"      %d             %d                  %d                       %d\n",
					pagina->nroPag, pagina->nroFrame, pagina->bit_pres, 0);
		}

		i++;
	}
}

void dumpEstructurasMemoriaTodosLosProcesos(FILE* archivoDump) {

	int cantidadProcesos = list_size(listaProcesos);
	int i = 0;

	while (i < cantidadProcesos) {
		t_proceso* proceso = list_get(listaProcesos, i);
		fprintf(archivoDump,
				"\n//////////////ESTRUCTURA DE MEMORIA PROCESO %d//////////////\n",
				proceso->PID);
		printf(
				"\n//////////////ESTRUCTURA DE MEMORIA PROCESO %d//////////////\n",
				proceso->PID);
		fprintf(archivoDump, "\nCantidad de frames asignados: %d\n",
				proceso->cantFramesAsignados);
		printf("\nCantidad de frames asignados: %d\n",
				proceso->cantFramesAsignados);
		fprintf(archivoDump, "\nCantidad de paginas: %d\n",
				proceso->cantPaginas);
		printf("\nCantidad de paginas: %d\n", proceso->cantPaginas);

		dumpTablaDePaginasDeProceso(proceso, archivoDump);
		i++;
	}
}

void dumpTodosLosProcesos() {

	char nombreArchivoDump[40] = "dumpTotal_";
	int pidLinux = process_getpid();
	strcat(nombreArchivoDump, string_itoa(pidLinux));
	strcat(nombreArchivoDump, ".txt");
	FILE* archivoDump = fopen(nombreArchivoDump, "w+");

	unsigned int tamanioTotalMemoria = tamanioDeMarcos * cantidadDeMarcos;

	dumpEstructurasMemoriaTodosLosProcesos(archivoDump);

	fprintf(archivoDump,
			"\n//////////////////MEMORIA PRINCIPAL//////////////////\n");

	printf("\n//////////////////MEMORIA PRINCIPAL//////////////////\n\n");

	pthread_mutex_lock(&mutexMemoriaReal);
	//hexdump(memoria_real, tamanioTotalMemoria, archivoDump);
	pthread_mutex_unlock(&mutexMemoriaReal);

	printf("\nDUMP DE MEMORIA PRINCIPAL PARA TODOS LOS PROCESOS REALIZADO\n");
	fprintf(archivoDump,
			"\n//////////////////MEMORIA PRINCIPAL//////////////////\n");

	fclose(archivoDump);
}

void dumpContenidoMemoriaProceso(t_proceso* proceso, FILE* archivoDump) {

	int i = 0;
	void* contenidoFrame;

	fprintf(archivoDump,
			"\n//////////////////MEMORIA PRINCIPAL//////////////////\n");

	printf("\n//////////////////MEMORIA PRINCIPAL//////////////////\n\n");

	while (i < proceso->cantPaginas) {
		t_pag* pagina = buscarPaginaEnListaDePaginas(proceso->PID, i);
		if (pagina->bit_pres == 1) {
			t_frame* frame = buscarFrame(pagina->nroFrame);
			contenidoFrame = obtenerContenido(frame->nroFrame, 0,tamanioDeMarcos);
			//hexdump(contenidoFrame, tamanioDeMarcos, archivoDump);
		}
		i++;
	}

	printf("\nDUMP DE MEMORIA PRINCIPAL PARA EL PROCESO %d REALIZADO\n",
			proceso->PID);
	fprintf(archivoDump,
			"\n//////////////////MEMORIA PRINCIPAL//////////////////\n");
}

void dumpProcesoParticular(int pid) {

	t_proceso* proceso = buscarProcesoEnListaProcesosParaDump(pid);

	if (proceso == NULL) {
		printf("No existe ese proceso\n");

	} else {

		char nombreArchivoDump[30] = "dumpProceso";
		int pidLinux = process_getpid();
		strcat(nombreArchivoDump, "_");
		strcat(nombreArchivoDump, string_itoa(pid));
		strcat(nombreArchivoDump, "_");
		strcat(nombreArchivoDump, string_itoa(pidLinux));
		strcat(nombreArchivoDump, ".txt");
		FILE* archivoDump = fopen(nombreArchivoDump, "w+");

		fprintf(archivoDump,
				"\n//////////////ESTRUCTURA DE MEMORIA PROCESO %d//////////////\n",
				proceso->PID);
		printf(
				"\n//////////////ESTRUCTURA DE MEMORIA PROCESO %d//////////////\n",
				proceso->PID);
		fprintf(archivoDump, "\nCantidad de frames asignados: %d\n",
				proceso->cantFramesAsignados);
		printf("\nCantidad de frames asignados: %d\n",
				proceso->cantFramesAsignados);
		fprintf(archivoDump, "\nCantidad de paginas: %d\n",
				proceso->cantPaginas);
		printf("\nCantidad de paginas: %d\n", proceso->cantPaginas);

		dumpTablaDePaginasDeProceso(proceso, archivoDump);
		dumpContenidoMemoriaProceso(proceso, archivoDump);

		fclose(archivoDump);
	}


}

void flushMemoria() {

	int i = 0;

	while (i < cantidadDeMarcos) {

		t_frame* frame = list_get(listaFrames, i);

		if (frame->pid != 0) {

			frame->bit_modif = 1;
		}

		i++;
	}
}


t_proceso* buscarProcesoEnListaProcesosParaDump(uint32_t pid) {

	int _soy_el_pid_buscado(t_proceso* proceso) {
		return (proceso->PID == pid);
	}

	return list_find(listaProcesos, (void*) _soy_el_pid_buscado);
}


void mostrarContenidoTodosLosProcesos(){
	int size = list_size(listaProcesos);
	int aux = 0;
	while (aux <= size){
		t_proceso* proceso = list_get(listaProcesos,aux);
		mostrarContenidoDeUnProceso(proceso->PID);
	}
}

void mostrarContenidoDeUnProceso(uint32_t pid){

	t_proceso* proceso = buscarProcesoEnListaProcesos(pid);
	int numeroPagina = 0;
	while(numeroPagina <= proceso->cantPaginas){
		lockProcesos();
		t_pag* pagina = buscarPaginaEnListaDePaginas(pid,numeroPagina);
		unlockProcesos();
		void* contenido_leido = obtenerContenido(pagina->nroFrame, 0, tamanioDeMarcos);

		printf("Numero de Pagina: %d \n",numeroPagina);
		printf("Contenido: %p",contenido_leido);

		pagina++;
	}

}

/*
 * libreriaMemoria.c
 *
 *  Created on: 3/4/2017
 *      Author: utnso
 */

#include "libreriaMemoria.h"

void mostrarArchivoConfig() {

	printf("El puerto de la MEMORIA es: %d\n", puertoMemoria);
	printf("La cantidad de Marcos es: %d\n", cantidadDeMarcos);
	printf("El tamaño de las Marcos es: %d\n", tamanioDeMarcos);
	printf("La cantidad de entradas de Cache habilitadas son: %d\n", cantidadaEntradasCache);
	printf("La cantidad maxima de entradas a la cache por proceso son: %d\n", cachePorProceso);
	printf("El algoritmo de reemplaco de la Cache es: %s\n", algoritmoReemplazo);
	printf("El retardo de la memoria es de: %d\n",retardoMemoria);

}


void escucharKERNEL(void* socket_kernel) {

	//Casteo socketKernel
	int socketKernel = (int) socket_kernel;

	uint32_t pid;
	uint32_t tamanioCodigo;
	uint32_t cantidadDePaginas;

	printf("Se conecto el Kernel\n");

	int bytesEnviados = send(socketKernel, "Conexion Aceptada", 18, 0);
	if (bytesEnviados <= 0) {
		printf("Error send Kernel");
		pthread_exit(NULL);
	}

	uint32_t header;
	while (1) {

		void* bufferKernel = malloc(1000);
		uint32_t tamanioValidoBufferKernel = 0;

		int bytesRecibidos = recv(socketKernel, &header, sizeof(uint32_t), 0);
		if (bytesRecibidos <= 0) {
			printf("El Kernel se ha desconectado");
			pthread_exit(NULL);
		}

		if (header != KERNEL) {
			printf("Error recv header nucleo");
			pthread_exit(NULL);
		}

		bytesRecibidos = recv(socketKernel, &header, sizeof(uint32_t), 0);
		if (bytesRecibidos <= 0) {
			printf("El Kernel se ha desconectado");
			pthread_exit(NULL);
		}

		switch (header) {
		case INICIALIZAR_PROGRAMA: {

			bytesRecibidos = recv(socketKernel, &pid, sizeof(uint32_t), 0);
			if (bytesRecibidos <= 0) {
					printf("El Kernel se ha desconectado");
					pthread_exit(NULL);
				}

			bytesRecibidos = recv(socketKernel, &cantidadDePaginas,sizeof(uint32_t), 0);
			if (bytesRecibidos <= 0) {
					printf("El Kernel se ha desconectado");
					pthread_exit(NULL);
				}

			bytesRecibidos = recv(socketKernel, &tamanioCodigo,sizeof(uint32_t), 0);
			if (bytesRecibidos <= 0) {
					printf("El Kernel se ha desconectado");
					pthread_exit(NULL);
				}

			void* codigoAnSISOP = calloc(cantidadDePaginas, tamanioDeMarcos);

			bytesRecibidos = recv(socketKernel, codigoAnSISOP, tamanioCodigo,0);
			if (bytesRecibidos <= 0) {
				printf("El Kernel se ha desconectado");
				pthread_exit(NULL);
			}

			printf("Solicitud de inicializar proceso %d con %d paginas", pid,cantidadDePaginas);

			//Inicializo programa en FS

			void* bufferFS = malloc(50);
			uint32_t tamanioValidoBufferFS = 0;

			header = MEMORIA;
			memcpy(bufferFS, &header, sizeof(uint32_t));
			tamanioValidoBufferFS += sizeof(uint32_t);

			header = INICIALIZAR_PROGRAMA;
			memcpy(bufferFS + tamanioValidoBufferFS, &header,
			sizeof(uint32_t));
			tamanioValidoBufferFS += sizeof(uint32_t);

			memcpy(bufferFS + tamanioValidoBufferFS, &pid,sizeof(uint32_t));
			tamanioValidoBufferFS += sizeof(uint32_t);

			memcpy(bufferFS + tamanioValidoBufferFS, &cantidadDePaginas,sizeof(uint32_t));
			tamanioValidoBufferFS += sizeof(uint32_t);

			pthread_mutex_lock(&mutexFS);

			bytesEnviados = send(socket_fs, bufferFS,tamanioValidoBufferFS, 0);
			if (bytesEnviados <= 0) {
				printf("Error send FS");
				pthread_mutex_unlock(&mutexFS);
				pthread_exit(NULL);
				}

			free(bufferFS);

			//espero la respuesta del FS si hay o no espacio

			bytesRecibidos = recv(socket_fs, &header, sizeof(uint32_t), 0);
			if (bytesRecibidos <= 0) {
				printf("El FS se ha desconectado");
				pthread_mutex_unlock(&mutexFS);
				pthread_exit(NULL);
			}

			if (header != FS) {
				printf("Error recv header FS");
				pthread_mutex_unlock(&mutexFS);
				pthread_exit(NULL);
			}

			bytesRecibidos = recv(socket_fs, &header, sizeof(uint32_t), 0);
			if (bytesRecibidos <= 0) {
				printf("El FS se ha desconectado");
				pthread_mutex_unlock(&mutexFS);
				pthread_exit(NULL);
			}

			switch (header) {
			case ESPACIO_INSUFICIENTE: {
				tamanioValidoBufferKernel = 0;

				bytesRecibidos = recv(socket_fs, &pid, sizeof(uint32_t), 0);
				if (bytesRecibidos <= 0) {
					printf("El FS se ha desconectado");
					pthread_mutex_unlock(&mutexFS);
					pthread_exit(NULL);
					}

				pthread_mutex_unlock(&mutexFS);

				printf("Espacio insuficiente para proceso: %d\n",pid);

				header = MEMORIA;
				memcpy(bufferKernel, &header, sizeof(uint32_t));
				tamanioValidoBufferKernel += sizeof(uint32_t);

				header = ESPACIO_INSUFICIENTE;
				memcpy(bufferKernel + tamanioValidoBufferKernel, &header,sizeof(uint32_t));
				tamanioValidoBufferKernel += sizeof(uint32_t);

				memcpy(bufferKernel + tamanioValidoBufferKernel, &pid,sizeof(uint32_t));
				tamanioValidoBufferKernel += sizeof(uint32_t);

				bytesEnviados = send(socketKernel, bufferKernel,tamanioValidoBufferKernel, 0);
				if (bytesEnviados <= 0) {
					printf("Error send Kernel");
					pthread_exit(NULL);
				}

				free(bufferKernel);
			}
				break;

			case ESPACIO_SUFICIENTE: {
				bytesRecibidos = recv(socket_fs, &pid, sizeof(uint32_t), 0);
				if (bytesRecibidos <= 0) {
					printf("El FS se ha desconectado");
					pthread_mutex_unlock(&mutexFS);
					pthread_exit(NULL);
				}

				pthread_mutex_unlock(&mutexFS);

				printf("Espacio suficiente para proceso: %d\n", pid);

				//Crear el proceso inicializando su lista de pags y agregarlo a la lista de procesos

				lockProcesos();

				crearProcesoYAgregarAListaDeProcesos(pid, cantidadDePaginas);

				unlockProcesos();

				//Enviarle el codigo al FS pagina por pagina

				uint8_t nroPag = 0;
				while (nroPag < cantidadDePaginas) {

					escribirPaginaEnFS(pid, nroPag,	codigoAnSISOP + (nroPag * tamanioDeMarcos));

					nroPag++;

				}

				free(codigoAnSISOP);

				tamanioValidoBufferKernel = 0;

				header = MEMORIA;
				memcpy(bufferKernel, &header, sizeof(uint32_t));
				tamanioValidoBufferKernel += sizeof(uint32_t);

				header = ESPACIO_SUFICIENTE;
				memcpy(bufferKernel + tamanioValidoBufferKernel, &header,sizeof(uint32_t));
				tamanioValidoBufferKernel += sizeof(uint32_t);

				memcpy(bufferKernel + tamanioValidoBufferKernel, &pid,sizeof(uint32_t));
				tamanioValidoBufferKernel += sizeof(uint32_t);

				bytesEnviados = send(socketKernel, bufferKernel,tamanioValidoBufferKernel, 0);
				if (bytesEnviados <= 0) {
					printf("Error send Kernel");
					pthread_exit(NULL);
				}

				free(bufferKernel);
			}
				break;
			}

		}
			break;
		case FINALIZAR_PROGRAMA: {
			bytesRecibidos = recv(socketKernel, &pid, sizeof(uint32_t), 0);
			if (bytesRecibidos <= 0) {
				printf("El Kernel se ha desconectado");
				pthread_exit(NULL);
			}

			printf("Finalizando proceso %d\n", pid);

			lockFramesYProcesos();

			liberarFramesDeProceso(pid);
			eliminarProcesoDeListaDeProcesos(pid);

			unlockFramesYProcesos();

			void* bufferFS = malloc(20);
			uint32_t tamanioValidoBufferFS = 0;

			header = MEMORIA;
			memcpy(bufferFS, &header, sizeof(uint32_t));
			tamanioValidoBufferFS += sizeof(uint32_t);

			header = FINALIZAR_PROGRAMA;
			memcpy(bufferFS + tamanioValidoBufferFS, &header,sizeof(uint32_t));
			tamanioValidoBufferFS += sizeof(uint32_t);

			memcpy(bufferFS + tamanioValidoBufferFS, &pid,sizeof(uint32_t));
			tamanioValidoBufferFS += sizeof(uint32_t);

			pthread_mutex_lock(&mutexFS);

			bytesEnviados = send(socket_fs, bufferFS,tamanioValidoBufferFS, 0);

			if (bytesEnviados <= 0) {

				printf("Error Send Finalizar Programa en FS\n");

				pthread_mutex_unlock(&mutexFS);

				free(bufferFS);
				free(bufferKernel);

				pthread_exit(NULL);
			}

			pthread_mutex_unlock(&mutexFS);

			free(bufferFS);
			free(bufferKernel);

		}
			break;
		}

	}
}

void escucharCPU(void* socket_cpu) {

	//Casteo socketCPU
	int socketCPU = (int) socket_cpu;

	//Casteo socket_cpu

	list_add(lista_cpus, socket_cpu);

	printf("Se conecto un CPU\n");

	int bytesEnviados = send(socketCPU, "Conexion Aceptada", 18, 0);
	if (bytesEnviados <= 0) {
		printf("Error send CPU");
		pthread_exit(NULL);
	}

	uint32_t header;
	int bytesRecibidos;
	while (1) {

			bytesRecibidos = recv(socket_cpu, &header, sizeof(uint32_t), 0);
			if (bytesRecibidos <= 0) {
				printf("Error recv CPU");
				pthread_exit(NULL);
			}

			if (header != CPU) {
				printf("La CPU se ha desconectado o etc");
				pthread_exit(NULL);
			}

			bytesRecibidos = recv(socket_cpu, &header, sizeof(uint32_t), 0);
			if (bytesRecibidos <= 0) {
				printf("Error recv CPU");
				pthread_exit(NULL);
			}

			switch (header) {
			case CAMBIO_PROCESO_ACTIVO: {

				}

			}
			break;
			case FIN_CPU: {
				printf("La CPU %d ha finalizado\n", socket_cpu);
				pthread_exit(NULL);
			}
			break;
			case LECTURA_PAGINA: {

			}
				break;

			case ESCRITURA_PAGINA: {

			}
				break;

			default:
				printf("Error");
			}

}











void* reservarMemoria(int cantidadMarcos, int capacidadMarco) {
	// La creo con calloc para que me la llene de \0
	void * memoria = calloc(cantidadMarcos, capacidadMarco);
	printf("Memoria reservada\n");
	return memoria;
}

void lockProcesos() {

	pthread_mutex_lock(&mutexListaProcesos);

}
void unlockProcesos() {

	pthread_mutex_unlock(&mutexListaProcesos);

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
		printf("Error escritura en FS");
		pthread_mutex_unlock(&mutexFS);
		pthread_exit(NULL);
	}

	printf("Escribi en FS pagina %d del proceso %d\n", nroPag, pid);

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

		printf("Frame %d liberado", frame->nroFrame);

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


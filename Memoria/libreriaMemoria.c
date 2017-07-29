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
	printf("La cantidad de entradas de Cache habilitadas son: %d\n", cantidadEntradasCache);
	log_info(log_memoria,"La cantidad de entradas de Cache habilitadas son: %d", cantidadEntradasCache);
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

	t_pid pid;
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

		if (recv(socketKernel, &pid, sizeof(t_pid), 0) <= 0) {
			log_info(log_memoria,"El Kernel se ha desconectado");
			pthread_exit(NULL);
		}
		//log_info(log_memoria, "Recibo solicitud proceso %d", pid);

		t_msg* msg = msg_recibir(socketKernel);
		if(msg->longitud > 0)
			msg_recibir_data(socketKernel, msg);

		header = msg->tipoMensaje;
		t_num16 stackSize;
		t_posicion puntero;
		t_HeapMetadata metadata, metadata2;

		switch (header) {

		case INICIALIZAR_PROGRAMA:

			recv(socketKernel, &stackSize, sizeof(t_num16), 0);

			cantidadDePaginas = msg->longitud / tamanioDeMarcos;
			cantidadDePaginas = (msg->longitud % tamanioDeMarcos) == 0? cantidadDePaginas: cantidadDePaginas + 1;

			cantidadDePaginas += stackSize;

			log_info(log_memoria, " --- --- Solicitud de inicializar proceso %d con %d paginas", pid, cantidadDePaginas);

			crearProcesoYAgregarAListaDeProcesos(pid);

			//guardo codigo en memoria

			log_info(log_memoria, "\n%s", msg->data);

			if(cantFramesLibres() >= cantidadDePaginas){

				//-----Retardo
				pthread_mutex_lock(&mutexRetardo);
				usleep(retardoMemoria * 1000);
				pthread_mutex_unlock(&mutexRetardo);
				//------------

				int i = 0, nroFrame;
				for(; i < cantidadDePaginas; i++){
					nroFrame = buscarFrameLibre(pid);
					int desplazamientoMemoria = (nroFrame * tamanioDeMarcos);

					log_info(log_memoria, "Escribo %d bytes - frame %d a partir de %d", tamanioDeMarcos, nroFrame, 0);
					pthread_mutex_lock(&mutexMemoriaReal);
					memcpy(memoria_real + desplazamientoMemoria, msg->data + i*tamanioDeMarcos, tamanioDeMarcos);
					pthread_mutex_unlock(&mutexMemoriaReal);
				}
				log_info(log_memoria, "Asigne correctamente");
		 		header = OK;
				send(socketKernel, &header, sizeof(uint8_t), 0);
			}else{
				log_warning(log_memoria, "No hay frames libres");
				header = MARCOS_INSUFICIENTES;
				send(socketKernel, &header, sizeof(uint8_t), 0);
				eliminarProcesoDeListaDeProcesos(pid);
			}
			break;

		case LECTURA_PAGINA:

			pthread_mutex_lock(&mutexCantAccesosMemoria);
			cantAccesosMemoria++;
			pthread_mutex_unlock(&mutexCantAccesosMemoria);

			memcpy(&puntero, msg->data, sizeof(t_posicion));
			log_info(log_memoria, " --- --- Solicitud de lectura. PID:%d Pag:%d Offset:%d Size:%d", pid, puntero.pagina, puntero.offset, puntero.size);

			if (paginaInvalida(pid, puntero.pagina)) {

				log_error(log_memoria, "pagina %d invalida proceso %d", puntero.pagina, pid);
				msg_enviar_separado(PAGINA_INVALIDA, 0, 0, socketKernel);

			} else if (Cache_Activada()) {

				if(!estaEnCache(pid, puntero.pagina))
					agregarEntradaCache(pid, puntero.pagina);

				void* contenido_leido = obtenerContenidoSegunCache(pid, puntero);

				t_HeapMetadata h;
				memcpy(&h, contenido_leido, sizeof(t_HeapMetadata) );

				log_info(log_memoria, "Heap %d %d", h.isFree, h.size);


				msg_enviar_separado(LECTURA_PAGINA, puntero.size, contenido_leido, socketKernel);
				free(contenido_leido);

			} else {

				//-----Retardo
				pthread_mutex_lock(&mutexRetardo);
				usleep(retardoMemoria * 1000);
				pthread_mutex_unlock(&mutexRetardo);
				//------------

				void* contenido_leido = obtenerContenido(pid, puntero);

				if(contenido_leido == NULL)
					msg_enviar_separado(LECTURA_PAGINA, puntero.size, contenido_leido, socketKernel);
				else{
					msg_enviar_separado(LECTURA_PAGINA, puntero.size, contenido_leido, socketKernel);
					free(contenido_leido);
				}

			}


			break;

		case ESCRITURA_PAGINA:

			pthread_mutex_lock(&mutexCantAccesosMemoria);
			cantAccesosMemoria++;
			pthread_mutex_unlock(&mutexCantAccesosMemoria);

			memcpy(&puntero, msg->data, sizeof(t_posicion));
			memcpy(&metadata, msg->data + sizeof(t_posicion), sizeof(t_HeapMetadata));
			if(msg->longitud == sizeof(t_posicion) + sizeof(t_HeapMetadata)*2)
				memcpy(&metadata2, msg->data + sizeof(t_posicion)+sizeof(t_HeapMetadata), sizeof(t_HeapMetadata));

			log_info(log_memoria, " --- --- Solicitud de escritura. PID: %d Pag:%d Offset:%d Size:%d", pid, puntero.pagina, puntero.offset, puntero.size);
			log_info(log_memoria, "Heap %d %d", metadata.isFree, metadata.size);

			void* basura = malloc(metadata.size);
			bzero(basura, metadata.size);

			if (paginaInvalida(pid, puntero.pagina)) {

				log_error(log_memoria, "pagina %d invalida proceso %d", puntero.pagina, pid);
				msg_enviar_separado(PAGINA_INVALIDA, 0, 0, socketKernel);

			} else if (Cache_Activada()) {

				if(!estaEnCache(pid, puntero.pagina))
					agregarEntradaCache(pid, puntero.pagina);

				//todo considerar nroPag
				puntero.size = sizeof(t_HeapMetadata);
				escribirContenidoSegunCache(pid, puntero, &metadata);
				puntero.size = metadata.size;
				puntero.offset = puntero.offset + sizeof(t_HeapMetadata);
				escribirContenidoSegunCache(pid, puntero, basura);

				if(msg->longitud == sizeof(t_posicion) + sizeof(t_HeapMetadata)*2){
					// significa q tengo q guardar otro t_HeapMetadata
					puntero.size = sizeof(t_HeapMetadata);
					puntero.offset = puntero.offset + metadata.size;
					escribirContenidoSegunCache(pid, puntero, &metadata2);
				}

				msg_enviar_separado(ESCRITURA_PAGINA, 0, 0, socketKernel);

			} else {

				//-----Retardo
				pthread_mutex_lock(&mutexRetardo);
				usleep(retardoMemoria * 1000);
				pthread_mutex_unlock(&mutexRetardo);
				//------------


				//todo considerar nroPag
				puntero.size = sizeof(t_HeapMetadata);
				if( escribirContenido(pid, puntero, &metadata) == -1)
					msg_enviar_separado(EXCEPCION_MEMORIA, 0, 0, socketKernel);
				else{

					puntero.size = metadata.size;
					puntero.offset = puntero.offset + sizeof(t_HeapMetadata);
					if( escribirContenido(pid, puntero, basura) == -1)
						msg_enviar_separado(EXCEPCION_MEMORIA, 0, 0, socketKernel);
					else{

						if(msg->longitud == sizeof(t_posicion) + sizeof(t_HeapMetadata)*2){
							// significa q tengo q guardar otro t_HeapMetadata
							puntero.size = sizeof(t_HeapMetadata);
							puntero.offset = puntero.offset + metadata.size;
							if( escribirContenido(pid, puntero, &metadata2) == -1)
								msg_enviar_separado(EXCEPCION_MEMORIA, 0, 0, socketKernel);
							else{
								msg_enviar_separado(ESCRITURA_PAGINA, 0, 0, socketKernel);
								log_info(log_memoria, "Escribi correctamente");
							}
						}else{
							msg_enviar_separado(ESCRITURA_PAGINA, 0, 0, socketKernel);
							log_info(log_memoria, "Escribi correctamente");
						}
					}
				}
			}
			free(basura);

		break;

		case ASIGNACION_MEMORIA:

			log_trace(log_memoria, " --- --- Solicitud nueva pagina pid %d", pid);

			pthread_mutex_lock(&mutexCantAccesosMemoria);
			cantAccesosMemoria++;
			pthread_mutex_unlock(&mutexCantAccesosMemoria);

			//-----Retardo
			pthread_mutex_lock(&mutexRetardo);
			usleep(retardoMemoria * 1000);
			pthread_mutex_unlock(&mutexRetardo);
			//------------

			int nroFrame = buscarFrameLibre(pid);

			if(nroFrame == -1){
				log_error(log_memoria, "No hay mas frames libres");
				msg_enviar_separado(PAGINA_INVALIDA, 0, 0, socketKernel);
			}else
				msg_enviar_separado(ASIGNACION_MEMORIA, 0, 0, socketKernel);

			break;
		case LIBERAR:

			pthread_mutex_lock(&mutexCantAccesosMemoria);
			cantAccesosMemoria++;
			pthread_mutex_unlock(&mutexCantAccesosMemoria);

			//-----Retardo
			pthread_mutex_lock(&mutexRetardo);
			usleep(retardoMemoria * 1000);
			pthread_mutex_unlock(&mutexRetardo);
			//------------

			memcpy(&puntero, msg->data, sizeof(t_posicion));
			log_info(log_memoria, " --- --- Solicitud liberar PID:%d Pag:%d", pid, puntero.pagina);

			liberarPagina(pid, puntero.pagina);

			msg_enviar_separado(LIBERAR, 0, 0, socketKernel);

			break;
		case FINALIZAR_PROGRAMA:
			log_info(log_memoria,"Finalizando proceso %d", pid);

			if(Cache_Activada())
				liberarCacheDeProceso(pid);
			liberarFramesDeProceso(pid);
			eliminarProcesoDeListaDeProcesos(pid);

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

	log_info(log_memoria,"Se conecto un CPU");

	int bytesEnviados = send(socketCPU, "Conexion Aceptada", 18, 0);
	if (bytesEnviados <= 0) {
		log_error(log_memoria,"Error send CPU");
		pthread_exit(NULL);
	}
	send(socketCPU, &tamanioDeMarcos, sizeof(t_num), 0);

	uint32_t header;
	while (1) {
			t_msg* msg = msg_recibir(socketCPU);
			if(msg->longitud > 0)
				msg_recibir_data(socketCPU, msg);

			header = msg->tipoMensaje;

			switch (header) {

			case FIN_CPU: {
				//MATO ES HILO
				log_info(log_memoria,"La CPU %d ha finalizado", socketCPU);
				pthread_exit(NULL);
			}
			break;




			case LECTURA_PAGINA: {
				pthread_mutex_lock(&mutexCantAccesosMemoria);
				cantAccesosMemoria++;
				pthread_mutex_unlock(&mutexCantAccesosMemoria);

				offset = 0;

				// desserializacion
				memcpy(&pidPeticion, msg->data + offset, tmpsize = sizeof(t_pid));
				offset += tmpsize;
				memcpy(&puntero.pagina, msg->data + offset, tmpsize = sizeof(t_num));
				offset += tmpsize;
				memcpy(&puntero.offset, msg->data + offset, tmpsize = sizeof(t_num));
				offset += tmpsize;
				memcpy(&puntero.size, msg->data + offset, tmpsize = sizeof(t_num));
				offset += tmpsize;

				log_info(log_memoria, " --- --- Solicitud de lectura. PID: %d Pag:%d Offset:%d Size:%d", pidPeticion, puntero.pagina, puntero.offset, puntero.size);


				if (paginaInvalida(pidPeticion, puntero.pagina)) {

					log_error(log_memoria, "pagina %d invalida proceso %d", puntero.pagina, pidPeticion);

					//AVISO FINALIZACION PROGRAMA A LA CPU
					msg_enviar_separado(PAGINA_INVALIDA, 0, 0, socketCPU);

				} else if (Cache_Activada()) {

					char* contenido_leido = malloc(puntero.size);
					void* contenidoTmp;

					if(!estaEnCache(pidPeticion, puntero.pagina))
						agregarEntradaCache(pidPeticion, puntero.pagina);

					int sizeRestante = puntero.size, offsetEscritura = 0;
					while(sizeRestante + puntero.offset > tamanioDeMarcos){
						contenidoTmp = obtenerContenidoSegunCache(pidPeticion, puntero);
						memcpy(contenido_leido + offsetEscritura, contenidoTmp, tamanioDeMarcos);

						sizeRestante = sizeRestante - (tamanioDeMarcos - puntero.offset);
						offsetEscritura = puntero.size - sizeRestante;
						puntero.pagina++;
						puntero.offset = 0;
						free(contenidoTmp);
						if(!estaEnCache(pidPeticion, puntero.pagina))
							agregarEntradaCache(pidPeticion, puntero.pagina);
					}

					contenidoTmp = obtenerContenidoSegunCache(pidPeticion, puntero);
					memcpy(contenido_leido + offsetEscritura, contenidoTmp, sizeRestante);
					free(contenidoTmp);
					msg_enviar_separado(LECTURA_PAGINA, puntero.size, contenido_leido, socketCPU);
					if(puntero.size != sizeof(t_num)){
						contenido_leido[puntero.size-1] = '\0';
						log_info(log_memoria, "contenido_leido %s", contenido_leido);
					}else
						log_info(log_memoria, "contenido_leido %d", contenido_leido);

					free(contenido_leido);

				} else {

					//-----Retardo
					pthread_mutex_lock(&mutexRetardo);
					usleep(retardoMemoria * 1000);
					pthread_mutex_unlock(&mutexRetardo);
					//------------

					char* contenido_leido = obtenerContenido(pidPeticion, puntero);

					if(contenido_leido == NULL)
						msg_enviar_separado(EXCEPCION_MEMORIA, 0, 0, socketCPU);
					else{
						msg_enviar_separado(LECTURA_PAGINA, puntero.size, contenido_leido, socketCPU);
						if(puntero.size != sizeof(t_num))
							contenido_leido[puntero.size-1] = '\0';
						log_info(log_memoria, "contenido_leido %s", contenido_leido);
						free(contenido_leido);
					}

				}

			}
			break;




			case ESCRITURA_PAGINA:

				pthread_mutex_lock(&mutexCantAccesosMemoria);
				cantAccesosMemoria++;
				pthread_mutex_unlock(&mutexCantAccesosMemoria);

				offset = 0;

				// desserializacion
				memcpy(&pidPeticion, msg->data + offset, tmpsize = sizeof(t_pid));
				offset += tmpsize;
				memcpy(&puntero, msg->data + offset, tmpsize = sizeof(t_posicion));
				offset += tmpsize;
				void* contenido_escribir = malloc(puntero.size);
				memcpy(contenido_escribir, msg->data + offset, tmpsize = puntero.size);
				offset += tmpsize;

				log_info(log_memoria, " --- --- Solicitud de escritura. PID:%d Pag:%d Offset:%d Size:%d", pidPeticion, puntero.pagina, puntero.offset, puntero.size);

				if (puntero.size == sizeof(t_valor_variable)) {
					int contenido;
					memcpy(&contenido, contenido_escribir, sizeof(t_valor_variable));
					log_info(log_memoria, "Contenido: %d", contenido);
				}

				if (paginaInvalida(pidPeticion, puntero.pagina)) {

					log_error(log_memoria, "pagina %d invalida proceso %d", puntero.pagina, pidPeticion);
					free(contenido_escribir);

					//AVISO FINALIZACION PROGRAMA A LA CPU
					header = PAGINA_INVALIDA;

					bytesEnviados = send(socketCPU, &header, sizeof(uint32_t), 0);
					if (bytesEnviados <= 0) {
						log_error(log_memoria, "Error send CPU");
						pthread_exit(NULL);
					}

				} else if (Cache_Activada()) {

					void* contenidoTmp;

					if(!estaEnCache(pidPeticion, puntero.pagina))
						agregarEntradaCache(pidPeticion, puntero.pagina);

					int sizeRestante = puntero.size, offsetEscritura = 0;
					while(sizeRestante + puntero.offset > tamanioDeMarcos){

						contenidoTmp = malloc(tamanioDeMarcos);
						memcpy(contenidoTmp, contenido_escribir + offsetEscritura, tamanioDeMarcos);
						escribirContenidoSegunCache(pidPeticion, puntero, contenidoTmp);
						free(contenidoTmp);

						sizeRestante = sizeRestante - (tamanioDeMarcos - puntero.offset);
						offsetEscritura = puntero.size - sizeRestante;
						puntero.pagina++;
						puntero.offset = 0;
						if(!estaEnCache(pidPeticion, puntero.pagina))
							agregarEntradaCache(pidPeticion, puntero.pagina);
					}

					contenidoTmp = malloc(sizeRestante);
					memcpy(contenidoTmp, contenido_escribir + offsetEscritura, sizeRestante);
					escribirContenidoSegunCache(pidPeticion, puntero, contenidoTmp);
					free(contenidoTmp);

					//ENVIO ESCRITURA OK
					header = ESCRITURA_PAGINA;

					bytesEnviados = send(socketCPU, &header, sizeof(uint32_t), 0);
					if (bytesEnviados <= 0) {
						log_error(log_memoria, "Error send CPU");
						pthread_exit(NULL);
					}

				} else {

					//-----Retardo
					pthread_mutex_lock(&mutexRetardo);
					usleep(retardoMemoria * 1000);
					pthread_mutex_unlock(&mutexRetardo);
					//------------

					if( escribirContenido(pidPeticion, puntero, contenido_escribir) == -1){
						header = EXCEPCION_MEMORIA;
						bytesEnviados = send(socketCPU, &header, sizeof(uint32_t), 0);
					}else{
						header = ESCRITURA_PAGINA; //OK
						bytesEnviados = send(socketCPU, &header, sizeof(uint32_t), 0);

						log_info(log_memoria, "Escribi correctamente");
					}
					if (bytesEnviados <= 0) {
						log_error(log_memoria, "Error send CPU");
						pthread_exit(NULL);
					}
					free(contenido_escribir);
				}

			break;





			case 0:
				log_trace(log_memoria, "Se desconecto cpu %d", socketCPU);
				pthread_exit(NULL);
				break;

			default:
				log_error(log_memoria,"Error");
			}
			msg_destruir(msg);

	}
}

/*			LOCKS MUTEX			*/

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

/*			LISTAPROCESOS			*/

void crearProcesoYAgregarAListaDeProcesos(t_pid pid) {

	t_proceso* procesoNuevo = malloc(sizeof(t_proceso));
	procesoNuevo->PID = pid;
	procesoNuevo->cantFramesAsignados = 0;
	procesoNuevo->cantFramesUsados = 0;

	list_add(listaProcesos, procesoNuevo);

}

void eliminarProcesoDeListaDeProcesos(t_pid pid) {

	int _soy_el_proceso_buscado(t_proceso* proceso) {
		return proceso->PID == pid;
	}

	lockProcesos();
	list_remove_and_destroy_by_condition(listaProcesos, (void*) _soy_el_proceso_buscado, free);
	unlockProcesos();
}

/*			LECTURA-ESCRITURA			*/

void* obtenerContenido(t_pid pid, t_posicion puntero) {

	int nroFrame = buscarFramePorPidPag(pid, puntero.pagina);

	int desplazamientoMemoria = (nroFrame * tamanioDeMarcos) + puntero.offset;

	if(nroFrame == -1){
		log_error(log_memoria, "No se encuentra el frame para pid %d pagina %d", pid, puntero.pagina);
		return NULL;
	}

	void* contenido = malloc(puntero.size);
	int sizeRestante = puntero.size;
	int i = 0, offsetLectura = 0;
	while(puntero.offset + sizeRestante > tamanioDeMarcos){

		log_info(log_memoria, "		READ %d bytes -> (frame, offset) %d, %d", (tamanioDeMarcos - puntero.offset), nroFrame, puntero.offset);

		pthread_mutex_lock(&mutexMemoriaReal);
		memcpy(contenido + offsetLectura, memoria_real + desplazamientoMemoria, (tamanioDeMarcos - puntero.offset));
		pthread_mutex_unlock(&mutexMemoriaReal);

		i++;
		sizeRestante = sizeRestante - (tamanioDeMarcos - puntero.offset);
		offsetLectura = puntero.size - sizeRestante;
		nroFrame = buscarFramePorPidPag(pid, puntero.pagina + i);
		desplazamientoMemoria = (nroFrame * tamanioDeMarcos);
		puntero.offset = 0;
	}

	log_info(log_memoria, "		READ %d bytes -> (frame, offset) %d, %d", sizeRestante, nroFrame, puntero.offset);

	pthread_mutex_lock(&mutexMemoriaReal);
	memcpy(contenido + offsetLectura, memoria_real + desplazamientoMemoria, sizeRestante);
	pthread_mutex_unlock(&mutexMemoriaReal);

	return contenido;
}

int escribirContenido(t_pid pid, t_posicion puntero, void* contenido) {

	int nroFrame = buscarFramePorPidPag(pid, puntero.pagina);
	int desplazamientoMemoria = (nroFrame * tamanioDeMarcos) + puntero.offset;

	if(nroFrame == -1){
		log_error(log_memoria, "No se encuentra el frame para pid %d pagina %d", pid, puntero.pagina);
		return -1;
	}

	int sizeRestante = puntero.size;
	int i = 0, offsetEscritura = 0;
	while(puntero.offset + sizeRestante > tamanioDeMarcos){

		log_info(log_memoria, "		kWRITE %d bytes -> (frame, offset) (%d, %d)", (tamanioDeMarcos - puntero.offset), nroFrame, puntero.offset);

		pthread_mutex_lock(&mutexMemoriaReal);
		memcpy(memoria_real + desplazamientoMemoria, contenido + offsetEscritura, (tamanioDeMarcos - puntero.offset));
		pthread_mutex_unlock(&mutexMemoriaReal);

		i++;
		sizeRestante = sizeRestante - (tamanioDeMarcos - puntero.offset);
		offsetEscritura = puntero.size - sizeRestante;
		nroFrame = buscarFramePorPidPag(pid, puntero.pagina + i);
		desplazamientoMemoria = (nroFrame * tamanioDeMarcos);
		puntero.offset = 0;
	}

	log_info(log_memoria, "		WRITE %d bytes -> (frame, offset) (%d, %d)", sizeRestante, nroFrame, puntero.offset);

	pthread_mutex_lock(&mutexMemoriaReal);
	memcpy(memoria_real + desplazamientoMemoria, contenido + offsetEscritura, sizeRestante);
	pthread_mutex_unlock(&mutexMemoriaReal);

	return 0;
}


/*			FRAMES			*/

void inicializarFrames(){
	t_frame* frameNuevo;
	int cantBytesEA = cantidadDeMarcos * sizeof(t_frame);
	int aux;
	cantidadFramesEstructurasAdministrativas = 1;

	for(aux = cantBytesEA; aux > tamanioDeMarcos; aux = aux - tamanioDeMarcos)
		cantidadFramesEstructurasAdministrativas++;

	log_trace(log_memoria, "El tamanio necesario para las estructuras administrativas es %d bytes, %d marcos",
			cantBytesEA, cantidadFramesEstructurasAdministrativas);

	int i=0;
	for(; i < cantidadDeMarcos ; i++){
		frameNuevo = malloc(sizeof(t_frame));
		frameNuevo->nroFrame = i;
		frameNuevo->nroPag = 0;

		if(i < cantidadFramesEstructurasAdministrativas)
			frameNuevo->pid = PID_RESERVADO;			//frame reservado para EA
		else
			frameNuevo->pid = 0;


		// i*sizeof(t_frame) es el offset
		memcpy(memoria_real + i*sizeof(t_frame), frameNuevo, sizeof(t_frame));
		//log_info(log_memoria, "Cree frame %d", frameNuevo->nroFrame);
		free(frameNuevo);
	}

	log_trace(log_memoria, "Memoria reservada y frames inicializados");
}

int cantFramesLibres() {
	int cantidadLibres = 0;
	t_frame* frame;

	lockFrames();
	int i=0;
	for(; i < cantidadDeMarcos ; i++){
		frame = malloc(sizeof(t_frame));
		// i*sizeof(t_frame) es el offset
		memcpy(frame, memoria_real + i*sizeof(t_frame), sizeof(t_frame));

		if(frame->pid == 0)
			cantidadLibres++;
		free(frame);
	}
	unlockFrames();

	log_trace(log_memoria, "Cantidad libres %d", cantidadLibres);
	return cantidadLibres;
}

int buscarFrameLibre(t_pid pid) {

	int _soy_el_pid_buscado(t_proceso* proceso) {
		return (proceso->PID == pid);
	}

	lockProcesos();
	t_proceso* proceso = list_remove_by_condition(listaProcesos, (void*) _soy_el_pid_buscado);

	int nroPagNueva = proceso->cantFramesUsados;

	log_trace(log_memoria, "nroPagNueva %d", nroPagNueva);
	int indice = funcionHashing(pid, nroPagNueva);
	t_frame* frame;

	lockFrames();
	int nroFrame = -1, i;
	bool flag_sigo = 1;
	for(i = indice; i < cantidadDeMarcos && flag_sigo; i++){
		frame = malloc(sizeof(t_frame));
		// i*sizeof(t_frame) es el offset
		memcpy(frame, memoria_real + i*sizeof(t_frame), sizeof(t_frame));

		if(frame->pid == 0){
			log_trace(log_memoria, "Asigno a frame %d pid %d pag %d", frame->nroFrame, pid, nroPagNueva);
			frame->pid = pid;
			frame->nroPag = nroPagNueva;
			memcpy(memoria_real + i*sizeof(t_frame), frame, sizeof(t_frame));
			proceso->cantFramesAsignados++;
			proceso->cantFramesUsados++;
			nroFrame = i;
			flag_sigo = 0;
		}
		free(frame);
	}
	unlockFrames();
	list_add(listaProcesos, proceso);
	unlockProcesos();

	return nroFrame;
}


/*			FRAMES POR PAGINA DE PROCESO			*/

int funcionHashing(t_pid pid, t_num16 nroPagina){

	return (pid*pid + nroPagina*nroPagina) % cantidadDeMarcos;

}

int paginaInvalida(t_pid pid, t_num16 nroPagina) {

	int _soy_el_pid_buscado(t_proceso* proceso) {
		return (proceso->PID == pid);
	}
	lockProcesos();
	t_proceso* proceso = list_find(listaProcesos, (void*) _soy_el_pid_buscado);

	int esPagInvalida = (nroPagina >= proceso->cantFramesUsados);
	unlockProcesos();

	return esPagInvalida;
}

int buscarFramePorPidPag(t_pid pid, t_num16 nroPagina){

	log_trace(log_memoria, "Busco frame de pid %d pag %d", pid, nroPagina);

	int _soy_el_pid_buscado(t_proceso* proceso) {
		return (proceso->PID == pid);
	}

	int indice = funcionHashing(pid, nroPagina);
	t_frame* frame;

	lockFrames();
	int nroFrame = -1, i;
	bool flag_sigo = 1;

	for(i = indice; i < cantidadDeMarcos && flag_sigo; i++){
		frame = malloc(sizeof(t_frame));
		// i*sizeof(t_frame) es el offset
		memcpy(frame, memoria_real + i*sizeof(t_frame), sizeof(t_frame));

		if(frame->pid == pid && frame->nroPag == nroPagina){
			nroFrame = i;
			flag_sigo = 0;
			log_trace(log_memoria, "Encontre frame %d", nroFrame);
		}
		free(frame);
	}
	unlockFrames();

	return nroFrame;
}

void liberarPagina(t_pid pid, t_num16 nroPagina){

	int _soy_el_pid_buscado(t_proceso* proceso) {
		return (proceso->PID == pid);
	}

	lockProcesos();
	t_proceso* proceso = list_remove_by_condition(listaProcesos, (void*) _soy_el_pid_buscado);

	int indice = funcionHashing(pid, nroPagina);
	t_frame* frame;

	lockFrames();
	int i;
	bool flag_sigo = 1;
	for(i = indice; i < cantidadDeMarcos && flag_sigo; i++){
		frame = malloc(sizeof(t_frame));
		// i*sizeof(t_frame) es el offset
		memcpy(frame, memoria_real + i*sizeof(t_frame), sizeof(t_frame));

		if(frame->pid == pid && frame->nroPag == nroPagina){
			frame->pid = 0;
			frame->nroPag = 0;
			flag_sigo = 0;
			proceso->cantFramesAsignados--;
			memcpy(memoria_real + i*sizeof(t_frame), frame, sizeof(t_frame));
			log_info(log_memoria,"Frame %d liberado", frame->nroFrame);
		}
		free(frame);
	}
	unlockFrames();
	list_add(listaProcesos, proceso);
	unlockProcesos();

}

void liberarFramesDeProceso(t_pid pid){

	int _soy_el_pid_buscado(t_proceso* proceso) {
		return (proceso->PID == pid);
	}
	lockProcesos();
	t_proceso* proceso = list_find(listaProcesos, (void*) _soy_el_pid_buscado);
	int cantPagsALiberar = proceso->cantFramesUsados, i;
	unlockProcesos();

	for(i = 0; i < cantPagsALiberar; i++){
		liberarPagina(pid, i);
	}

}





/*			MISC			*/
void terminarMemoria(){			//aca libero todos

	list_destroy_and_destroy_elements(lista_cpus, free);

	list_destroy_and_destroy_elements(listaProcesos, free);

	void _liberarCache(t_cache* entradaCache) {
		free(entradaCache->contenido);
		free(entradaCache);
	}

	list_destroy_and_destroy_elements(Cache, (void*) _liberarCache);

	free(memoria_real);

	log_trace(log_memoria, "Termino Proceso");
	log_destroy(log_memoria);
	exit(1);
}










/*					CACHE 					*/
t_list* crearCache() {
	t_list* cache = list_create();
	return cache;
}

void eliminarCache() {
	int pidAux;
	int _soy_el_pid_buscado(t_proceso* proceso) {
		return (proceso->PID == pidAux);
	}
	void _liberarCache(t_cache* entradaCache) {
		pidAux = entradaCache->pid;
		t_posicion p = { 0, entradaCache->numPag, tamanioDeMarcos};

		escribirContenido(entradaCache->pid, p, entradaCache->contenido);
		free(entradaCache->contenido);
		free(entradaCache);

		t_proceso* proceso = list_remove_by_condition(listaProcesos, (void*) _soy_el_pid_buscado);
		if(proceso == NULL)
			log_warning(log_memoria, "No encontre proceso %d", pidAux);
		else{
			proceso->cantEntradasCache--;
			list_add(listaProcesos, proceso);
		}
	}

	list_destroy_and_destroy_elements(Cache, (void*) _liberarCache);
}


void liberarCacheDeProceso(t_pid pid){

	int i, cantPagsALiberar = 0;
	int _pid_buscado(t_proceso* p) {
		return (p->PID == pid);
	}
	int _esEntrada(t_cache* cache) {
		return (cache->pid == pid) && (cache->numPag == i);
	}

	lockProcesos();
	t_proceso* proceso = list_remove_by_condition(listaProcesos, (void*) _pid_buscado);
	if(proceso == NULL)
		log_warning(log_memoria, "No encontre proceso %d", pid);
	else
		cantPagsALiberar = proceso->cantFramesUsados;

	void _liberarCache(t_cache* entradaCache) {
		t_posicion p = { 0, entradaCache->numPag, tamanioDeMarcos};

		escribirContenido(entradaCache->pid, p, entradaCache->contenido);

		free(entradaCache->contenido);
		free(entradaCache);

		if(proceso != NULL)
			proceso->cantEntradasCache--;
	}

	for(i = 0; i < cantPagsALiberar; i++){
		if(estaEnCache(pid, i)){
			list_remove_and_destroy_by_condition(Cache, (void*) _esEntrada, (void*) _liberarCache);
			pthread_mutex_unlock(&mutexCache);
		}
	}
	if(proceso != NULL)
		list_add(listaProcesos, proceso);
	unlockProcesos();

}

void vaciarCache() {

	if (Cache_Activada()) {
		pthread_mutex_lock(&mutexCache);
		eliminarCache();
		Cache = crearCache();
		pthread_mutex_unlock(&mutexCache);

		printf(PRINT_COLOR_BLUE "flush Cache realizado" PRINT_COLOR_RESET "\n");
		log_info(log_memoria, "Flush Cache realizado");

	} else {
		printf(PRINT_COLOR_YELLOW "Cache no activada" PRINT_COLOR_RESET "\n");
		log_error(log_memoria, "Se intento hacer flush cache pero no esta activada");
	}

}

t_cache* buscarEntradaCache(t_pid pid, t_num16 numero_pagina) {
	int _soy_la_entrada_cache_buscada(t_cache* entradaCache) {
		return ((entradaCache->pid == pid) && (entradaCache->numPag == numero_pagina));
	}

	return list_find(Cache, (void*) _soy_la_entrada_cache_buscada);

}

int estaEnCache(t_pid pid, t_num16 numero_pagina) {

	pthread_mutex_lock(&mutexCache);
	t_cache* entradaCache = buscarEntradaCache(pid, numero_pagina);

	if (entradaCache != NULL) {
		log_info(log_memoria, "La pagina %d esta en la Cache", numero_pagina);
		return 1;
	} else {
		log_info(log_memoria, "Cache Miss");
		pthread_mutex_unlock(&mutexCache);
		return 0;
	}
}

t_cache* crearRegistroCache(t_pid pid, t_num16 numPag) {

	t_cache* unaEntradaCache = malloc(sizeof(t_cache));
	unaEntradaCache->pid = pid;
	unaEntradaCache->numPag = numPag;
	unaEntradaCache->contenido = malloc(tamanioDeMarcos);
	bzero(unaEntradaCache->contenido, tamanioDeMarcos);

	pthread_mutex_lock(&mutexCantAccesosMemoria);
	unaEntradaCache->ultimoAcceso = cantAccesosMemoria;
	pthread_mutex_unlock(&mutexCantAccesosMemoria);

	return unaEntradaCache;
}

int hayEspacioEnCache() {

	int tamanioCache = list_size(Cache);

	return tamanioCache < cantidadEntradasCache;
}

void agregarEntradaCache(t_pid pid, t_num16 numero_pagina) {

	int _soy_el_pid_buscado(t_proceso* proceso) {
		return (proceso->PID == pid);
	}

	pthread_mutex_lock(&mutexCache);
	lockProcesos();
	t_proceso* proceso = list_remove_by_condition(listaProcesos, (void*) _soy_el_pid_buscado);
	t_cache* nuevaEntradaCache = crearRegistroCache(pid, numero_pagina);

	t_posicion p;
	p.offset = 0;
	p.size = tamanioDeMarcos;
	p.pagina = numero_pagina;
	void* bufaux = obtenerContenido(pid, p);
	memcpy(nuevaEntradaCache->contenido, bufaux, tamanioDeMarcos);
	free(bufaux);

	if(proceso->cantEntradasCache == cachePorProceso)
		algoritmoLRU(pid);

	else if(!hayEspacioEnCache())
		algoritmoLRU(0);

	list_add(Cache, nuevaEntradaCache);
	proceso->cantEntradasCache++;
	log_info(log_memoria, "Pagina %d del proceso %d agregada a Cache", numero_pagina, pid);

	list_add(listaProcesos, proceso);
	unlockProcesos();
	pthread_mutex_unlock(&mutexCache);
}

void* obtenerContenidoSegunCache(t_pid pid, t_posicion puntero){

	t_cache* entradaCache = buscarEntradaCache(pid, puntero.pagina);

	pthread_mutex_lock(&mutexCantAccesosMemoria);
	entradaCache->ultimoAcceso = cantAccesosMemoria;
	pthread_mutex_unlock(&mutexCantAccesosMemoria);

	log_info(log_memoria, "		CACHE READ %d bytes offset %d", puntero.size, puntero.offset);

	void* contenido = malloc(puntero.size);
	memcpy(contenido, entradaCache->contenido + puntero.offset, puntero.size);

	pthread_mutex_unlock(&mutexCache);

	return contenido;
}

void escribirContenidoSegunCache(t_pid pid, t_posicion puntero, void* contenido_escribir) {

	t_cache* entradaCache = buscarEntradaCache(pid, puntero.pagina);

	pthread_mutex_lock(&mutexCantAccesosMemoria);
	entradaCache->ultimoAcceso = cantAccesosMemoria;
	pthread_mutex_unlock(&mutexCantAccesosMemoria);

	log_info(log_memoria, "		CACHE WRITE %d bytes offset %d", puntero.size, puntero.offset);

	memcpy(entradaCache->contenido + puntero.offset, contenido_escribir, puntero.size);

	pthread_mutex_unlock(&mutexCache);

}


int Cache_Activada() {

	return (cantidadEntradasCache > 0) && (cachePorProceso > 0);

}

void algoritmoLRU(t_pid pid) {

	int _menor_ultimo_acceso(t_cache* unRegistroCache, t_cache* otroRegistroCache) {
		return (unRegistroCache->ultimoAcceso < otroRegistroCache->ultimoAcceso);
	}
	int _soy_el_pid_buscado(t_proceso* proceso) {
		return (proceso->PID == pid);
	}

	int _pid_buscado(t_cache* entrada) {
		if(pid == 0)
			//	pid == 0 reemplazo global
			return 1;
		else
			//  pid != 0 reemplazo local
			return entrada->pid == pid;
	}

	void _liberarCache(t_cache* entradaCache) {
		t_posicion p;
		p.offset = 0;
		p.pagina = entradaCache->numPag;
		p.size = tamanioDeMarcos;

		escribirContenido(entradaCache->pid, p, entradaCache->contenido);
		free(entradaCache->contenido);
		free(entradaCache);

		t_proceso* proceso = list_remove_by_condition(listaProcesos, (void*) _soy_el_pid_buscado);
		if(proceso == NULL)
			log_warning(log_memoria, "No encontre proceso %d", pid);
		else{
			proceso->cantEntradasCache--;
			list_add(listaProcesos, proceso);
		}
	}

	log_info(log_memoria, "Algoritmo LRU");

	list_sort(Cache, (void*) _menor_ultimo_acceso);

	t_cache* entradaCachereemplazada = list_remove_by_condition(Cache, (void*) _pid_buscado);
	log_info(log_memoria, "En Cache se reemplazo la pagina %d del proceso %d", entradaCachereemplazada->numPag, entradaCachereemplazada->pid);

	_liberarCache(entradaCachereemplazada);

}





/*					COMANDOS					*/



void ejecutarComandos() {

	char* comando = malloc(200);

	while (1) {
		fgets(comando, 200, stdin);
		comando[strlen(comando)-1] = '\0';
		log_error(log_memoria, "Comando: %s", comando);

		if (string_starts_with(comando,"retardo ")) {

				int retardoNuevo = atoi(string_substring_from(comando, 8));

				pthread_mutex_lock(&mutexRetardo);
				retardoMemoria = retardoNuevo;
				pthread_mutex_unlock(&mutexRetardo);

				fprintf(stderr, PRINT_COLOR_BLUE "Nuevo retardo: %d ms" PRINT_COLOR_RESET "\n", retardoNuevo);
				log_info(log_memoria, "Retardo modificado correctamente a %d ms",retardoMemoria);

		} else if (string_equals_ignore_case(comando, "dump")) {

				printf("Elegir opcion: \n"
						PRINT_COLOR_BLUE "  a -" PRINT_COLOR_RESET " Dump completo de la memoria Caché \n"
						PRINT_COLOR_BLUE "  b -" PRINT_COLOR_RESET " Tabla de Páginas y Listado de procesos Activos \n"
						PRINT_COLOR_BLUE "  c -" PRINT_COLOR_RESET " Datos almacenados en la memoria de todos los procesos [0] o de un proceso en particular [pid] \n");

				switch(getchar()){
				case 'a':
					lockFramesYProcesos();
						dumpCache();
					unlockFramesYProcesos();
					break;

				case 'b':
					lockFramesYProcesos();
						dumpTablaPaginas();
					unlockFramesYProcesos();
					break;

				case 'c':
					lockProcesos();

					int pid;
					printf("Ingrese pid: ");
					scanf("%d", &pid);
					if (pid == 0)
						dumpTodosLosProcesos();
					else
						dumpProcesoParticular(pid);

					unlockProcesos();
					break;

				default:
					fprintf(stderr, PRINT_COLOR_YELLOW "Capo que me tiraste?" PRINT_COLOR_RESET "\n");
				}

		}else if (string_equals_ignore_case(comando, "flush")) {

				vaciarCache();

		} else if(string_equals_ignore_case(comando, "size memory")) {

				int cantLibres = cantFramesLibres();

				printf("Cantidad de frames en memoria: %d\n", cantidadDeMarcos);
				printf("Cantidad de frames usados: %d\n", cantidadDeMarcos - cantLibres);
				printf("Cantidad de frames libres: %d\n", cantLibres);

		}else if(string_starts_with(comando,"size pid")){

				int pid = atoi(string_substring_from(comando, 9));

				t_proceso* proceso = buscarProcesoEnListaProcesosParaDump(pid);

				if (proceso == NULL)
					printf(PRINT_COLOR_YELLOW "No existe ese proceso" PRINT_COLOR_RESET "\n");
				else
					printf("El tamaño es de %d frames\n",proceso->cantFramesAsignados);

		}else if(string_equals_ignore_case(comando, "help")){
				printf("Comandos:\n"
						PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "retardo [milisegundos]: " PRINT_COLOR_RESET "Modificar tiempo de espera ante un acceso a MP \n"
						PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "dump: " PRINT_COLOR_RESET "Reporte en pantalla y en disco de cache, tabla de paginas o contenido en memoria\n"
						PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "flush: " PRINT_COLOR_RESET "Limpiar completamente el contenido de la Caché\n"
						PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "size memory: " PRINT_COLOR_RESET "Tamaño de la memoria en cantidad de frames, frames ocupados y frames libres\n"
						PRINT_COLOR_BLUE "  ● " PRINT_COLOR_CYAN "size pid [pid]: " PRINT_COLOR_RESET "Tamaño total de un proceso\n");
		}else if(string_equals_ignore_case(comando, "\0")){
		}else
			fprintf(stderr, PRINT_COLOR_YELLOW "Comando incorrecto" PRINT_COLOR_RESET "\n");
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

		i++;
	}
}

void dumpTodosLosProcesos() {


	char* nombreArchivoDump = string_new();
	string_append_with_format(&nombreArchivoDump, "%d_dumpTotal.dump", process_getpid());
	FILE* archivoDump = fopen(nombreArchivoDump, "w+");

	//unsigned int tamanioTotalMemoria = tamanioDeMarcos * cantidadDeMarcos;

	dumpEstructurasMemoriaTodosLosProcesos(archivoDump);

	fprintf(archivoDump,
			"\n//////////////////          MEMORIA PRINCIPAL           //////////////////\n");

	printf("\n"
			PRINT_COLOR_CYAN "//////////////////" PRINT_COLOR_RESET
			"          MEMORIA PRINCIPAL           "
			PRINT_COLOR_CYAN "//////////////////" PRINT_COLOR_RESET "\n\n");

	pthread_mutex_lock(&mutexMemoriaReal);
	hexdump(memoria_real, tamanioDeMarcos * tamanioDeMarcos, archivoDump);
	hexdump(memoria_real, tamanioDeMarcos * tamanioDeMarcos, stderr);
	pthread_mutex_unlock(&mutexMemoriaReal);

	printf("\nDUMP DE MEMORIA PRINCIPAL PARA TODOS LOS PROCESOS REALIZADO\n");

	fclose(archivoDump);
}

void dumpContenidoMemoriaProceso(t_pid pid, FILE* archivoDump) {

	int nroPag = 0;
	void* contenidoFrame;
	t_proceso* proceso = buscarProcesoEnListaProcesosParaDump(pid);
	t_posicion p;
	p.offset = 0;
	p.size = tamanioDeMarcos;

	fprintf(archivoDump,
			"\n//////////////////          MEMORIA PRINCIPAL           //////////////////\n");

	printf("\n"
			PRINT_COLOR_CYAN "//////////////////" PRINT_COLOR_RESET
			"          MEMORIA PRINCIPAL           " PRINT_COLOR_CYAN
			"//////////////////" PRINT_COLOR_RESET "\n\n");

	while (nroPag < proceso->cantFramesUsados) {
		log_trace(log_memoria, "nroPag %d proceso->cantFramesUsados %d", nroPag, proceso->cantFramesUsados);
		p.pagina = nroPag;
		contenidoFrame = obtenerContenido(pid, p);
		if(contenidoFrame != NULL){
			log_trace(log_memoria, "hexdump");
			hexdump(contenidoFrame, tamanioDeMarcos, archivoDump);
			hexdump(contenidoFrame, tamanioDeMarcos, stderr);
		}

		nroPag++;
	}

	printf("\nDUMP DE MEMORIA PRINCIPAL PARA EL PROCESO %d REALIZADO\n", proceso->PID);
}

void dumpProcesoParticular(t_pid pid) {

	t_proceso* proceso = buscarProcesoEnListaProcesosParaDump(pid);

	if (proceso == NULL) {
		printf(PRINT_COLOR_YELLOW "No existe ese proceso" PRINT_COLOR_RESET "\n");
	} else {


		char* nombreArchivoDump = string_new();
		string_append_with_format(&nombreArchivoDump, "%d_dumpProceso_%d.dump", process_getpid(), pid);
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

		dumpContenidoMemoriaProceso(pid, archivoDump);

		fclose(archivoDump);
	}


}


void dumpCache(){

	if(!Cache_Activada()){
		fprintf(stderr, PRINT_COLOR_YELLOW "La cache no esta activada" PRINT_COLOR_RESET);
		return;
	}

	if(list_size(Cache) == 0){
		fprintf(stderr, PRINT_COLOR_CYAN "La cache esta vacia \n" PRINT_COLOR_RESET);
		return;
	}

	char* nombreArchivoDump = string_new();
	string_append_with_format(&nombreArchivoDump, "%d_dumpCache.dump", process_getpid());
	FILE* archivoDump = fopen(nombreArchivoDump, "w+");
	int i = 0;

	void _dump(t_cache* entradaCache){

		fprintf(archivoDump, "\n//////////////////////       ENTRADA CACHE %2d        /////////////////////\n\n", i);
		fprintf(archivoDump,
				" PID %d \n nº Pagina: %d \n",
				entradaCache->pid, entradaCache->numPag);

		fprintf(stderr, PRINT_COLOR_CYAN "\n//////////////////////" PRINT_COLOR_RESET
				"       ENTRADA CACHE %2d        "
				PRINT_COLOR_CYAN "/////////////////////\n\n" PRINT_COLOR_RESET, i);

		fprintf(stderr,
				" PID %d \n nº Pagina: %d \n",
				entradaCache->pid, entradaCache->numPag);


		hexdump(entradaCache->contenido, tamanioDeMarcos, archivoDump);
		hexdump(entradaCache->contenido, tamanioDeMarcos, stderr);

		i++;
	}
	list_iterate(Cache, (void*) _dump);
	fclose(archivoDump);
}




void dumpTablaPaginas(){

	char* nombreArchivoDump = string_new();
	string_append_with_format(&nombreArchivoDump, "%d_TablaPaginas.dump", process_getpid());
	FILE* archivoDump = fopen(nombreArchivoDump, "w+");

	fprintf(stderr, PRINT_COLOR_CYAN "   ····  Frame  ····   PID   ····  Pagina ···· " PRINT_COLOR_RESET "\n");
	fprintf(archivoDump, "   ····  Frame  ····   PID   ····  Pagina ···· \n");

	char* nrof, *nrop, *procActivos = string_new();
	t_frame* frame;
	int nroFrame = 0;
	for(; nroFrame < cantidadDeMarcos; nroFrame++){

		frame = malloc(sizeof(t_frame));
		memcpy(frame, memoria_real + nroFrame*sizeof(t_frame), sizeof(t_frame));

		if(frame->pid == PID_RESERVADO){
			nrof = " TP";
			nrop = " TP";
		}else if(frame->pid == 0){
			nrof = " - ";
			nrop = " - ";
		}else{
			nrof = string_itoa(frame->pid);
			nrop = string_itoa(frame->nroPag);
		}

		fprintf(stderr, "   ····   %3d   ····   %3s   ····   %3s   ···· \n",
			frame->nroFrame, nrof, nrop);

		fprintf(archivoDump, "  ····   %3d   ····   %3s   ····   %3s   ···· \n",
			frame->nroFrame, nrof, nrop);

		free(frame);
	}

	string_append(&procActivos, "[ ");
	void _listarProcs(t_proceso* p){
		string_append_with_format(&procActivos, " %d,", p->PID);
	}
	list_iterate(listaProcesos, (void*) _listarProcs);

	if(procActivos[string_length(procActivos)-1] == ',')
		procActivos[string_length(procActivos)-1] = ' ';
	string_append(&procActivos, " ]");
	if(string_length(procActivos) == 4)
		procActivos = "No hay procesos activos";
	fprintf(stderr, PRINT_COLOR_CYAN "\nProcesos activos: " PRINT_COLOR_RESET "%s\n", procActivos);
	fprintf(archivoDump, "\nProcesos activos: %s\n", procActivos);

	fclose(archivoDump);
}


t_proceso* buscarProcesoEnListaProcesosParaDump(t_pid pid) {

	int _soy_el_pid_buscado(t_proceso* proceso) {
		return (proceso->PID == pid);
	}

	return list_find(listaProcesos, (void*) _soy_el_pid_buscado);
}


void mostrarContenidoTodosLosProcesos(){
	void mostrarContenido(t_proceso* p){
		mostrarContenidoDeUnProceso(p->PID);
	}
	list_iterate(listaProcesos, (void*) mostrarContenido);
}

void mostrarContenidoDeUnProceso(t_pid pid){

	int _soy_el_pid_buscado(t_proceso* proceso) {
		return (proceso->PID == pid);
	}
	lockProcesos();
	t_proceso* proceso = list_find(listaProcesos, (void*) _soy_el_pid_buscado);
	int cantPagsAMostrar = proceso->cantFramesUsados;
	unlockProcesos();

	t_posicion p;
	p.offset = 0;
	p.size = tamanioDeMarcos;

	int nroPag;
	for(nroPag = 0; nroPag <= cantPagsAMostrar; nroPag++){

		p.pagina = nroPag;
		char* contenido_leido = obtenerContenido(pid, p);
		if(contenido_leido != NULL){
			printf("Numero de Pagina: %d \n\n", nroPag);
			printf("Contenido: %s",contenido_leido);
		}
	}

}





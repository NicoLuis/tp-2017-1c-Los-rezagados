/*
 * libreriaMemoria.c
 *
 *  Created on: 3/4/2017
 *      Author: utnso
 */

#include "libreriaMemoria.h"

// todo consideraciones grals:
// - ver retardosMemoria, en q momento se tiene q usar?
// 		esta mal usado ahora, ej: hay veces q hace el usleep y entra a una funcion q vuelve a hacer el usleep
// - poner colores a consola
// - agregar un comando help

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
	printf("La cantidad de Frames reservados para estructuras Administrtivas es de: %d",cantidadFramesEstructurasAdministrativas);
	log_info(log_memoria,"La cantidad de Frames reservados para estructuras Administrtivas es de: %d",cantidadFramesEstructurasAdministrativas);

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
		void* tmpBuffer;

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

			log_info(log_memoria, "Solicitud de inicializar proceso %d con %d paginas", pid, cantidadDePaginas);

			crearProcesoYAgregarAListaDeProcesos(pid);

			//guardo codigo en memoria

			log_info(log_memoria, "\n%s", msg->data);

			if(hayFramesLibres(cantidadDePaginas)){
				int i = 0, nroFrame;
				for(; i < cantidadDePaginas; i++){
					tmpBuffer = malloc(tamanioDeMarcos);
					memcpy(tmpBuffer, msg->data + i*tamanioDeMarcos, tamanioDeMarcos);

					nroFrame = buscarFrameLibre(pid);
					escribirContenido(nroFrame, 0, tamanioDeMarcos, tmpBuffer);
					free(tmpBuffer);
				}
				log_info(log_memoria, "Asigne correctamente");
		 		header = OK;
				send(socketKernel, &header, sizeof(uint8_t), 0);
			}else{
				log_warning(log_memoria, "No hay frames libres");
				header = MARCOS_INSUFICIENTES;
				send(socketKernel, &header, sizeof(uint8_t), 0);
			}
			break;

		case LECTURA_PAGINA:

			pthread_mutex_lock(&mutexCantAccesosMemoria);
			cantAccesosMemoria++;
			pthread_mutex_unlock(&mutexCantAccesosMemoria);

			memcpy(&puntero, msg->data, sizeof(t_posicion));
			log_info(log_memoria, "Solicitud de lectura. Pag:%d Offset:%d Size:%d", puntero.pagina, puntero.offset, puntero.size);

			if (paginaInvalida(pid, puntero.pagina)) {

				log_error(log_memoria, "pagina %d invalida proceso %d", puntero.pagina, pid);
				msg_enviar_separado(PAGINA_INVALIDA, 0, 0, socketKernel);

			} else if ((Cache_Activada()) && (estaEnCache(pid, puntero.pagina))) {
				void* contenido_leido = obtenerContenidoSegunCache(pid, puntero.pagina, puntero.offset, puntero.size);
				msg_enviar_separado(LECTURA_PAGINA, puntero.size, contenido_leido, socketKernel);
				free(contenido_leido);

			} else {

				//-----Retardo
				pthread_mutex_lock(&mutexRetardo);
				usleep(retardoMemoria * 1000);
				pthread_mutex_unlock(&mutexRetardo);
				//------------

				int nroFrame = buscarFramePorPidPag(pid, puntero.pagina);
				char* contenido_leido = obtenerContenido(nroFrame, puntero.offset, puntero.size);
				msg_enviar_separado(LECTURA_PAGINA, puntero.size, contenido_leido, socketKernel);
				free(contenido_leido);

				if (Cache_Activada()) {
					agregarEntradaCache(pid, puntero.pagina, nroFrame);
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

			log_info(log_memoria, "Solicitud de escritura. Pag:%d Offset:%d Size:%d", puntero.pagina, puntero.offset, puntero.size);

			void* basura = malloc(metadata.size);
			bzero(basura, metadata.size);

			if (paginaInvalida(pid, puntero.pagina)) {

				log_error(log_memoria, "pagina %d invalida proceso %d", puntero.pagina, pid);
				msg_enviar_separado(PAGINA_INVALIDA, 0, 0, socketKernel);

			} else if ((Cache_Activada()) && (estaEnCache(pid, puntero.pagina))) {
				escribirContenidoSegunCache(pid, puntero.pagina, puntero.offset, sizeof(t_HeapMetadata), &metadata);
				escribirContenidoSegunCache(pid, puntero.pagina, puntero.offset + sizeof(t_HeapMetadata), metadata.size, basura);
				free(basura);

				if(msg->longitud == sizeof(t_posicion) + sizeof(t_HeapMetadata)*2)
					// significa q tengo q guardar otro t_HeapMetadata
					escribirContenidoSegunCache(pid, puntero.pagina, puntero.offset + sizeof(t_HeapMetadata) + metadata.size, sizeof(t_HeapMetadata), &metadata2);

				msg_enviar_separado(ESCRITURA_PAGINA, 0, 0, socketKernel);

			} else {

				//-----Retardo
				pthread_mutex_lock(&mutexRetardo);
				usleep(retardoMemoria * 1000);
				pthread_mutex_unlock(&mutexRetardo);
				//------------

				int nroFrame = buscarFramePorPidPag(pid, puntero.pagina);
				escribirContenido(nroFrame, puntero.offset, sizeof(t_HeapMetadata), &metadata);
				escribirContenido(nroFrame, puntero.offset + sizeof(t_HeapMetadata), metadata.size, basura);
				free(basura);

				if(msg->longitud == sizeof(t_posicion) + sizeof(t_HeapMetadata)*2)
					// significa q tengo q guardar otro t_HeapMetadata
					escribirContenido(nroFrame, puntero.offset + sizeof(t_HeapMetadata) + metadata.size, sizeof(t_HeapMetadata), &metadata2);

				msg_enviar_separado(ESCRITURA_PAGINA, 0, 0, socketKernel);

				if(Cache_Activada()){
					agregarEntradaCache(pid,puntero.pagina,nroFrame);
				}

				log_info(log_memoria, "Escribi correctamente");

			}

			break;

		case ASIGNACION_MEMORIA:

			log_error(log_memoria, "Solicitud nueva pagina pid %d", pid);

			pthread_mutex_lock(&mutexCantAccesosMemoria);
			cantAccesosMemoria++;
			pthread_mutex_unlock(&mutexCantAccesosMemoria);


			buscarFrameLibre(pid);


			msg_enviar_separado(ASIGNACION_MEMORIA, 0, 0, socketKernel);

			break;
		case LIBERAR:

			pthread_mutex_lock(&mutexCantAccesosMemoria);
			cantAccesosMemoria++;
			pthread_mutex_unlock(&mutexCantAccesosMemoria);

			memcpy(&puntero, msg->data, sizeof(t_posicion));
			log_info(log_memoria, "Solicitud liberar Pag:%d", puntero.pagina);

			liberarPagina(pid, puntero.pagina);

			msg_enviar_separado(LIBERAR, 0, 0, socketKernel);

			break;
		case FINALIZAR_PROGRAMA:
			log_info(log_memoria,"Finalizando proceso %d", pid);

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

				log_info(log_memoria,"Solicitud de lectura. Pag:%d Offset:%d Size:%d",puntero.pagina, puntero.offset, puntero.size);


				if (paginaInvalida(pidPeticion, puntero.pagina)) {

					log_error(log_memoria, "pagina %d invalida proceso %d", puntero.pagina, pidPeticion);

					//AVISO FINALIZACION PROGRAMA A LA CPU
					msg_enviar_separado(PAGINA_INVALIDA, 0, 0, socketCPU);

				} else if ((Cache_Activada()) && (estaEnCache(pidPeticion, puntero.pagina))) {

					char* contenido_leido = obtenerContenidoSegunCache(pidPeticion,puntero.pagina, puntero.offset, puntero.size);
					msg_enviar_separado(LECTURA_PAGINA, puntero.size, contenido_leido, socketCPU);
					if(puntero.size != sizeof(t_num))
						contenido_leido[puntero.size-1] = '\0';
					log_info(log_memoria, "contenido_leido %s", contenido_leido);
					free(contenido_leido);

				} else {

					log_info(log_memoria, "La pagina %d esta en Memoria Real", puntero.pagina);

					//-----Retardo
					pthread_mutex_lock(&mutexRetardo);
					usleep(retardoMemoria * 1000);
					pthread_mutex_unlock(&mutexRetardo);
					//------------

					int nroFrame = buscarFramePorPidPag(pidPeticion, puntero.pagina);
					char* contenido_leido = obtenerContenido(nroFrame, puntero.offset, puntero.size);
					msg_enviar_separado(LECTURA_PAGINA, puntero.size, contenido_leido, socketCPU);
					if(puntero.size != sizeof(t_num))
						contenido_leido[puntero.size-1] = '\0';
					log_info(log_memoria, "contenido_leido %s", contenido_leido);
					free(contenido_leido);

					if (Cache_Activada()) {
						agregarEntradaCache(pidPeticion, puntero.pagina, nroFrame);
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
				memcpy(&pidPeticion, msg->data + offset, tmpsize = sizeof(t_pid));
				offset += tmpsize;
				memcpy(&puntero, msg->data + offset, tmpsize = sizeof(t_posicion));
				offset += tmpsize;
				void* contenido_escribir = malloc(puntero.size);
				memcpy(contenido_escribir, msg->data + offset, tmpsize = puntero.size);
				offset += tmpsize;

				log_info(log_memoria, "Solicitud de escritura. Pag:%d Offset:%d Size:%d", puntero.pagina, puntero.offset, puntero.size);

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

				} else if ((Cache_Activada()) && (estaEnCache(pidPeticion, puntero.pagina))) {

					escribirContenidoSegunCache(pidPeticion, puntero.pagina, puntero.offset, puntero.size, contenido_escribir);
					free(contenido_escribir);

					//ENVIO ESCRITURA OK
					header = ESCRITURA_PAGINA;

					bytesEnviados = send(socketCPU, &header, sizeof(uint32_t), 0);
					if (bytesEnviados <= 0) {
						log_error(log_memoria, "Error send CPU");
						pthread_exit(NULL);
					}

				} else {

						log_info(log_memoria, "La pagina %d esta en Memoria Real",puntero.pagina);

						//-----Retardo
						pthread_mutex_lock(&mutexRetardo);
						usleep(retardoMemoria * 1000);
						pthread_mutex_unlock(&mutexRetardo);
						//------------


						int nroFrame = buscarFramePorPidPag(pidPeticion, puntero.pagina);
						escribirContenido(nroFrame, puntero.offset, puntero.size, contenido_escribir);
						free(contenido_escribir);

						//ENVIO ESCRITURA OK
						header = ESCRITURA_PAGINA;//OK

						bytesEnviados = send(socketCPU, &header, sizeof(uint32_t), 0);
						if (bytesEnviados <= 0) {
							log_error(log_memoria, "Error send CPU");
							pthread_exit(NULL);
						}

						if(Cache_Activada()){
							agregarEntradaCache(pidPeticion, puntero.pagina, nroFrame);
						}

						log_info(log_memoria, "Escribi correctamente");

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

	list_add(listaProcesos, procesoNuevo);

}

void eliminarProcesoDeListaDeProcesos(t_pid pid) {

	int _soy_el_proceso_buscado(t_proceso* proceso) {
		return proceso->PID == pid;
	}

	lockProcesos();
	list_remove_and_destroy_by_condition(listaProcesos, (void*) _soy_el_proceso_buscado, free);
	unlockProcesos();

	//-----Retardo
	pthread_mutex_lock(&mutexRetardo);
	usleep(retardoMemoria * 1000);
	pthread_mutex_unlock(&mutexRetardo);
	//------------

}

/*			LECTURA-ESCRITURA			*/

void* obtenerContenido(int frame, int offset, int tamanio_leer) {

	//-----Retardo
	pthread_mutex_lock(&mutexRetardo);
	usleep(retardoMemoria * 1000);
	pthread_mutex_unlock(&mutexRetardo);
	//------------

	// fixme: no considera q offset + tamanio_leer > tamanioFrame
	// 		en ese caso tengo q buscar un nuevo frame para seguir leyendo lo faltante (tamanio_leer - (tamanioFrame - offset)) <<--------------------------------------------------------------------------------
	// fixme: no considera q tamanio_leer > tamanioFrame
	// 		lo q significa q tendria q leer 2 o + frames (buscar siguiente frame del pid)
	//		pondria un int nroFrame = buscarPidPag(pidPeticion, puntero.pagina);
	// 		y un for( tamanio_leer > i*tamanioFrame, i++ ) o un do while, ver cual
	// fixme: no considera frame == -1
	//		osea q no se encontro el frame

	log_info(log_memoria, "Leo %d bytes - frame %d a partir de %d", tamanio_leer, frame, offset);

	void* contenido = malloc(tamanio_leer);
	int desplazamiento = (frame * tamanioDeMarcos) + offset;

	pthread_mutex_lock(&mutexMemoriaReal);
	memcpy(contenido, memoria_real + desplazamiento, tamanio_leer);
	pthread_mutex_unlock(&mutexMemoriaReal);

	return contenido;
}

void escribirContenido(int frame, int offset, int tamanio_escribir,	void* contenido) {

	//-----Retardo
	pthread_mutex_lock(&mutexRetardo);
	usleep(retardoMemoria * 1000);
	pthread_mutex_unlock(&mutexRetardo);
	//------------

	// fixme: no considera q offset + tamanio_escribir > tamanioFrame
	// 		en ese caso tengo q buscar un nuevo frame para seguir escribiendo lo faltante (tamanio_escribir - (tamanioFrame - offset)) <<--------------------------------------------------------------------------------
	// fixme: no considera q tamanio_escribir > tamanioFrame
	// 		lo q significa q tendria q escribir 2 o + frames (buscar siguiente frame del pid)
	//		pondria un int nroFrame = buscarPidPag(pidPeticion, puntero.pagina);
	// 		y un for( tamanio_escribir > i*tamanioFrame, i++ ) o un do while, ver cual
	// fixme: no considera frame == -1
	//		osea q no se encontro el frame

	int desplazamiento = (frame * tamanioDeMarcos) + offset;

	log_info(log_memoria, "Escribo %d bytes en (frame %d * tamanioDeMarcos %d) + offset %d = desplazamiento %d ",
			tamanio_escribir, frame, tamanioDeMarcos, offset, desplazamiento);

	pthread_mutex_lock(&mutexMemoriaReal);
	memcpy(memoria_real + desplazamiento, contenido, tamanio_escribir);
	pthread_mutex_unlock(&mutexMemoriaReal);

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
			frameNuevo->pid = -1;			//frame reservado para EA
		else
			frameNuevo->pid = 0;


		// i*sizeof(t_frame) es el offset
		memcpy(memoria_real + i*sizeof(t_frame), frameNuevo, sizeof(t_frame));
		log_info(log_memoria, "Cree frame %d", frameNuevo->nroFrame);
		free(frameNuevo);
		// fixme ver si tamanioMarcoRestante < sizeof(t_frame) afecta en algo
		// (si arranco en un frame nuevo o lo guardo por la mitad)
	}

	log_trace(log_memoria, "Memoria reservada y frames inicializados");
}

int hayFramesLibres(int cantidad) {
	int cantidadLibres = 0;
	t_frame* frame;

	lockFrames();
	int i=0;
	for(; i < cantidadDeMarcos ; i++){
		frame = malloc(sizeof(t_frame));
		// i*sizeof(t_frame) es el offset
		memcpy(frame, memoria_real + i*sizeof(t_frame), sizeof(t_frame));
		// fixme ver tamanioMarcoRestante < sizeof(t_frame) ...

		if(frame->pid == 0)
			cantidadLibres++;
		free(frame);
	}
	unlockFrames();

	log_trace(log_memoria, "Cantidad libres %d", cantidadLibres);
	return cantidad <= cantidadLibres;
}

int buscarFrameLibre(t_pid pid) {

	int _soy_el_pid_buscado(t_proceso* proceso) {
		return (proceso->PID == pid);
	}
	lockProcesos();
	t_proceso* proceso = list_remove_by_condition(listaProcesos, (void*) _soy_el_pid_buscado);

	int nroPagNueva = proceso->cantFramesAsignados;
	// fixme mal uso, deberia buscar el mayor nroFrame
	// 	 no considera un nro de pag > cantFramesAsignados

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
		// fixme ver tamanioMarcoRestante < sizeof(t_frame) ...

		if(frame->pid == 0){
			log_trace(log_memoria, "Asigno a frame %d pid %d pag %d", frame->nroFrame, pid, nroPagNueva);
			frame->pid = pid;
			frame->nroPag = nroPagNueva;
			memcpy(memoria_real + i*sizeof(t_frame), frame, sizeof(t_frame));
			proceso->cantFramesAsignados++;
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
	int indice = ((pid * cantidadDeMarcos) + (nroPagina * tamanioDeMarcos)) / cantidadDeMarcos;
	// fixme: no considera indice >= cantidadDeMarcos
	// ej: pid=8 nroPagina=5 cantMarcos=30 tamanioMarocs=256  =>  (8*30 + 5*256)/30 = 50
	//			=> voy al marco 50 pero en total son 30 => explota
	return indice;
}

int paginaInvalida(t_pid pid, t_num16 nroPagina) {

	int _soy_el_pid_buscado(t_proceso* proceso) {
		return (proceso->PID == pid);
	}
	lockProcesos();
	t_proceso* proceso = list_find(listaProcesos, (void*) _soy_el_pid_buscado);

	int esPagInvalida = (nroPagina >= proceso->cantFramesAsignados);
	// fixme mal uso, deberia buscar el mayor nroFrame ...
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
		// fixme ver tamanioMarcoRestante < sizeof(t_frame) ...

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
/*
int agregarNuevaPagina(t_num8 pid){

	int _soy_el_pid_buscado(t_proceso* proceso) {
		return (proceso->PID == pid);
	}
	t_frame* frame;
	int nroFrame = buscarFrameLibre(pid);

	lockFrames();
	if (nroFrame != -1) {
		lockProcesos();
		t_proceso* proceso = list_find(listaProcesos, (void*) _soy_el_pid_buscado);

		frame = malloc(sizeof(t_frame));
		frame->nroFrame = nroFrame;
		frame->nroPag = proceso->cantFramesAsignados - 1;
		// fixme mal uso, deberia buscar el mayor nroFrame
		frame->pid = pid;
		memcpy(memoria_real + nroFrame*sizeof(t_frame), frame, sizeof(t_frame));

		unlockProcesos();
	}
	unlockFrames();

	log_info(log_memoria, "Nueva Pagina: %d pid %d en frame %d", frame->nroPag, pid, nroFrame);
	free(frame);
	return nroFrame;
}*/

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
		// fixme ver tamanioMarcoRestante < sizeof(t_frame) ...

		if(frame->pid == pid && frame->nroPag == nroPagina){
			frame->pid = 0;
			frame->nroPag = 0;
			flag_sigo = 0;
			proceso->cantFramesAsignados--;
			memcpy(frame, memoria_real + i*sizeof(t_frame), sizeof(t_frame));
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
	int cantPagsALiberar = proceso->cantFramesAsignados, i;
	// fixme mal uso, deberia buscar el mayor nroFrame
	unlockProcesos();

	for(i = 0; i < cantPagsALiberar; i++){
		liberarPagina(pid, i);
	}

}





/*			MISC			*/
void terminarMemoria(){			//aca libero todos

	// todo list_destroy(listaCPUs);

	list_destroy_and_destroy_elements(listaProcesos, free);

	free(memoria_real);

	log_trace(log_memoria, "Termino Proceso");
	log_destroy(log_memoria);
	exit(1);
}










/*			CACHE (y otros)			*/
t_list* crearCache() {
	t_list* cache = list_create();
	return cache;
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

t_cache* crearRegistroCache(t_pid pid, t_num16 numPag, int numFrame) {

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

void agregarEntradaCache(t_pid pid, t_num16 numero_pagina, int nroFrame) {

	t_cache* nuevaEntradaCache = crearRegistroCache(pid, numero_pagina, nroFrame);

	pthread_mutex_lock(&mutexCache);
	if (hayEspacioEnCache()) {
		list_add(Cache, nuevaEntradaCache);
	}

	else {
		algoritmoLRU();
		list_add(Cache, nuevaEntradaCache);
	}

	log_info(log_memoria, "Pagina %d del proceso %d agregada a Cache", numero_pagina, pid);

	pthread_mutex_unlock(&mutexCache);
}

void* obtenerContenidoSegunCache(t_pid pid, t_num16 numero_pagina, uint32_t offset, uint32_t tamanio_leer) {

	t_cache* entradaCache = buscarEntradaCache(pid, numero_pagina);

	pthread_mutex_lock(&mutexCantAccesosMemoria);
	entradaCache->ultimoAcceso = cantAccesosMemoria;
	pthread_mutex_unlock(&mutexCantAccesosMemoria);

	void* contenido = obtenerContenido(entradaCache->numFrame, offset,tamanio_leer);

	pthread_mutex_unlock(&mutexCache);

	return contenido;

}

void escribirContenidoSegunCache(t_pid pid, t_num16 numero_pagina, uint32_t offset, uint32_t tamanio_escritura, void* contenido_escribir) {

	t_cache* entradaCache = buscarEntradaCache(pid, numero_pagina);

	pthread_mutex_lock(&mutexCantAccesosMemoria);
	entradaCache->ultimoAcceso = cantAccesosMemoria;
	pthread_mutex_unlock(&mutexCantAccesosMemoria);

	escribirContenido(entradaCache->numFrame, offset, tamanio_escritura,contenido_escribir);

	pthread_mutex_unlock(&mutexCache);

}

void vaciarCache() {

	if (Cache_Activada()) {
		pthread_mutex_lock(&mutexCache);
		eliminarCache(Cache);
		Cache = crearCache();
		pthread_mutex_unlock(&mutexCache);

		printf("Flush Cache realizado\n");
		log_info(log_memoria, "Flush Cache realizado");

	} else {
		printf("Se intento hacer flush cache pero no esta activada\n");
		log_error(log_memoria, "Se intento hacer flush cache pero no esta activada");
	}

}

void borrarEntradasCacheSegunPID(t_pid pid) {

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

	return (cantidadEntradasCache > 0);

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
		printf("Ingrese comando:\n\n");
		scanf("%s", buffer);

		if (!strcmp(buffer, "retardo")) {

			log_error(log_memoria, "Comando: retardo");
			int retardoNuevo;

			printf("\nIngrese nuevo valor de retardo:\n\n");
			scanf("%d", &retardoNuevo);

			pthread_mutex_lock(&mutexRetardo);
			retardoMemoria = retardoNuevo;
			pthread_mutex_unlock(&mutexRetardo);

			printf("Retardo modificado correctamente a %d ms\n\n", retardoMemoria);

			log_info(log_memoria, "Retardo modificado correctamente a %d ms",retardoMemoria);

		} else if (!strcmp(buffer, "dump")) {

			log_error(log_memoria, "Comando: dump");
			int pid;

			printf("Ingrese 0 si quiere un reporte de todos los procesos o ingrese el pid del proceso para un reporte particular\n\n");
			scanf("%d", &pid);

			/* fixme no responde a enunciado
				○ Caché:​ Este comando hará un dump completo de la memoria Caché.
				○ Estructuras de memoria​: Tabla de Páginas y Listado de procesos Activos
				○ Contenido de memoria​: Datos almacenados en la memoria de todos los procesos o de un proceso en particular.
			*/


			lockFramesYProcesos();

			if (pid == 0) {
				dumpTodosLosProcesos();

			} else {
				dumpProcesoParticular(pid);
			}

			unlockFramesYProcesos();

			}else if (!strcmp(buffer, "flush")) {

				log_error(log_memoria, "Comando: flush");
					vaciarCache();

			} else if(!strcmp(buffer,"size")) {

				printf("Ingrese memoria o pid\n\n");
				scanf("%s",buffer);

				if(!strcmp(buffer,"memoria")){
					printf("La cantidad de frames en memoria es de : %d\n\n",cantidadDeMarcos);

					/*
					 todo: rehacer considerando q no hay listFrames
					int cantidadDeFrames = list_size(listaFrames);
					printf("La cantidad de frames segun lista de frames es de : %d\n\n",cantidadDeFrames);

					int frameUsado(t_frame* frame) {
							return (frame->bit_modif != 0);
						}

					int cantidadDeFramesLlenos = list_size(list_filter(listaFrames,(void*) frameUsado));
					printf("La cantidad de frames usados es de : %d\n\n",cantidadDeFramesLlenos);


					int cantidadDeFramesVacios = cantidadDeFrames - cantidadDeFramesLlenos;
					printf("La cantidad de frames vacios es de %d\n\n",cantidadDeFramesVacios);
					*/

				}else if(!strcmp(buffer,"pid")){

					int pid;
					printf("Ingrese el numero de PID:\n\n");
					scanf("%d",&pid);

					t_proceso* proceso = buscarProcesoEnListaProcesosParaDump(pid);

					if (proceso == NULL)
						printf("No existe ese proceso\n\n");
					else
						printf("El tamaño es de %d frames\n\n",proceso->cantFramesAsignados);

			}
			else{
				printf("Comando Incorrecto\n\n");
			}
		}
		else{
			printf("Comando Incorrecto\n\n");
		}

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

	char nombreArchivoDump[40] = "dumpTotal_";
	int pidLinux = process_getpid();
	strcat(nombreArchivoDump, string_itoa(pidLinux));
	strcat(nombreArchivoDump, ".txt");
	FILE* archivoDump = fopen(nombreArchivoDump, "w+");

	//unsigned int tamanioTotalMemoria = tamanioDeMarcos * cantidadDeMarcos;

	dumpEstructurasMemoriaTodosLosProcesos(archivoDump);

	fprintf(archivoDump,
			"\n//////////////////MEMORIA PRINCIPAL//////////////////\n");

	printf("\n//////////////////MEMORIA PRINCIPAL//////////////////\n\n");

	pthread_mutex_lock(&mutexMemoriaReal);
	//hexdump(memoria_real, tamanioTotalMemoria, archivoDump); todo q onda esto?
	pthread_mutex_unlock(&mutexMemoriaReal);

	printf("\nDUMP DE MEMORIA PRINCIPAL PARA TODOS LOS PROCESOS REALIZADO\n");
	fprintf(archivoDump,
			"\n//////////////////MEMORIA PRINCIPAL//////////////////\n");

	fclose(archivoDump);
}

void dumpContenidoMemoriaProceso(t_pid pid, FILE* archivoDump) {

	int i = 0, nroFrame;
	void* contenidoFrame;
	t_proceso* proceso = buscarProcesoEnListaProcesosParaDump(pid);

	fprintf(archivoDump,
			"\n//////////////////MEMORIA PRINCIPAL//////////////////\n");

	printf("\n//////////////////MEMORIA PRINCIPAL//////////////////\n\n");

	// fixme mal uso, deberia buscar el mayor nroFrame
	while (i < proceso->cantFramesAsignados) {

		nroFrame = buscarFramePorPidPag(pid, i);
		contenidoFrame = obtenerContenido(nroFrame, 0, tamanioDeMarcos);
		hexdump(contenidoFrame, tamanioDeMarcos, archivoDump);

		i++;
	}

	printf("\nDUMP DE MEMORIA PRINCIPAL PARA EL PROCESO %d REALIZADO\n",
			proceso->PID);
}

void dumpProcesoParticular(t_pid pid) {

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

		dumpContenidoMemoriaProceso(pid, archivoDump);

		fclose(archivoDump);
	}


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
	int cantPagsAMostrar = proceso->cantFramesAsignados;
	// fixme mal uso, deberia buscar el mayor nroFrame
	unlockProcesos();

	int numeroPagina, nroFrame;
	for(numeroPagina = 0; numeroPagina <= cantPagsAMostrar; numeroPagina++){

		nroFrame = buscarFramePorPidPag(pid, numeroPagina);
		char* contenido_leido = obtenerContenido(nroFrame, 0, tamanioDeMarcos);

		printf("Numero de Pagina: %d \n\n", numeroPagina);
		printf("Contenido: %s",contenido_leido);

	}

}





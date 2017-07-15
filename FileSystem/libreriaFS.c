#include "libreriaFS.h"

void mostrarArchivoConfig() {

	printf("El puerto del FS es: %d\n", puertoFS);
	printf("El punto de montaje es: %s\n", puntoMontaje);


}

void escucharKERNEL(void* socket_kernel) {

	//Casteo socketNucleo
	int socketKernel = (int) socket_kernel;

	fprintf(stderr, "Se conecto el Kernel\n");

	int bytesEnviados = send(socketKernel, "Conexion Aceptada", 18, 0);
	if (bytesEnviados <= 0) {
		fprintf(stderr, "Error send KERNEL\n");
		pthread_exit(NULL);
	}

	while(1){

		t_msg* msgRecibido = msg_recibir(socketKernel);
		if(msgRecibido->longitud > 0)
			msg_recibir_data(socketKernel, msgRecibido);
		char* path;
		void* buffer;
		int tmpsize = 0, tmpoffset = 0;
		t_num offset, size;
		t_num sizePath = 0;
		tipoError = 0;

		pthread_mutex_lock(&mutex_solicitud);
		log_info(logFS, "Atiendo solicitud de Kernel %d", socketKernel);

		switch(msgRecibido->tipoMensaje){

		case 0:
			fprintf(stderr, "El kernel %d se ha desconectado \n", socketKernel);
			log_trace(logFS, "Se desconecto el kernel %d", socketKernel);
			pthread_mutex_unlock(&mutex_solicitud);
			pthread_exit(NULL);
			break;


		case VALIDAR_ARCHIVO:
			log_info(logFS, "Validar archivo");
			path = malloc(msgRecibido->longitud+1);
			memcpy(path, msgRecibido->data, msgRecibido->longitud);
			path[msgRecibido->longitud] = '\0';
			log_info(logFS, "Path: %s", path);

			char* rutaMetadata = string_new();
			string_append(&rutaMetadata, puntoMontaje);
			string_append(&rutaMetadata, "/Archivos");
			string_append(&rutaMetadata, path);
			log_info(logFS, "rutaMetadata %s", rutaMetadata);

			int fd = open(path, O_RDONLY);

			if (fd < 0){
				log_info(logFS, "El archivo no existe");
				t_num exitcode = ARCHIVO_INEXISTENTE;
				msg_enviar_separado(ERROR, sizeof(t_num), &exitcode, socketKernel);
			}else{
				log_info(logFS, "El archivo existe");
				msg_enviar_separado(VALIDAR_ARCHIVO, 0, 0, socketKernel);
			}
			close(fd);
			free(path);
			break;


		case CREAR_ARCHIVO:
			log_info(logFS, "Crear archivo");

			path = malloc(msgRecibido->longitud+1);
			memcpy(path, msgRecibido->data, msgRecibido->longitud);
			path[msgRecibido->longitud] = '\0';
			log_info(logFS, "Path: %s", path);

			crearArchivo(path);

			if(tipoError < 0)
				msg_enviar_separado(tipoError, 0, 0, socketKernel);
			else
				msg_enviar_separado(CREAR_ARCHIVO, 0, 0, socketKernel);
			free(path);
			break;


		case BORRAR:
			log_info(logFS, "Borrar archivo");

			path = malloc(msgRecibido->longitud+1);
			memcpy(path, msgRecibido->data, msgRecibido->longitud);
			path[msgRecibido->longitud] = '\0';
			log_info(logFS, "Path: %s", path);

			borrarArchivo(path);

			if(tipoError < 0)
				msg_enviar_separado(tipoError, 0, 0, socketKernel);
			else
				msg_enviar_separado(BORRAR, 0, 0, socketKernel);
			free(path);
			break;


		case OBTENER_DATOS:
			log_info(logFS, "Obtener datos archivo");

			memcpy(&sizePath, msgRecibido->data + tmpoffset, tmpsize = sizeof(t_num));
			tmpoffset += tmpsize;
			path = malloc(sizePath+1);
			memcpy(path, msgRecibido->data + tmpoffset, tmpsize = sizePath);
			path[sizePath] = '\0';
			tmpoffset += tmpsize;
			memcpy(&offset, msgRecibido->data + tmpoffset, tmpsize = sizeof(t_valor_variable));
			tmpoffset += tmpsize;
			memcpy(&size, msgRecibido->data + tmpoffset, tmpsize = sizeof(t_valor_variable));
			tmpoffset += tmpsize;
			log_info(logFS, "Path: %s - offset: %d - size %d", path, offset, size);

			char* data = leerBloquesArchivo(path, offset, size);

			if(tipoError < 0)
				msg_enviar_separado(tipoError, 0, 0, socketKernel);
			else
				msg_enviar_separado(OBTENER_DATOS, size, data, socketKernel);
			free(data);
			free(path);
			break;


		case GUARDAR_DATOS:
			log_info(logFS, "Guardar datos archivo");

			memcpy(&sizePath, msgRecibido->data + tmpoffset, tmpsize = sizeof(t_num));
			tmpoffset += tmpsize;
			path = malloc(sizePath+1);
			memcpy(path, msgRecibido->data + tmpoffset, sizePath);
			path[sizePath] = '\0';
			tmpoffset += tmpsize;
			memcpy(&offset, msgRecibido->data + tmpoffset, sizeof(t_valor_variable));
			tmpoffset += sizeof(t_valor_variable);
			memcpy(&size, msgRecibido->data + tmpoffset, sizeof(t_valor_variable));
			tmpoffset += sizeof(t_valor_variable);
			buffer = malloc(size);
			memcpy(buffer, msgRecibido->data + tmpoffset, size);
			log_info(logFS, "Path: %s - offset: %d - size %d \n buffer %s", path, offset, size, buffer);

			escribirBloquesArchivo(path, offset, size, buffer);

			if(tipoError < 0)
				msg_enviar_separado(tipoError, 0, 0, socketKernel);
			else
				msg_enviar_separado(GUARDAR_DATOS, 0, 0, socketKernel);
			free(data);
			free(path);
			break;

		}

		log_info(logFS, "Finalizo solicitud de Kernel %d", socketKernel);
		pthread_mutex_unlock(&mutex_solicitud);
	}
}




void leerMetadataArchivo(){

	char* rutaMetadata = string_duplicate(puntoMontaje);
	string_append(&rutaMetadata, "/Metadata/Metadata.bin");
	log_info(logFS, "rutaMetadata %s", rutaMetadata);

	t_config* metadata = config_create(rutaMetadata);
	if(metadata == NULL){
		log_error(logFS, "No se encuentra metadata");
		fprintf(stderr, "No se encuentra metadata\n");
		finalizarFS();
	}
	tamanioBloques = config_get_int_value(metadata, "TAMANIO_BLOQUES");
	cantidadBloques = config_get_int_value(metadata, "CANTIDAD_BLOQUES");

	if(!string_equals_ignore_case( config_get_string_value(metadata, "MAGIC_NUMBER") , "SADICA")){
		log_error(logFS, "No es un FS SADICA");
		fprintf(stderr, "No es un FS SADICA\n");
		finalizarFS();
	}
	config_destroy(metadata);

}

void leerBitMap(){
	int fd;
	char *data;
	struct stat sbuf;
	char* rutaBitMap = string_duplicate(puntoMontaje);
	string_append(&rutaBitMap, "/Metadata/Bitmap.bin");
	log_info(logFS, "rutaBitMap %s", rutaBitMap);

	fd = open(rutaBitMap, O_RDWR);
	if (fd < 0) {
		perror("error al abrir el archivo bitmap");
		finalizarFS();
	}
	if (stat(rutaBitMap, &sbuf) < 0) {
		perror("stat, chequear si el archivo esta corrupto");
		finalizarFS();
	}

	data = mmap((caddr_t)0, sbuf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED) {
		perror("Fallo el mmap del bitmap");
		finalizarFS();
	}
	bitMap = bitarray_create_with_mode(data, sbuf.st_size, MSB_FIRST);

	close(fd);
}

void crearArchivo(void* path){
	int nroBloque = -1, i;
	char* rutaMetadata = string_new();
	string_append(&rutaMetadata, puntoMontaje);
	string_append(&rutaMetadata, "/Archivos");
	string_append(&rutaMetadata, path);
	log_info(logFS, "rutaMetadata %s", rutaMetadata);
	log_info(logFS, "bitMap->size %d", bitMap->size);

	for(i = 0; i < bitMap->size && nroBloque == -1; i++){
		if(!bitarray_test_bit(bitMap, i)){
			bitarray_set_bit(bitMap, i);
			nroBloque = i;
		}
	}

	char* data = string_from_format("TAMANIO=1 \nBLOQUES=[%d]", nroBloque);
	// si arrancara en 1 los bloques seria nroBloque+1

	system(string_from_format("touch %s", rutaMetadata));

	int fd = open(rutaMetadata, O_RDWR);
	if(fd < 0){
		log_error(logFS, "No se creo el archivo");
		tipoError = SIN_DEFINICION;
	}
	write(fd, data, string_length(data));
	close(fd);

	free(rutaMetadata);
}

void borrarArchivo(void* path){
	int i;
	char* rutaMetadata = string_new();
	string_append(&rutaMetadata, puntoMontaje);
	string_append(&rutaMetadata, "/Archivos");
	string_append(&rutaMetadata, path);
	log_info(logFS, "rutaMetadata %s", rutaMetadata);

	t_config* metadata = config_create(rutaMetadata);
	if(metadata == NULL){
		log_error(logFS, "No se encuentra metadata");
		fprintf(stderr, "No se encuentra archivo %s\n", rutaMetadata);
		free(rutaMetadata);
		free(metadata);
		tipoError = ARCHIVO_INEXISTENTE;
		return;
	}
	int tamanio = config_get_int_value(metadata, "TAMANIO");
	char** bloques = config_get_array_value(metadata, "BLOQUES");

	for(i = 0; i*tamanioBloques < tamanio; i++){
		int nroBloque =  atoi(bloques[i]);
		bitarray_clean_bit(bitMap, nroBloque);
		// si arrancara en 1 los bloques seria nroBloque-1
	}

	config_destroy(metadata);
	system(string_from_format("rm -f %s", rutaMetadata));
	free(rutaMetadata);
	free(bloques);
}

char* leerBloquesArchivo(void* path, int offset, int size){
	char* data = malloc(size+1), *tmpdata, *pathBloque;
	int i, tmpoffset = 0;
	char* rutaMetadata = string_new();
	string_append(&rutaMetadata, puntoMontaje);
	string_append(&rutaMetadata, "/Archivos");
	string_append(&rutaMetadata, path);
	log_info(logFS, "rutaMetadata %s", rutaMetadata);

	t_config* metadata = config_create(rutaMetadata);
	if(metadata == NULL){
		log_error(logFS, "No se encuentra metadata");
		fprintf(stderr, "No se encuentra archivo %s\n", rutaMetadata);
		free(rutaMetadata);
		free(metadata);
		tipoError = ARCHIVO_INEXISTENTE;
		return NULL;
	}
	int tamanio = config_get_int_value(metadata, "TAMANIO");
	char** bloques = config_get_array_value(metadata, "BLOQUES");

	for(i = offset / tamanioBloques; i*tamanioBloques < tamanio; i++, tmpoffset += tamanioBloques){
		pathBloque = string_new();
		string_append(&pathBloque, puntoMontaje);
		string_append_with_format(&pathBloque, "/Bloques/%s.bin", bloques[i]);
		tmpdata = leerArchivo(pathBloque);

		if(size-tmpoffset > tamanioBloques)	//lo q falta
			memcpy(data + tmpoffset, tmpdata, tamanioBloques);
		else{
			memcpy(data + tmpoffset, tmpdata, size-tmpoffset);
			break;
		}
	}
	data[size] = '\0';
	log_info(logFS, "Contenido leido %s", data);

	free(rutaMetadata);
	free(pathBloque);
	free(bloques);
	return data;
}

void escribirBloquesArchivo(void* path, int offset, int size, char* buffer){
	char *tmpdata, *pathBloque;
	int i, tmpoffset = 0;
	char* rutaMetadata = string_new();
	string_append(&rutaMetadata, puntoMontaje);
	string_append(&rutaMetadata, "/Archivos");
	string_append(&rutaMetadata, path);
	log_info(logFS, "rutaMetadata %s", rutaMetadata);

	t_config* metadata = config_create(rutaMetadata);
	if(metadata == NULL){
		log_error(logFS, "No se encuentra metadata");
		fprintf(stderr, "No se encuentra archivo %s\n", rutaMetadata);
		free(rutaMetadata);
		free(metadata);
		tipoError = ARCHIVO_INEXISTENTE;
		return;
	}
	int tamanio = config_get_int_value(metadata, "TAMANIO");
	char** bloques = config_get_array_value(metadata, "BLOQUES");

	for(i = offset / tamanioBloques; i < tamanio; i += tamanioBloques, tmpoffset += tamanioBloques){
		pathBloque = string_new();
		string_append(&pathBloque, puntoMontaje);
		string_append_with_format(&pathBloque, "/Bloques/%s.bin", bloques[i / tamanioBloques]);
		tmpdata = leerArchivo(pathBloque);

		if(tmpdata == NULL)
			return;

		if(size-tmpoffset > tamanioBloques)	//lo q falta
			memcpy(tmpdata, buffer + tmpoffset, tamanioBloques);
		else{
			memcpy(tmpdata, buffer + tmpoffset, size-tmpoffset);
			break;
		}
	}

	free(rutaMetadata);
	free(pathBloque);
	free(bloques);
}

char* leerArchivo(void* path){
	int fd;
	char *data;
	struct stat sbuf;

	fd = open(path, O_RDWR);
	if (fd < 0) {
		perror("error al abrir el archivo");
		tipoError = ARCHIVO_INEXISTENTE;
		return NULL;
	}
	if (stat(path, &sbuf) == -1) {
		perror("stat, chequear si el archivo esta corrupto");
		tipoError = SIN_DEFINICION;
		return NULL;
	}
	data = mmap((caddr_t)0, sbuf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED) {
		perror("Fallo el mmap");
		tipoError = SIN_DEFINICION;
		return NULL;
	}
	close(fd);
	return data;
}



void finalizarFS(){
	log_trace(logFS, "Finalizo FileSystem");

	if(bitMap != NULL)
		bitarray_destroy(bitMap);

	exit(1);
}

#include "libreriaFS.h"

void mostrarArchivoConfig() {

	printf("El puerto del FS es: %d\n", puertoFS);
	printf("El punto de montaje es: %s\n", puntoMontaje);


}

void escucharKERNEL(void* socket_kernel) {

	//Casteo socketNucleo
	int socketKernel = (int) socket_kernel;

	printf("Se conecto el Kernel\n");

	int bytesEnviados = send(socketKernel, "Conexion Aceptada", 18, 0);
	if (bytesEnviados <= 0) {
		printf("Error send KERNEL");
		pthread_exit(NULL);
	}

	while(1){

		// lo unico q esta haciendo es mostrar lo que se recibio
		t_msg* msgRecibido = msg_recibir(socketKernel);
		msg_recibir_data(socketKernel, msgRecibido);
		void* path, *buffer;
		int tmpsize = 0, tmpoffset = 0;
		t_num offset, size;

		pthread_mutex_lock(&mutex_solicitud);
		log_info(logFS, "Atiendo solicitud de %d", socketKernel);

		switch(msgRecibido->tipoMensaje){
		case 0:
			fprintf(stderr, "El kernel %d se ha desconectado \n", socketKernel);
			log_trace(logFS, "Se desconecto el kernel %d", socketKernel);
			pthread_mutex_unlock(&mutex_solicitud);
			pthread_exit(NULL);
			break;
		case VALIDAR_ARCHIVO:
			path = malloc(msgRecibido->longitud);
			memcpy(path, msgRecibido->data, msgRecibido->longitud);
			log_info(logFS, "Path: %s", path);

			//todo: implementar
			break;
		case CREAR_ARCHIVO:
			path = malloc(msgRecibido->longitud);
			memcpy(path, msgRecibido->data, msgRecibido->longitud);
			log_info(logFS, "Path: %s", path);

			crearArchivo(path);
			msg_enviar_separado(CREAR_ARCHIVO, 0, 0, socketKernel);
			free(path);
			break;
		case BORRAR:
			path = malloc(msgRecibido->longitud);
			memcpy(path, msgRecibido->data, msgRecibido->longitud);
			log_info(logFS, "Path: %s", path);

			borrarArchivo(path);
			msg_enviar_separado(BORRAR, 0, 0, socketKernel);
			free(path);
			break;
		case OBTENER_DATOS:
			tmpsize = msgRecibido->longitud - sizeof(t_num)*2;
			path = malloc(tmpsize);
			memcpy(path, msgRecibido->data, tmpsize);
			tmpoffset += tmpsize;
			memcpy(&offset, msgRecibido->data + tmpoffset, sizeof(t_num));
			tmpoffset += sizeof(t_num);
			memcpy(&size, msgRecibido->data + tmpoffset, sizeof(t_num));
			log_info(logFS, "Path: %s - offset: %d - size %d", path, offset, size);

			char* data = leerBloquesArchivo(path, offset, size);
			msg_enviar_separado(OBTENER_DATOS, size, data, socketKernel);
			free(data);

			break;
		case GUARDAR_DATOS:

			memcpy(&tmpsize, msgRecibido->data, sizeof(t_num));
			tmpoffset += sizeof(t_num);
			path = malloc(tmpsize);
			memcpy(path, msgRecibido->data + tmpoffset, tmpsize);
			tmpoffset += tmpsize;
			memcpy(&offset, msgRecibido->data + tmpoffset, sizeof(t_num));
			tmpoffset += sizeof(t_num);
			memcpy(&size, msgRecibido->data + tmpoffset, sizeof(t_num));
			tmpoffset += sizeof(t_num);
			buffer = malloc(size);
			memcpy(buffer, msgRecibido->data + tmpoffset, size);
			log_info(logFS, "Path: %s - offset: %d - size %d \n buffer %s", path, offset, size, buffer);

			escribirBloquesArchivo(path, offset, size, buffer);
			msg_enviar_separado(GUARDAR_DATOS, 0, 0, socketKernel);
			free(data);
			break;

		}

		log_info(logFS, "Finalizo solicitud de %d", socketKernel);
		pthread_mutex_unlock(&mutex_solicitud);
	}
}




void leerMetadataArchivo(){

	char* rutaMetadata = string_new();
	string_append(&rutaMetadata, puntoMontaje);
	string_append(&rutaMetadata, "/Metadata/Metadata.bin");
	log_info(logFS, "rutaMetadata %s", rutaMetadata);

	t_config* metadata = config_create(rutaMetadata);
	if(metadata == NULL){
		log_error(logFS, "No se encuentra metadata");
		free(rutaMetadata);
		config_destroy(metadata);
		exit(1);
	}
	tamanioBloques = config_get_int_value(metadata, "TAMANIO_BLOQUES");
	cantidadBloques = config_get_int_value(metadata, "CANTIDAD_BLOQUES");
	char* magicNumber = config_get_string_value(metadata, "MAGIC_NUMBER");

	if(!string_equals_ignore_case(magicNumber, "SADICA")){
		log_error(logFS, "No es un FS SADICA");
		config_destroy(metadata);
		free(rutaMetadata);
		free(magicNumber);
		exit(1);
	}
	config_destroy(metadata);
	free(rutaMetadata);
	free(magicNumber);
}

void leerBitMap(){
	int fd;
	char *data;
	struct stat sbuf;
	char* rutaBitMap = string_new();
	string_append(&rutaBitMap, puntoMontaje);
	string_append(&rutaBitMap, "/Metadata/Bitmap.bin");
	log_info(logFS, "rutaBitMap %s", rutaBitMap);

	fd = open(rutaBitMap, O_RDWR);
	if (fd == -1) {
		perror("error al abrir el archivo bitmap");
		exit(1);		//fixme: reemplazar exit por funcion finalizar
	}
	if (stat(rutaBitMap, &sbuf) == -1) {
		perror("stat, chequear si el archivo esta corrupto");
		exit(1);
	}
	data = mmap((caddr_t)0, sbuf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED) {
		perror("Fallo el mmap del bitmap");
		exit(1);
	}
	bitMap = bitarray_create_with_mode(data, sbuf.st_size, LSB_FIRST);	//fixme: LSB_FISRT o MSB_FIRST ??

	close(fd);
}

void crearArchivo(void* path){
	int nroBloque;
	char* rutaMetadata = string_new();
	string_append(&rutaMetadata, puntoMontaje);
	string_append(&rutaMetadata, path);
	log_info(logFS, "rutaMetadata %s", rutaMetadata);

	for(nroBloque = 0; nroBloque < bitMap->size; nroBloque++){
		if(!bitarray_test_bit(bitMap, nroBloque)){
			bitarray_set_bit(bitMap, nroBloque);
			break;
		}
	}

	char* data = string_from_format("TAMANIO=1 BLOQUES=[%d]", nroBloque);

	system(string_from_format("touch %s", rutaMetadata));

	int fd = open(rutaMetadata, O_RDWR);
	write(fd, data, string_length(data));
	close(fd);

	free(rutaMetadata);
}

void borrarArchivo(void* path){
	int i;
	char* rutaMetadata = string_new();
	string_append(&rutaMetadata, puntoMontaje);
	string_append(&rutaMetadata, path);
	log_info(logFS, "rutaMetadata %s", rutaMetadata);

	t_config* metadata = config_create(rutaMetadata);
	if(metadata == NULL){
		log_error(logFS, "No se encuentra metadata");
		free(rutaMetadata);
		config_destroy(metadata);
		return;
	}
	int tamanio = config_get_int_value(metadata, "TAMANIO");
	char** bloques = config_get_array_value(metadata, "BLOQUES");

	for(i = 0; i*tamanioBloques < tamanio; i++){
		int nroBloque =  atoi(bloques[i / tamanioBloques]);
		bitarray_clean_bit(bitMap, nroBloque);
	}

	config_destroy(metadata);
	system(string_from_format("rm -f %s", rutaMetadata));
	free(rutaMetadata);
	free(bloques);
}

char* leerBloquesArchivo(void* path, int offset, int size){
	char* data = malloc(size), *tmpdata, *pathBloque;
	int i, tmpoffset = 0;
	char* rutaMetadata = string_new();
	string_append(&rutaMetadata, puntoMontaje);
	string_append(&rutaMetadata, path);
	log_info(logFS, "rutaMetadata %s", rutaMetadata);

	t_config* metadata = config_create(rutaMetadata);
	if(metadata == NULL){
		log_error(logFS, "No se encuentra metadata");
		free(rutaMetadata);
		config_destroy(metadata);
		return NULL;
	}
	int tamanio = config_get_int_value(metadata, "TAMANIO");
	char** bloques = config_get_array_value(metadata, "BLOQUES");

	for(i = offset / tamanioBloques; i < tamanio; i += tamanioBloques, tmpoffset += tamanioBloques){
		pathBloque = string_new();
		string_append(&pathBloque, puntoMontaje);
		string_append_with_format(&pathBloque, "/Bloques/%s.bin", bloques[i / tamanioBloques]);
		tmpdata = leerArchivo(pathBloque);

		if(size-tmpoffset > tamanioBloques)	//lo q falta
			memcpy(data + tmpoffset, tmpdata, tamanioBloques);
		else{
			memcpy(data + tmpoffset, tmpdata, size-tmpoffset);
			break;
		}
	}

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
	string_append(&rutaMetadata, path);
	log_info(logFS, "rutaMetadata %s", rutaMetadata);

	t_config* metadata = config_create(rutaMetadata);
	if(metadata == NULL){
		log_error(logFS, "No se encuentra metadata");
		free(rutaMetadata);
		config_destroy(metadata);
		return;
	}
	int tamanio = config_get_int_value(metadata, "TAMANIO");
	char** bloques = config_get_array_value(metadata, "BLOQUES");

	for(i = offset / tamanioBloques; i < tamanio; i += tamanioBloques, tmpoffset += tamanioBloques){
		pathBloque = string_new();
		string_append(&pathBloque, puntoMontaje);
		string_append_with_format(&pathBloque, "/Bloques/%s.bin", bloques[i / tamanioBloques]);
		tmpdata = leerArchivo(pathBloque);

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
	if (fd == -1) {
		perror("error al abrir el archivo");
		return NULL;
	}
	if (stat(path, &sbuf) == -1) {
		perror("stat, chequear si el archivo esta corrupto");
		return NULL;
	}
	data = mmap((caddr_t)0, sbuf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED) {
		perror("Fallo el mmap");
		return NULL;
	}
	close(fd);
	return data;
}

#ifndef MEMORIA_LIBRERIAMEMORIA_H_
#define MEMORIA_LIBRERIAMEMORIA_H_

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/queue.h>
#include <commons/bitarray.h>
#include <commons/log.h>
#include <commons/process.h>
#include <herramientas/sockets.h>
#include <herramientas/enum.h>



//Archivo de Configuracion
int puertoFS;
char* puntoMontaje;
int tamanioBloques;
int cantidadBloques;
t_bitarray* bitMap;

t_log* logFS;

pthread_mutex_t mutex_solicitud;


void mostrarArchivoConfig();
void leerMetadataArchivo();
void leerBitMap();
char* leerBloquesArchivo(void* path, int offset, int size);
void escribirBloquesArchivo(void* path, int offset, int size, char* buffer);
char* leerArchivo(void* path);

void escucharKERNEL(void*);

#endif

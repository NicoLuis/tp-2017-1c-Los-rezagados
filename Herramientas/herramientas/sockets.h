#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>

int conectarAServidor(char* ipServidor, int puertoServidor);

int crearSocketDeEscucha(int puerto);

int aceptarCliente(int socketEscucha);

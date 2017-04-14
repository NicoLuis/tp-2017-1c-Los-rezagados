#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>

int conectarAServidor(char* ipServidor, int puertoServidor);

int crearSocketDeEscucha(int puerto);

int aceptarCliente(int socketEscucha);

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

}

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

		if (msgRecibido->tipoMensaje == 0) {
			fprintf(stderr, "El Kernel %d se ha desconectado \n", socketKernel);
			pthread_exit(NULL);
		}

		fprintf(stderr, "tipoMensaje %d\n", msgRecibido->tipoMensaje);
		fprintf(stderr, "longitud %d\n", msgRecibido->longitud);
		fprintf(stderr, "texto %s\n", (char*) msgRecibido->data);

	}
}

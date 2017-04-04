/*
 * memoria.c
 *
 *  Created on: 3/4/2017
 *      Author: utnso
 */


#include "libreriaMemoria.h"




int main(int argc, char* argv[]) {


	//Archivo de configuracion

	if (argc == 1) {
		printf("Debe ingresar la ruta del archivo en LC");
		return -1;
	}

	if (argc != 2) {
		printf("Numero incorrecto de argumentos");
		return -2;
	}


	//Cargo archivo de configuracion

	t_config* configuracion = config_create(argv[1]);
	puertoMemoria = config_get_int_value(configuracion, "PUERTO_MEMORIA");
	cantidadDeMarcos = config_get_int_value(configuracion, "MARCOS");
	tamanioDeMarcos = config_get_int_value(configuracion, "MARCO_SIZE");
	cantidadaEntradasCache = config_get_int_value(configuracion, "ENTRADAS_CACHE");
	cachePorProceso = config_get_int_value(configuracion, "CACHE_X_PROC");
	algoritmoReemplazo = config_get_string_value(configuracion, "REEMPLAZO_CACHE");
	retardoMemoria = config_get_int_value(configuracion, "RETARDO_MEMORIA");
	puertoCPU = config_get_int_value(configuracion, "PUERTO_CPU");
	puertoKernel = config_get_int_value(configuracion, "PUERTO_KERNEL");
	ipKernel = config_get_string_value(configuracion, "IP_KERNEL");

	//Muestro archivo de configuracion

	mostrarArchivoConfig();

	return 0;

}

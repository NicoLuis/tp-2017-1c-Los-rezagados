#ifndef HEXDUMP_H
#define HEXDUMP_H

#include <stdio.h>
#include <ctype.h>
#include "libreriaMemoria.h"

void hexdump(void *mem, unsigned int len, FILE* archi);

#endif /* HEXDUMP_H_ */

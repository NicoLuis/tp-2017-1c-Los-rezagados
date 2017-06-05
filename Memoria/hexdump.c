#include "hexdump.h"

#ifndef HEXDUMP_COLS
#define HEXDUMP_COLS 16
#endif

void hexdump(void *mem, unsigned int len,FILE*archi)
{
        unsigned int i, j;

        for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++)
        {
                /* print offset */
                if(i % HEXDUMP_COLS == 0)
                {
                        fprintf(archi,"0x%06x: ",i);
                        printf("0x%06x: ",i);
                }

                /* print hex data */
                if(i < len)
                {
                        fprintf(archi,"%02x ",0xFF & ((char*)mem)[i]);
                        printf("%02x ",0xFF & ((char*)mem)[i]);
                }
                else /* end of block, just aligning for ASCII dump */
                {
                        fprintf(archi,"   ");
                        printf("   ");
                }

                /* print ASCII dump */
                if(i % HEXDUMP_COLS == (HEXDUMP_COLS - 1))
                {
                        for(j = i - (HEXDUMP_COLS - 1); j <= i; j++)
                        {
                                if(j >= len) /* end of block, not really printing */
                                {
                                        fputc(' ',archi);
                                        putchar(' ');
                                }
                                else if(isprint(((char*)mem)[j])) /* printable char */
                                {
                                        fputc(0xFF & ((char*)mem)[j],archi);
                                        putchar(0xFF & ((char*)mem)[j]);
                                }
                                else /* other char */
                                {
                                        fputc('.',archi);
                                        putchar('.');
                                }
                        }
                        fputc('\n',archi);
                        putchar('\n');
                }
        }
}


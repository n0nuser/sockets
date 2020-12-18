#include <locale.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

#define BUFFERSIZE 1024

void printChars(char buf[])
{
    setlocale(LC_ALL, "C");
    int i = 0;
    while (i < strlen(buf))
    {
        unsigned int c = (unsigned int)(unsigned char)buf[i]; // (2)

        if (isprint(c) && c != '\\')
            putchar(c);
        else
            printf("\\x%02x", c);
        i++;
    }
}

void list(char *content, char *ficheroGroup, FILE *g)
{
    char buffer[BUFFERSIZE];
    strcpy(content, "215 listado de los grupos en formato \"nombre ultimo primero fecha descripcion\"\n");
    if (NULL == (g = (fopen(ficheroGroup, "r"))))
    {
        fprintf(stderr, "Error en la apertura del fichero %s\n", ficheroGroup);
        exit(2);
    }
    while (fgets(buffer, BUFFERSIZE, g) != NULL)
    {
        strcat(content, buffer);
    }
}
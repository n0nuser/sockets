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

int group(char *content, char *ficheroGroup, FILE *g, char *grupo)
{
    char buffer[BUFFERSIZE], ultimo[BUFFERSIZE] = "", primero[BUFFERSIZE] = "", total[BUFFERSIZE] = "";
    char *aux, *s;
    char trozos[3][BUFFERSIZE];
    int q = 0;
    strcpy(content, "211 ");
    printf("\e[94mFICHEROGROUP: %s\e[0m\n", ficheroGroup);
    if (NULL == (g = (fopen(ficheroGroup, "r"))))
    {
        fprintf(stderr, "Error en la apertura del fichero %s\n", ficheroGroup);
        exit(2);
    }

    //Se lee el fichero y se mete en el vector
    while (fgets(buffer, BUFFERSIZE, g) != NULL)
    {
        //Extrae el primer token de la línea
        aux = strtok(buffer, " ");
        printf("\e[95mAUX   : \"%s\"\e[0m\n", aux);
        printf("\e[94mGRUPO : \"%s\"\e[0m\n", grupo);

        //Si son iguales
        if (strcmp(aux, grupo) == 0)
        {
            while (aux != NULL)
            {
                strcpy(trozos[q], aux);
                // Seguimos con el siguiente token
                aux = strtok(NULL, " ");
                q++;
            }

            s = trozos[1];
            while (*s && *s == '0')
                s++;
            strcpy(ultimo, s);

            s = trozos[2];
            while (*s && *s == '0')
                s++;
            strcpy(primero, s);

            sprintf(total, "%d", atoi(ultimo) - atoi(primero) + 1);
            strcat(content, total);
            strcat(content, " ");
            strcat(content, primero);
            strcat(content, " ");
            strcat(content, ultimo);
            strcat(content, " ");
            strcat(content, grupo);

            //Concatena el contenido de la línea al 211 y se pone en "content"
            printf("\e[95mCONTENT: %s\e[0m\n", content);
            return 1;
        }
    }
    return 0;
}

int article(char *content, char *pathArticulos, FILE *a, char *articulo, char *grupo)
{
    char *aux, tempGrupo[100], pathGrupo[BUFFERSIZE], buffer[BUFFERSIZE];
    strcpy(tempGrupo, grupo);
    aux = strtok(tempGrupo, ".");
    aux = strtok(NULL, " ");
    strcpy(tempGrupo, aux);
    printf("\e[93mGRUPO %s\e[0m\n", tempGrupo);
    
    strcpy(pathGrupo, pathArticulos);
    strcat(pathGrupo, tempGrupo);
    strcat(pathGrupo, "/");
    //RESULTADO PATH FICHERO
    strcat(pathGrupo, articulo);
    strtok(pathGrupo,"\r\n");

    printf("\e[93mPATH.FICHERO %s\e[0m\n", pathGrupo);
    if (NULL == (a = (fopen(pathGrupo, "r"))))
    {
        printf("\e[94mError en la apertura del fichero %s\e[0m\n", pathGrupo);
        return 0;
    }
    while (fgets(buffer, BUFFERSIZE, a) != NULL)
    {
        strcat(content, buffer);
    }
    return 1;
}
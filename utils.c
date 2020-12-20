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

void group (char *content, char * ficheroGroup, FILE *g) 
{
    char buffer[BUFFERSIZE];
    char ultimo[BUFFERSIZE] ="";
    char primero[BUFFERSIZE]="";
    char total[BUFFERSIZE]="";
    char trozos[20][BUFFERSIZE];
    char lineas[20][BUFFERSIZE];
    char *aux, *s;
    int q = 0, l = 0, contLineas = 0;
    strcpy(content, "211 ");
    if (NULL == (g = (fopen(ficheroGroup, "r"))))
    {
        fprintf(stderr, "Error en la apertura del fichero %s\n", ficheroGroup);
        exit(2);
    }
     
    for(l = 0; l < 20; l++) {
        strcpy(lineas[l],"");
    }
    l = 0; 
    
    while (fgets(buffer, BUFFERSIZE, g) != NULL)
    {
        strcpy(lineas[l], buffer);
        l++;
        contLineas++;       
    }

    for(l = 0; l < contLineas; l++) {

        for(int i = 0; i < 20; i++) {
        strcpy(trozos[i],"");
        } 
        //Separamos la linea leida en tokens delimitados por espacios
        aux = strtok(lineas[l], " ");   
        while(aux != NULL){
            strcpy(trozos[q], aux);
            // Seguimos con el siguiente token
            aux = strtok(NULL, " ");
            q++;                 
        }                   

    if(strcmp(trozos[0],"local.redes") == 0) {
            s = trozos[1];
            while (*s && *s == '0') s++;
            strcpy(ultimo, s);

            s = trozos[2];
            while (*s && *s == '0') s++;
            strcpy(primero, s);

            sprintf(total,"%d",atoi(ultimo)-atoi(primero)+1);
            strcat(content,total);
            strcat(content," ");
            strcat(content,primero);
            strcat(content," ");
            strcat(content,ultimo);
            strcat(content," ");
            strcat(content,trozos[0]);
        } else if (strcmp(trozos[0],"local.deportes") == 0){
            s = trozos[1];
            while (*s && *s == '0') s++;
            strcpy(ultimo, s);

            s = trozos[2];
            while (*s && *s == '0') s++;
            strcpy(primero, s);

            sprintf(total,"%d",atoi(ultimo)-atoi(primero)+1);
            strcat(content,total);
            strcat(content," ");
            strcat(content,primero);
            strcat(content," ");
            strcat(content,ultimo);
            strcat(content," ");
            strcat(content,trozos[0]);
        }
    }   
    
}
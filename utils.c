/*
** Fichero: servidor.c
** Autores:
** Sergio García González
** Pablo Jesús González Rubio
*/

#include <locale.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
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
    fclose(g);
}

int group(char *content, char *ficheroGroup, FILE *g, char *grupo)
{
    char buffer[BUFFERSIZE], ultimo[BUFFERSIZE] = "", primero[BUFFERSIZE] = "", total[BUFFERSIZE] = "";
    char *aux, *s;
    char trozos[3][BUFFERSIZE];
    int q = 0;
    strcpy(content, "211 ");
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
            strcat(content, "\r\n");
            return 1;
        }
    }
    fclose(g);
    return 0;
}

int article(char *content, char *pathArticulos, FILE *a, char *articulo, char *grupo)
{
    char *aux, tempGrupo[100], pathGrupo[BUFFERSIZE], buffer[BUFFERSIZE];
    strcpy(tempGrupo, grupo);
    aux = strtok(tempGrupo, ".");
    aux = strtok(NULL, " ");
    strcpy(tempGrupo, aux);

    strcpy(pathGrupo, pathArticulos);
    strcat(pathGrupo, tempGrupo);
    strcat(pathGrupo, "/");
    strcat(pathGrupo, articulo);
    strtok(pathGrupo, "\r\n");

    if (NULL == (a = (fopen(pathGrupo, "r"))))
    {
        fprintf(stderr, "Error en la apertura del fichero %s\n", pathGrupo);
        return 0;
    }
    strcpy(content, "");
    while (fgets(buffer, BUFFERSIZE, a) != NULL)
    {
        strcat(content, buffer);
    }
    fclose(a);
    return 1;
}

int head(char *content, char *pathArticulos, FILE *a, char *articulo, char *grupo)
{
    char *aux, tempGrupo[100], pathGrupo[BUFFERSIZE], buffer[BUFFERSIZE], temp[BUFFERSIZE];
    strcpy(tempGrupo, grupo);
    aux = strtok(tempGrupo, ".");
    aux = strtok(NULL, " ");
    strcpy(tempGrupo, aux);

    strcpy(pathGrupo, pathArticulos);
    strcat(pathGrupo, tempGrupo);
    strcat(pathGrupo, "/");
    strcat(pathGrupo, articulo);
    strtok(pathGrupo, "\r\n");

    if (NULL == (a = (fopen(pathGrupo, "r"))))
    {
        fprintf(stderr, "Error en la apertura del fichero %s\n", pathGrupo);
        return 0;
    }
    strcpy(content, "");
    while (fgets(buffer, BUFFERSIZE, a) != NULL)
    {
        strcpy(temp, buffer);
        aux = strtok(temp, " ");
        if (strcmp(aux, "Message-ID:") == 0)
        {
            strcat(content, buffer);
            break;
        }
        strcat(content, buffer);
    }
    fclose(a);
    return 1;
}

int body(char *content, char *pathArticulos, FILE *a, char *articulo, char *grupo)
{
    char *aux, tempGrupo[100], pathGrupo[BUFFERSIZE], buffer[BUFFERSIZE], temp[BUFFERSIZE];
    strcpy(tempGrupo, grupo);
    aux = strtok(tempGrupo, ".");
    aux = strtok(NULL, " ");
    strcpy(tempGrupo, aux);

    strcpy(pathGrupo, pathArticulos);
    strcat(pathGrupo, tempGrupo);
    strcat(pathGrupo, "/");
    strcat(pathGrupo, articulo);
    strtok(pathGrupo, "\r\n");

    if (NULL == (a = (fopen(pathGrupo, "r"))))
    {
        fprintf(stderr, "Error en la apertura del fichero %s\n", pathGrupo);
        return 0;
    }
    strcpy(content, "");
    while (fgets(buffer, BUFFERSIZE, a) != NULL)
    {
        strcpy(temp, buffer);
        aux = strtok(temp, " ");
        if (strcmp(aux, "Message-ID:") == 0 || strcmp(aux, "Date:") == 0 || strcmp(aux, "Subject:") == 0 || strcmp(aux, "Newsgroups:") == 0)
            continue;
        strcat(content, buffer);
    }
    fclose(a);
    return 1;
}

void newgroups(char *content, char *ficheroGroup, FILE *g, char *date, char *time)
{
    char buffer[BUFFERSIZE], grupo[BUFFERSIZE] = "", fecha[BUFFERSIZE] = "", hora[BUFFERSIZE] = "";
    char *aux;
    char trozos[3][BUFFERSIZE];
    int i = 0;
    strcpy(content, "231 List of new newsgroups follows (multi-line):\n");
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
        i = 1;
        strcpy(grupo, aux);
        while (aux != NULL)
        {
            aux = strtok(NULL, " ");
            if (i == 3)
            {
                strcpy(fecha, aux);
            }
            else if (i == 4)
            {
                strcpy(hora, aux);
                break;
            }
            i++;
        }
        //Si es posterior la fecha del grupo a la especificada
        if (atoi(fecha) > atoi(date))
        {
            strcat(content, grupo);
            strcat(content, "\n");
        } // Si son iguales se compara la hora
        else if (atoi(fecha) == atoi(date))
        {
            //Si la hora es posterior se añade
            if (atoi(hora) > atoi(time))
            {
                strcat(content, grupo);
                strcat(content, "\n");
            } //Si coincide igual
            else if (atoi(hora) == atoi(time))
            {
                strcat(content, grupo);
                strcat(content, "\n");
            } //Si no, se rechaza
            else
            {
                continue;
            }
        } //Si no, se rechaza
        else
        {
            continue;
        }
    }
    fclose(g);
    strcat(content, ".\n\n");
}

void newnews(char *content, char *ficheroGroup, char *pathArticulos, FILE *g, char *grupo, char *date, char *time)
{
    char buffer[BUFFERSIZE], tempBuffer[BUFFERSIZE], fecha[BUFFERSIZE] = "", hora[BUFFERSIZE] = "";
    char subject[BUFFERSIZE], messageId[BUFFERSIZE];
    char primero[10], ultimo[10];
    char tempGrupo[BUFFERSIZE], pathGrupo[BUFFERSIZE];
    char *aux, *s;
    char trozos[3][BUFFERSIZE];
    int i = 0, j = 0, r = 0;
    int num_articulos_total = 0;
    char num_articulo[10];
    int recuento = 0; //Nº de artículos que coinciden con el criterio

    strcpy(content, "230 list follows since ");
    strcat(content, time);
    strcat(content, " ");
    strcat(content, date);
    strcat(content, "\n");

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
        //Si son iguales
        if (strcmp(aux, grupo) == 0)
        {
            aux = strtok(NULL, " ");
            s = aux;
            while (*s && *s == '0')
                s++;
            strcpy(ultimo, s);
            aux = strtok(NULL, " ");
            s = aux;
            while (*s && *s == '0')
                s++;
            strcpy(primero, s);
            num_articulos_total = atoi(ultimo) - atoi(primero) + 1;
        }
    }
    fclose(g);

    strcpy(tempGrupo, grupo);         // Lo ponemos en uno temporal para que no modifique el original
    aux = strtok(tempGrupo, ".");     // Aqui coge Local
    aux = strtok(NULL, " ");          // Aqui coge redes/deporte
    strcpy(tempGrupo, aux);           // Guardamos redes/deporte
    strcpy(pathGrupo, pathArticulos); // Le metemos a pathGrupo la dirección de los articulos
    strcat(pathGrupo, tempGrupo);
    strcat(pathGrupo, "/");

    for (i = 1; i <= num_articulos_total; i++)
    {
        strcpy(tempGrupo, pathGrupo);
        sprintf(num_articulo, "%d", i);
        strcat(tempGrupo, num_articulo);

        //Reseteo de las variables
        j = 0;
        strcpy(subject, "");
        strcpy(fecha, "");
        strcpy(hora, "");
        strcpy(messageId, "");

        if (NULL == (g = (fopen(tempGrupo, "r"))))
        {
            fprintf(stderr, "Error en la apertura del fichero %s\n", tempGrupo);
            exit(2);
        }
        while (fgets(buffer, BUFFERSIZE, g) != NULL)
        {
            aux = strtok(buffer, " ");
            if (strcmp(aux, "Subject:") == 0)
            {
                aux = strtok(NULL, " ");
                while (aux != NULL)
                {
                    strcat(subject, aux);
                    strcat(subject, " ");
                    aux = strtok(NULL, " ");
                }
                strtok(subject, "\r\n");
                j++;
            }
            else if (strcmp(aux, "Date:") == 0)
            {
                aux = strtok(NULL, " ");
                strcpy(fecha, aux);
                aux = strtok(NULL, " ");
                strcpy(hora, aux);
                j++;
            }
            else if (strcmp(aux, "Message-ID:") == 0)
            {
                aux = strtok(NULL, " ");
                strcpy(messageId, aux);
                strtok(messageId, "\r\n");
                j++;
            }
            if (j == 3)
            {
                break;
            }
        }
        fclose(g);

        //Si es posterior la fecha del grupo a la especificada
        if (atoi(fecha) > atoi(date))
        {
            strcat(content, num_articulo);
            strcat(content, " ");
            strcat(content, messageId);
            strcat(content, " ");
            strcat(content, subject);
            strcat(content, "\n");
        } // Si son iguales se compara la hora
        else if (atoi(fecha) == atoi(date))
        {
            //Si la hora es posterior se añade
            if (atoi(hora) > atoi(time))
            {
                strcat(content, num_articulo);
                strcat(content, " ");
                strcat(content, messageId);
                strcat(content, " ");
                strcat(content, subject);
                strcat(content, "\n");
            } //Si coincide igual
            else if (atoi(hora) == atoi(time))
            {
                strcat(content, num_articulo);
                strcat(content, " ");
                strcat(content, messageId);
                strcat(content, " ");
                strcat(content, subject);
                strcat(content, "\n");
            } //Si no, se rechaza
            else
            {
                continue;
            }
        } //Si no, se rechaza
        else
        {
            continue;
        }
        memset(tempGrupo, 0, BUFFERSIZE);
    }
    strcat(content, ".\n");
}

int post(int socket, char *mensajeOriginal, char *ficheroGroup, char *pathArticulos, FILE *g)
{
    char mensajeTotal[BUFFERSIZE * 3] = "";
    char mensaje[BUFFERSIZE];
    char tempMensaje[BUFFERSIZE];
    char buffer[BUFFERSIZE];
    char grupo[BUFFERSIZE], grupoConcreto[BUFFERSIZE];
    char aGuardarFGrupo[BUFFERSIZE * 5], aGuardarFGrupoLineaMod[BUFFERSIZE], aGuardarTemp[BUFFERSIZE];
    char pathArticulo[BUFFERSIZE];
    char numUltimoArticuloStringCortado[5], numUltimoArticuloString[50];
    int numUltimoArticulo = 0;
    int len, lenArticuloCortado, lenArticulo, flagLeido = 1, grupoExiste = 0;
    char *aux, *lineaActual, *mensajeTemp, *s, *m;

    //COGE LA LINEA ACTUAL DE UN STRING
    strcpy(mensaje, mensajeOriginal);
    lineaActual = strtok_r(mensaje, "\r\n", &mensajeTemp);
    lineaActual = strtok_r(NULL, "\r\n", &mensajeTemp);

    do
    {
        strcat(lineaActual, "\r\n");

        if (strcmp(lineaActual, ".\r\n") == 0)
            break;

        if (flagLeido) //Una vez lea el newsgroups no lo leera mas
        {
            strcpy(tempMensaje, lineaActual);
            aux = strtok(tempMensaje, " ");
            strcpy(grupo, "");
            if (strcmp(aux, "Newsgroups:") == 0)
            {
                aux = strtok(NULL, " ");
                strcat(grupo, aux);
                strtok(grupo, "\r\n");
                flagLeido = 0;
            }
            if (strcmp(grupo, "") != 0)
            {
                if (NULL == (g = (fopen(ficheroGroup, "r"))))
                {
                    fprintf(stderr, "Error en la apertura del fichero %s\n", ficheroGroup);
                    exit(2);
                }
                strcpy(aGuardarFGrupo, "");
                while (fgets(buffer, BUFFERSIZE, g) != NULL)
                {
                    strcpy(aGuardarTemp, buffer);
                    aux = strtok(buffer, " ");
                    if (strcmp(aux, grupo) == 0)
                    {
                        grupoExiste = 1;
                        aux = strtok(NULL, " ");
                        lenArticulo = strlen(aux);
                        s = aux;
                        while (*s && *s == '0')
                            s++;

                        lenArticuloCortado = strlen(s);
                        numUltimoArticulo = atoi(s);
                        numUltimoArticulo++;
                        //Añadir tantos 0's como la diferencia entre lenArticulo y lenArticuloCortado
                        strcpy(numUltimoArticuloString, "0");
                        for (int i = 0; i < (lenArticulo - lenArticuloCortado) - 1; i++)
                        {
                            sprintf(numUltimoArticuloString, "0%s", numUltimoArticuloString);
                        }
                        sprintf(numUltimoArticuloString, "%s%d", numUltimoArticuloString, numUltimoArticulo);
                        strcpy(aGuardarFGrupoLineaMod, grupo);
                        strcat(aGuardarFGrupoLineaMod, " ");
                        strcat(aGuardarFGrupoLineaMod, numUltimoArticuloString);
                        while (aux != NULL)
                        {
                            aux = strtok(NULL, " ");
                            if (aux != NULL)
                            {
                                strcat(aGuardarFGrupoLineaMod, " ");
                                strcat(aGuardarFGrupoLineaMod, aux);
                            }
                        }
                        strcpy(grupoConcreto, grupo); //Así por un lado local.redes y por otro redes
                        aux = strtok(grupoConcreto, ".");
                        aux = strtok(NULL, " ");
                        strcpy(grupoConcreto, aux);
                        strcpy(pathArticulo, pathArticulos);
                        strtok(pathArticulo, "\r\n");
                        strcat(pathArticulo, grupoConcreto);
                        strcat(pathArticulo, "/");
                        sprintf(numUltimoArticuloStringCortado, "%d", numUltimoArticulo);
                        strcat(pathArticulo, numUltimoArticuloStringCortado);
                        strcat(aGuardarFGrupo, aGuardarFGrupoLineaMod);
                    }
                    else
                        strcat(aGuardarFGrupo, aGuardarTemp);
                }
                fclose(g);
            }
        }
        memset(lineaActual, 0, sizeof lineaActual);
        memset(tempMensaje, 0, sizeof tempMensaje);
        memset(grupo, 0, sizeof grupo);

    } while ((lineaActual = strtok_r(NULL, "\r\n", &mensajeTemp)) != NULL);

    if (grupoExiste)
    {
        if ((g = fopen(ficheroGroup, "w")) == NULL)
            fprintf(stderr, "File could not be opened %s", ficheroGroup);

        fprintf(g, "%s", aGuardarFGrupo);
        fclose(g);

        //QUITA TODAS LAS LINEAS HASTA QUE ENCUENTRA
        // LA N MAYUSCULA DE NEWSGROUP
        strcpy(mensaje, mensajeOriginal);
        m = mensaje;
        while (*m && *m != '\n')
            m++;
        m++;
        strcpy(mensajeTotal, m);
        ////////////////////////////////////////////

        if ((g = fopen(pathArticulo, "w")) == NULL)
        {
            fprintf(stderr, "File could not be opened %s", pathArticulo);
        }
        fprintf(g, "%s", mensajeTotal);
        fclose(g);
    }
    return grupoExiste;
}

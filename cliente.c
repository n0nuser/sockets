/*
** Fichero: cliente.c
** Autores:
** Sergio Garcia Gonzalez
** Pablo Jesus Gonzalez Rubio
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "utils.h"

extern int errno;

#define PUERTO 4492     //Puerto
#define BUFFERSIZE 1024 //Tamaño del buffer

#define RETRIES 5 //Intentos de recepcion de mensajes
#define TIMEOUT 6 //Timeout de la señal
#define MAXHOST 512

typedef struct
{
    char respuesta[100];
    char server[100];
    char connection[100];
    char final[10];
} respInfo;

/*PROTOTIPOS DE FUNCIONES*/
void TCP(FILE *, int, char **);
void UDP(FILE *, int, char **);
void handler();

int main(int argc, char *argv[])
{
    FILE *f;

    /*Informa del uso si los argumentos pasados al programa son erroneos*/
    if (argc != 4 || (strcmp(argv[2], "UDP") && strcmp(argv[2], "TCP")))
    {
        fprintf(stderr, "Argumentos erroneos, USO:\n");
        fprintf(stderr, "%s [Nombre servidor] [TCP | UDP] [Fichero de ordenes]\n", argv[0]);
        exit(1);
    }
    /*Si el fichero es erroneo..*/
    if (NULL == (f = (fopen(argv[3], "r"))))
    {
        fprintf(stderr, "Error en la apertura del fichero %s\n", argv[3]);
        exit(2);
    }
    /*Distinguimos entre si es TCP o UDP*/
    if (strcmp(argv[2], "TCP") == 0)
        TCP(f, argc, argv);
    else
        UDP(f, argc, argv);
    fclose(f);
    return 0;
}

void TCP(FILE *f, int argc, char *argv[])
{

    int s; // Socket
    struct addrinfo hints, *res;
    long timevar;                   // Tiene la fecha y la hora que devuelve time()
    struct sockaddr_in myaddr_in;   // Guarda al direccion local
    struct sockaddr_in servaddr_in; // Guarda la direccion del servidor
    int addrlen, len, i, j, errcode;
    int flagPost = 0;

    char puertoEfimero[100];    // Nombre del fichero que guarda el progreso y la depuracion del cliente
    char respuesta[BUFFERSIZE]; //String para la respuesta del servidor
    char buf[BUFFERSIZE * 5];   // Contiene lo leido en el fichero linea a linea
    char tempBuf[BUFFERSIZE * 5];
    char caracteresRetorno[] = "\r\n";
    FILE *c, *g;

    /* Create the socket. */
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1)
    {
        perror(argv[0]);
        if (NULL == (g = (fopen(puertoEfimero, "a"))))
            fprintf(stderr, "No se ha podido abrir el fichero");
        fprintf(g, "%s: unable to create socket\n", argv[0]);
        fclose(g);
        exit(1);
    }

    // Inicializa las estructuras de la informacion de cliente y servidor
    memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
    memset((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

    // Pone a punto la direccion a la que se conectara
    servaddr_in.sin_family = AF_INET;

    // Get the host information for the hostname that the user passed in.
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    // Esta función es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta
    errcode = getaddrinfo(argv[1], NULL, &hints, &res);
    if (errcode != 0)
    {
        if (NULL == (g = (fopen(puertoEfimero, "a"))))
            fprintf(stderr, "No se ha podido abrir el fichero");
        fprintf(g, "%s: Couldn't resolve IP for %s\n", argv[0], argv[1]);
        fclose(g);
        exit(1);
    }
    else
    {
        // Copia la direccion del host
        servaddr_in.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
    }
    freeaddrinfo(res);

    // Puerto del servidor
    servaddr_in.sin_port = htons(PUERTO);

    // Intenta establecer la conexion con la IP que ha conseguido del servidor.
    if (connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1)
    {
        perror(argv[0]);
        if (NULL == (g = (fopen(puertoEfimero, "a"))))
            fprintf(stderr, "No se ha podido abrir el fichero");
        fprintf(g, "%s: unable to connect to remote server.\n", argv[0]);
        fclose(g);
        exit(1);
    }
    /* Since the connect call assigns a free address
         * to the local end of this connection, let's use
         * getsockname to see what it assigned.  Note that
         * addrlen needs to be passed in as a pointer,
         * because getsockname returns the actual length
         * of the address.
         */
    addrlen = sizeof(struct sockaddr_in);
    if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1)
    {
        perror(argv[0]);
        if (NULL == (g = (fopen(puertoEfimero, "a"))))
            fprintf(stderr, "No se ha podido abrir el fichero");
        fprintf(g, "%s: unable to read socket address.\n", argv[0]);
        fclose(g);
        exit(1);
    }

    /* Print out a startup message for the user. */
    time(&timevar);
    /* The port number must be converted first to host byte
         * order before printing.  On most hosts, this is not
         * necessary, but the ntohs() call is included here so
         * that this program could easily be ported to a host
         * that does require it.
         */

    printf("[C] Connected to %s on port %u at %s\n", argv[1], ntohs(myaddr_in.sin_port), (char *)ctime(&timevar));

    // PATH DEL FICHERO PUERTO_EFIMERO DE CADA CLIENTE
    strcpy(puertoEfimero, "");
    sprintf(puertoEfimero, "%u", ntohs(myaddr_in.sin_port));
    strcat(puertoEfimero, "_TCP.txt");

    //RECIBE EL ESTABLECIMIENTO DE CONEXIÓN DEL SERVIDOR (200 PREPARADO)
    if (-1 == (recv(s, respuesta, BUFFERSIZE, 0)))
    {
        if (NULL == (g = (fopen(puertoEfimero, "a"))))
            fprintf(stderr, "No se ha podido abrir el fichero");
        fprintf(g, "%s: error reading result\n", argv[0]);
        fclose(g);
        exit(1);
    }
    printf("%s\n", respuesta);
    memset(respuesta, 0, BUFFERSIZE);

    // Limpiamos el buffer antes de que empieze a leer
    strcpy(buf, "");
    strcpy(tempBuf, "");

    // COMENZAMOS A LEER EL FICHERO DE ORDENES
    while (fgets(buf, BUFFERSIZE, f) != NULL)
    {
        //Si lee una linea vacia la omite
        if (strcmp(buf, caracteresRetorno) == 0)
            continue;

        //Si lee POST entra en un estado condicional que mantiene leyendo el fichero
        if (strcmp(buf, "POST\r\n") == 0)
            flagPost = 1;

        //Y guardandolo en tempbuf
        if (flagPost)
            strcat(tempBuf, buf);
        //Hasta que lee el punto, entonces flagPost = 0 y lo manda todo de golpe
        if (flagPost == 1 && strcmp(buf, ".\r\n") == 0)
        {
            flagPost = 0;
            strcpy(buf, tempBuf);
        }

        if (flagPost == 0)
        {
            // Guardamos el mensaje de progreso en el fichero de puerto efimero
            if (NULL == (g = (fopen(puertoEfimero, "a"))))
                fprintf(stderr, "No se ha podido abrir el fichero");
            fputs(buf, g);
            fclose(g);

            sleep(1);

            //////////////////////////
            // ENVIO DE LA PETICION //
            //////////////////////////

            len = send(s, buf, strlen(buf), 0);
            if (len != strlen(buf))
            {
                if (NULL == (g = (fopen(puertoEfimero, "a"))))
                    fprintf(stderr, "No se ha podido abrir el fichero");
                fprintf(g, "%s: Connection aborted on error\nMessage was: %s\nLength returned by send() was: %d\n", argv[0], buf, len);
                fclose(g);
                exit(1);
            }
            strcpy(tempBuf, ""); //Se resetea el buffer de lineas

            /////////////////////////////////
            // RECEPCION DESDE EL SERVIDOR //
            /////////////////////////////////

            if (-1 == (recv(s, respuesta, BUFFERSIZE, 0)))
            {
                if (NULL == (g = (fopen(puertoEfimero, "a"))))
                    fprintf(stderr, "No se ha podido abrir el fichero");
                fprintf(g, "%s: error reading result\n", argv[0]);
                fclose(g);
                exit(1);
            }

            // Guardamos el mensaje de progreso en el fichero de puerto efimero
            if (NULL == (g = (fopen(puertoEfimero, "a"))))
                fprintf(stderr, "No se ha podido abrir el fichero");
            fputs(respuesta, g);
            fputs("\n", g);
            fclose(g);
            printf("%s\n", respuesta);

            //Una vez operado ya todo lo necesario con respuesta
            //se reinicializa para no dar problemas con caracteres raros
            memset(respuesta, 0, BUFFERSIZE);
        }
    }
    /* Now, shutdown the connection for further sends.
         * This will cause the server to receive an end-of-file
         * condition after it has received all the requests that
         * have just been sent, indicating that we will not be
         * sending any further requests.
         */
    if (shutdown(s, 1) == -1)
    {
        perror(argv[0]);
        if (NULL == (g = (fopen(puertoEfimero, "a"))))
            fprintf(stderr, "No se ha podido abrir el fichero");
        fprintf(g, "%s: unable to shutdown socket\n", argv[0]);
        fclose(g);
        exit(1);
    }

    // Print message indicating completion of task.
    time(&timevar);
    printf("All done at %s", (char *)ctime(&timevar));
}

void UDP(FILE *f, int argc, char *argv[])
{
    int s; // Socket
    int i, errcode, addrlen, n_retry, len, flagPost = 0, q = 0, p = 0;
    int retry = RETRIES;            /* holds the retry count */
    long timevar;                   // Tiene la fecha y la hora que devuelve time()
    struct sockaddr_in myaddr_in;   // Guarda al direccion local
    struct sockaddr_in servaddr_in; // Guarda la direccion del servidor
    struct in_addr reqaddr;         /* for returned internet address */
    struct addrinfo hints, *res;
    struct sigaction vec;
    char hostname[MAXHOST];
    char puertoEfimero[100];    // Nombre del fichero que guarda el progreso y la depuracion del cliente
    char respuesta[BUFFERSIZE]; //String para la respuesta del servidor
    char buf[BUFFERSIZE * 5];   // Contiene lo leido en el fichero linea a linea
    char tempBuf[BUFFERSIZE * 5];
    char caracteresRetorno[] = "\r\n";
    char envio[BUFFERSIZE]; //String para el envio al servidor
    char conexionRed[] = "NNTP";
    char *corta; //Puntero que apuntará a cada parte de la linea para separar
    char vect[3][100];
    char aux[7];
    FILE *c, *g;

    /* Create the socket. */
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == -1)
    {
        perror(argv[0]);
        if (NULL == (g = (fopen(puertoEfimero, "a"))))
            fprintf(stderr, "No se ha podido abrir el fichero");
        fprintf(g, "%s: unable to create socket\n", argv[0]);
        fclose(g);
        exit(1);
    }

    // Inicializa las estructuras de la informacion de cliente y servidor
    memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
    memset((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

    /* Bind socket to some local address so that the
         * server can send the reply back.  A port number
         * of zero will be used so that the system will
         * assign any available port number.  An address
         * of INADDR_ANY will be used so we do not have to
         * look up the internet address of the local host.
         */
    myaddr_in.sin_family = AF_INET;
    myaddr_in.sin_port = 0;
    myaddr_in.sin_addr.s_addr = INADDR_ANY;
    if (bind(s, (const struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1)
    {
        perror(argv[0]);
        if (NULL == (g = (fopen(puertoEfimero, "a"))))
            fprintf(stderr, "No se ha podido abrir el fichero");
        fprintf(g, "%s: unable to bind socket\n", argv[0]);
        fclose(g);
        exit(1);
    }
    addrlen = sizeof(struct sockaddr_in);
    if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1)
    {
        perror(argv[0]);
        if (NULL == (g = (fopen(puertoEfimero, "a"))))
            fprintf(stderr, "No se ha podido abrir el fichero");
        fprintf(g, "%s: unable to read socket address\n", argv[0]);
        fclose(g);
        exit(1);
    }

    /* Print out a startup message for the user. */
    time(&timevar);
    /* The port number must be converted first to host byte
     * order before printing.  On most hosts, this is not
     * necessary, but the ntohs() call is included here so
     * that this program could easily be ported to a host
     * that does require it.
     */
    printf("Connected to %s on port %u at %s", argv[1], ntohs(myaddr_in.sin_port), (char *)ctime(&timevar));
    strcpy(puertoEfimero, "");
    sprintf(puertoEfimero, "%u", ntohs(myaddr_in.sin_port));
    strcat(puertoEfimero, "_UDP.txt");

    // Pone a punto la direccion a la que se conectara
    servaddr_in.sin_family = AF_INET;

    // Get the host information for the hostname that the user passed in.
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    // Esta función es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta
    errcode = getaddrinfo(argv[1], NULL, &hints, &res);
    if (errcode != 0)
    {
        // Name was not found.  Return a special value signifying the error.
        if (NULL == (g = (fopen(puertoEfimero, "a"))))
            fprintf(stderr, "No se ha podido abrir el fichero");
        fprintf(g, "%s: Couldn't resolve IP for %s\n", argv[0], argv[1]);
        fclose(g);
        exit(1);
    }
    else
    {
        // Copia la direccion del host
        servaddr_in.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
    }
    freeaddrinfo(res);
    // Puerto del servidor
    servaddr_in.sin_port = htons(PUERTO);

    /* Registrar SIGALRM para no quedar bloqueados en los recvfrom */
    vec.sa_handler = (void *)handler;
    vec.sa_flags = 0;
    if (sigaction(SIGALRM, &vec, (struct sigaction *)0) == -1)
    {
        perror("sigaction(SIGALRM)");
        if (NULL == (g = (fopen(puertoEfimero, "a"))))
            fprintf(stderr, "No se ha podido abrir el fichero");
        fprintf(g, "%s: unable to register the SIGALRM signal.\n", argv[0]);
        fclose(g);
        exit(1);
    }
    // Limpiamos el buffer antes de que empieze a leer
    strcpy(buf, "");
    strcpy(tempBuf, "");

    // COMENZAMOS A LEER EL FICHERO DE ORDENES
    while (fgets(buf, BUFFERSIZE, f) != NULL)
    {
        //////////////////////////
        // ENVIO DE LA PETICION //
        //////////////////////////
        n_retry = RETRIES;

        //Si lee una linea vacia la omite
        if (strcmp(buf, caracteresRetorno) == 0)
            continue;

        //Si lee POST entra en un estado condicional que mantiene leyendo el fichero
        if (strcmp(buf, "POST\r\n") == 0)
            flagPost = 1;

        //Y guardandolo en tempbuf
        if (flagPost)
            strcat(tempBuf, buf);

        //Hasta que lee el punto, entonces flagPost = 0 y lo manda todo de golpe
        if (flagPost == 1 && strcmp(buf, ".\r\n") == 0)
        {
            flagPost = 0;
            strcpy(buf, tempBuf);
        }

        if (flagPost == 0)
        {
            while (n_retry > 0)
            {

                //Mete mensaje progeso
                if (NULL == (g = (fopen(puertoEfimero, "a"))))
                    fprintf(stderr, "No se ha podido abrir el fichero");
                fputs(buf, g);
                fclose(g);

                /*Enviamos con el tamaño de la estructura enviada, si no devuelve el mismo tamaño da error*/
                if (sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) != strlen(buf))
                {
                    perror(argv[0]);
                    if (NULL == (g = (fopen(puertoEfimero, "a"))))
                        fprintf(stderr, "No se ha podido abrir el fichero");
                    fprintf(g, "%s: unable to send request\n", argv[0]);
                    fclose(g);
                }
                strcpy(tempBuf, "");
                /*Establecemos una alarma para que el recvfrom no se quede en espera infinita por ser bloqueante*/
                alarm(TIMEOUT);

                /////////////////////////////////
                // RECEPCION DESDE EL SERVIDOR //
                /////////////////////////////////
                if ((len = recvfrom(s, respuesta, BUFFERSIZE, 0, (struct sockaddr *)&servaddr_in, &addrlen)) == -1)
                {
                    if (errno == EINTR)
                    {
                        // Si se produce el SIGALRM restamos un RETRY
                        printf("Attempt %d (Retries %d).\n", n_retry, RETRIES);
                        n_retry--;
                    }
                    else
                    {
                        printf("Unable to get response from.\n");
                        exit(1);
                    }
                }
                else
                {
                    alarm(0); //Cancelamos la alarma

                    //Mete mensaje progeso
                    if (NULL == (g = (fopen(puertoEfimero, "a"))))
                        fprintf(stderr, "No se ha podido abrir el fichero");
                    fputs(respuesta, g);
                    fputs("\n", g);
                    fclose(g);

                    // Mostramos la respuesta
                    printf("%s - %s", argv[1], respuesta);
                    break;
                }
            }
        }
        // Mostramos por pantalla los distintos intentos de respuesta
        if (n_retry == 0)
        {
            printf("Unable to get response from.\n");
            printf("%s after %d Attempts.\n", argv[1], RETRIES);
        }
    }

    // Ha terminado UDP
    time(&timevar);
    printf("All done at %s", (char *)ctime(&timevar));
}

// Funcion que necesita el SIGACTION
void handler()
{
    printf("La recepcion ha superado el timeout: %d\n", TIMEOUT);
}

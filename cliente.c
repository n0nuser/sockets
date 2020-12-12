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

extern int errno;

#define PUERTO 4492     //Puerto
#define TAM_BUFFER 400  //Tamaño del buffer para la lectura del fichero
#define BUFFERSIZE 1024 //Tamaño del buffer de envio y recepcion de datos

#define ADDRNOTFOUND 0xffffffff // value returned for unknown host
#define RETRIES 5               //Intentos de recepcion de mensajes
#define TIMEOUT 6               //Timeout de la señal
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
    {

        TCP(f, argc, argv);
    }
    else
    {

        UDP(f, argc, argv);
    }
    fclose(f);
    return 0;
}

void TCP(FILE *f, int argc, char *argv[])
{

    int s; /* connected socket descriptor */
    struct addrinfo hints, *res;
    long timevar;                   /* contains time returned by time() */
    struct sockaddr_in myaddr_in;   /* for local socket address */
    struct sockaddr_in servaddr_in; /* for server socket address */
    int addrlen, i, j, errcode;

    char aux[7];
    char envio[BUFFERSIZE];     //String para el envio al servidor
    char respuesta[BUFFERSIZE]; //String para la respuesta del servidor
    FILE *c;
    char buf[TAM_BUFFER]; /*Contiene lo leido en el fichero linea a linea*/
    char conexionRed[] = "NNTP";
    char caracteresRetorno[] = "\r\n";
    char *str, *corta; //Puntero que apuntará a cada parte de la linea para separar
    char vect[3][100];
    char salida[BUFFERSIZE / 2];
    int q = 0, p = 0;

    /* Create the socket. */
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1)
    {
        perror(argv[0]);
        fprintf(stderr, "%s: unable to create socket\n", argv[0]);
        exit(1);
    }

    /* clear out address structures */
    memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
    memset((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

    /* Set up the peer address to which we will connect. */
    servaddr_in.sin_family = AF_INET;

    /* Get the host information for the hostname that the
         * user passed in. */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    /* esta función es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
    errcode = getaddrinfo(argv[1], NULL, &hints, &res);
    if (errcode != 0)
    {
        /* Name was not found. Return a special value signifying the error. */
        fprintf(stderr, "%s: No es posible resolver la IP de %s\n", argv[0], argv[1]);
        exit(1);
    }
    else
    {
        /* Copy address of host */
        servaddr_in.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
    }
    freeaddrinfo(res);

    /* puerto del servidor en orden de red*/
    servaddr_in.sin_port = htons(PUERTO);

    /* Try to connect to the remote server at the address
         * which was just built into peeraddr.
         */
    if (connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1)
    {
        perror(argv[0]);
        fprintf(stderr, "%s: unable to connect to remote\n", argv[0]);
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
        fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
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

    /*Se lee el fichero de ordenes correspondiente, linea a linea*/
    while (fgets(buf, TAM_BUFFER, f) != NULL)
    {
        //DEBUG
        printf("FGETS: %s", buf);
        /*
        //Vaciamos el vector que contendra cada argumento de la orden
        for (p = 0; p < 3; p++)
        {
            strcpy(vect[p], "");
        }

        //Reseteamos la variable que guarda en el vector
        q = 0;
        //Separamos la linea leida en tokens delimitados por espacios
        str = strtok(buf, " ");
        //DEBUG
        for (int i = 0; i < 3;)
            printf("%s", str[i]);
        while (str != NULL)
        {

            strcpy(vect[q], str);
            //Seguimos con el siguiente token
        str = strtok(NULL, " ");
        q++; //Aumentamos la fila del vector
    }

    //Quitamos los caracteres finales a la pagina para que no de problemas
    str = strtok(vect[1], caracteresRetorno);
    strcpy(vect[1], str);

    //CABECERA: le pasamos la orden
    strcpy(envio, vect[0]);
    strcat(envio, " ");

    //FINAL: caracteres finales del protocolo
    strcat(buf, caracteresRetorno);

    //DEBUG
    printf("ENVIO: %s", envio);
    */
        /********************ENVIO**********************/
        /*Enviamos con el tamaño de la estructura enviada, si no devuelve el mismo tamaño da error*/
        if (send(s, buf, strlen(buf), 0) != strlen(buf))
        {
            fprintf(stderr, "%s: Connection aborted on error ", argv[0]);
            exit(1);
        }

        /********************RECEPCION DE RESPUESTA***********************/
        if (-1 == (recv(s, respuesta, BUFFERSIZE, 0)))
        {
            perror(argv[0]);
            fprintf(stderr, "%s: error reading result\n", argv[0]);
            exit(1);
        }

        corta = strtok(respuesta, caracteresRetorno);
        while (corta != NULL)
        {
            /*Copiamos la cadena que salga sobreescribiendo la que haya para quedarnos con la ultima*/
            strcpy(salida, corta);
            /*Seguimos con el siguiente token*/
            corta = strtok(NULL, caracteresRetorno);
        }
        strcat(salida, caracteresRetorno);
        /*Convertimos a string el puerto efimero*/
        snprintf(aux, sizeof(aux), "%d", myaddr_in.sin_port);
        strcat(aux, ".txt");
        if (NULL == (c = (fopen(aux, "a"))))
        {
            fprintf(stderr, "No se ha podido abrir el fichero");
        }
        fputs(salida, c); //Ponemos en el fichero la cabecera
        fclose(c);        //Cerramos el fichero
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
        fprintf(stderr, "%s: unable to shutdown socket\n", argv[0]);
        exit(1);
    }

    /* Print message indicating completion of task. */
    time(&timevar);
    printf("All done at %s", (char *)ctime(&timevar));
}

void UDP(FILE *f, int argc, char *argv[])
{

    int i, errcode;
    int retry = RETRIES;            /* holds the retry count */
    int s;                          /* socket descriptor */
    long timevar;                   /* contains time returned by time() */
    struct sockaddr_in myaddr_in;   /* for local socket address */
    struct sockaddr_in servaddr_in; /* for server socket address */
    struct in_addr reqaddr;         /* for returned internet address */
    int addrlen, n_retry, len, len1;
    struct sigaction vec;
    char hostname[MAXHOST];
    struct addrinfo hints, *res;

    char envio[BUFFERSIZE];     //String para el envio al servidor
    char respuesta[BUFFERSIZE]; //String para la respuesta del servidor
    char buf[TAM_BUFFER];       /*Contiene lo leido en el fichero linea a linea*/
    char conexionRed[] = "NNTP";
    char caracteresRetorno[] = "\r\n";
    char *str, *corta; //Puntero que apuntará a cada parte de la linea para separar
    char vect[3][100];
    char salida[BUFFERSIZE / 2];
    int q = 0, p = 0;
    FILE *c;
    char aux[7];

    /* Create the socket. */
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == -1)
    {
        perror(argv[0]);
        fprintf(stderr, "%s: unable to create socket\n", argv[0]);
        exit(1);
    }

    /* clear out address structures */
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
        fprintf(stderr, "%s: unable to bind socket\n", argv[0]);
        exit(1);
    }
    addrlen = sizeof(struct sockaddr_in);
    if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1)
    {
        perror(argv[0]);
        fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
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

    /* Set up the server address. */
    servaddr_in.sin_family = AF_INET;
    /* Get the host information for the server's hostname that the
         * user passed in.
         */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    /* esta función es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
    errcode = getaddrinfo(argv[1], NULL, &hints, &res);
    if (errcode != 0)
    {
        /* Name was not found.  Return a
                 * special value signifying the error. */
        fprintf(stderr, "%s: No es posible resolver la IP de %s\n", argv[0], argv[1]);
        exit(1);
    }
    else
    {
        /* Copy address of host */
        servaddr_in.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
    }
    freeaddrinfo(res);
    /* puerto del servidor en orden de red*/
    servaddr_in.sin_port = htons(PUERTO);

    /* Registrar SIGALRM para no quedar bloqueados en los recvfrom */
    vec.sa_handler = (void *)handler;
    vec.sa_flags = 0;
    if (sigaction(SIGALRM, &vec, (struct sigaction *)0) == -1)
    {
        perror(" sigaction(SIGALRM)");
        fprintf(stderr, "%s: unable to register the SIGALRM signal\n", argv[0]);
        exit(1);
    }

    /*Se lee el fichero de ordenes correspondiente, linea a linea*/
    while (fgets(buf, TAM_BUFFER, f) != NULL)
    {
        /*Vaciamos el vector que contendra cada argumento de la orden*/
        for (p = 0; p < 3; p++)
        {
            strcpy(vect[p], "");
        }

        //Reseteamos la variable que guarda en el vector
        q = 0;
        /*Separamos la linea leida en tokens delimitados por espacios*/
        str = strtok(buf, " ");
        while (str != NULL)
        {

            strcpy(vect[q], str);
            /*Seguimos con el siguiente token*/
            str = strtok(NULL, " ");
            q++; //Aumentamos la fila del vector
        }

        /*Si no existe un caracter que indique la conexion*/
        if (!strcmp(vect[2], ""))
        {
            /*Quitamos los caracteres finales a la pagina para que no de problemas*/
            str = strtok(vect[1], caracteresRetorno);
            strcpy(vect[1], str);
        }

        /*CABECERA: Copiamos la parte de la orden, la url, la conexion y los caracteres finales*/
        strcpy(envio, vect[0]);
        strcat(envio, " ");
        strcat(envio, vect[1]);
        strcat(envio, " ");
        strcat(envio, conexionRed);
        strcat(envio, caracteresRetorno);
        /*HOST: */
        strcat(envio, "Host: ");
        strcat(envio, argv[1]);
        strcat(envio, caracteresRetorno);
        /*CONEXION: En caso de que haya una especificacion de keep alive la metemos en la conecction, sino no*/
        if (vect[2][0] == 'k')
        {
            /*Si hay conexion (se ha especificado 'k')*/
            strcat(envio, "Connection: ");
            strcat(envio, "keep-alive");
            strcat(envio, caracteresRetorno);
        }

        /*FINAL: caracteres finales del protocolo*/
        strcat(envio, caracteresRetorno);

        /********************ENVIO**********************/
        n_retry = RETRIES;

        while (n_retry > 0)
        {
            /*Enviamos con el tamaño de la estructura enviada, si no devuelve el mismo tamaño da error*/
            if (sendto(s, envio, strlen(envio), 0, (struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) != strlen(envio))
            {
                perror(argv[0]);
                fprintf(stderr, "%s: unable to send request\n", argv[0]);
                exit(1);
            }
            /*Establecemos una alarma para que el recvfrom no se quede en espera infinita por ser bloqueante*/
            alarm(TIMEOUT);

            /*******************RECEPCION DE RESPUESTA******************/
            if ((len = recvfrom(s, respuesta, BUFFERSIZE, 0, (struct sockaddr *)&servaddr_in, &addrlen)) == -1)
            {
                if (errno == EINTR)
                {
                    /*Si salta la alarma quitamos un intento*/
                    printf("attempt %d (retries %d).\n", n_retry, RETRIES);
                    n_retry--;
                }
                else
                {
                    printf("Unable to get response from");
                    exit(1);
                }
            }
            else
            {
                alarm(0); //Cancelamos la alarma
                /*Sacamos la salida*/
                corta = strtok(respuesta, caracteresRetorno);
                while (corta != NULL)
                {
                    /*Copiamos la cadena que salga sobreescribiendo la que haya para quedarnos con la ultima*/
                    strcpy(salida, corta);
                    /*Seguimos con el siguiente token*/
                    corta = strtok(NULL, caracteresRetorno);
                }
                strcat(salida, caracteresRetorno);
                /*Convertimos a string el puerto efimero*/
                snprintf(aux, sizeof(aux), "%d", myaddr_in.sin_port);
                strcat(aux, ".txt");
                if (NULL == (c = (fopen(aux, "a"))))
                {
                    fprintf(stderr, "No se ha podido abrir el fichero");
                }
                fputs(salida, c); //Ponemos en el fichero la cabecera
                fclose(c);        //Cerramos el fichero
                break;            //Salimos del bucle de los intentos
            }
        }
        /*Se imprime el error de los intentos*/
        if (n_retry == 0)
        {
            printf("Unable to get response from");
            printf(" %s after %d attempts.\n", argv[1], RETRIES);
        }
    }
}

/*Señal para la señal de alarma*/
void handler()
{

    printf("La recepcion ha superado el timeout: %d\n", TIMEOUT);
}

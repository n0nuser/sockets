/*
** Fichero: servidor.c
** Autores:
** Sergio García González
** Pablo Jesús González Rubio
*/

/*
 *          		S E R V I D O R
 */
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include "utils.h"

#define _GNU_SOURCE
#define _XOPEN_SOURCE
#define PUERTO 4492
#define BUFFERSIZE 1024 /* maximum size of packets to be received */
#define MAXHOST 128
#define TIMEOUT 6 //Timeout para la señal de alarma

extern int errno;

/*
 *			M A I N
 *
 *	This routine starts the server.  It forks, leaving the child
 *	to do all the work, so it does not have to be run in the
 *	background.  It sets up the sockets.  It
 *	will loop forever, until killed by a signal.
 *
 */

void serverTCP(int s, struct sockaddr_in peeraddr_in);
void serverUDP(int s, char *buffer, struct sockaddr_in clientaddr_in);
void errout(char *); /* declare error out routine */
void handler();

int FIN = 0; /* Para el cierre ordenado */
void finalizar() { FIN = 1; }

int main(argc, argv) int argc;
char *argv[];
{

	int s_TCP, s_UDP; /* connected socket descriptor */
	int ls_TCP;		  /* listen socket descriptor */

	int cc; /* contains the number of bytes read */

	struct sigaction sa = {.sa_handler = SIG_IGN}; /* used to ignore SIGCHLD */

	struct sockaddr_in myaddr_in;	  /* for local socket address */
	struct sockaddr_in clientaddr_in; /* for peer socket address */
	int addrlen;

	fd_set readmask;
	int numfds, s_mayor;

	char buffer[BUFFERSIZE]; /* buffer for packets to be read into */

	struct sigaction vec;

	/* Create the listen socket. */
	ls_TCP = socket(AF_INET, SOCK_STREAM, 0);
	if (ls_TCP == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
		exit(1);
	}
	/* clear out address structures */
	memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));

	addrlen = sizeof(struct sockaddr_in);

	/* Set up address structure for the listen socket. */
	myaddr_in.sin_family = AF_INET;
	/* The server should listen on the wildcard address,
		 * rather than its own internet address.  This is
		 * generally good practice for servers, because on
		 * systems which are connected to more than one
		 * network at once will be able to have one server
		 * listening on all networks at once.  Even when the
		 * host is connected to only one network, this is good
		 * practice, because it makes the server program more
		 * portable.
		 */
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = htons(PUERTO);

	/* Bind the listen address to the socket. */
	if (bind(ls_TCP, (const struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
		exit(2);
	}
	/* Initiate the listen on the socket so remote users
		 * can connect.  The listen backlog is set to 5, which
		 * is the largest currently supported.
		 */
	if (listen(ls_TCP, 5) == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to listen on socket\n", argv[0]);
		exit(3);
	}

	/* Create the socket UDP. */
	s_UDP = socket(AF_INET, SOCK_DGRAM, 0);
	if (s_UDP == -1)
	{
		perror(argv[0]);
		printf("%s: unable to create socket UDP\n", argv[0]);
		exit(4);
	}
	/* Bind the server's address to the socket. */
	if (bind(s_UDP, (struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1)
	{
		perror(argv[0]);
		printf("%s: unable to bind address UDP\n", argv[0]);
		exit(5);
	}

	/* Now, all the initialization of the server is
		 * complete, and any user errors will have already
		 * been detected.  Now we can fork the daemon and
		 * return to the user.  We need to do a setpgrp
		 * so that the daemon will no longer be associated
		 * with the user's control terminal.  This is done
		 * before the fork, so that the child will not be
		 * a process group leader.  Otherwise, if the child
		 * were to open a terminal, it would become associated
		 * with that terminal as its control terminal.  It is
		 * always best for the parent to do the setpgrp.
		 */
	setpgrp();

	switch (fork())
	{
	case -1: /* Unable to fork, for some reason. */
		perror(argv[0]);
		fprintf(stderr, "%s: unable to fork daemon\n", argv[0]);
		exit(6);

	case 0: /* The child process (daemon) comes here. */

		/* Close stdin and stderr so that they will not
			 * be kept open.  Stdout is assumed to have been
			 * redirected to some logging file, or /dev/null.
			 * From now on, the daemon will not report any
			 * error messages.  This daemon will loop forever,
			 * waiting for connections and forking a child
			 * server to handle each one.
			 */
		fclose(stdin);
		fclose(stderr);

		/* Set SIGCLD to SIG_IGN, in order to prevent
			 * the accumulation of zombies as each child
			 * terminates.  This means the daemon does not
			 * have to make wait calls to clean them up.
			 */
		if (sigaction(SIGCHLD, &sa, NULL) == -1)
		{
			perror(" sigaction(SIGCHLD)");
			fprintf(stderr, "%s: unable to register the SIGCHLD signal\n", argv[0]);
			exit(7);
		}

		/* Registrar SIGTERM para la finalizacion ordenada del programa servidor */
		vec.sa_handler = (void *)finalizar;
		vec.sa_flags = 0;
		if (sigaction(SIGTERM, &vec, (struct sigaction *)0) == -1)
		{
			perror(" sigaction(SIGTERM)");
			fprintf(stderr, "%s: unable to register the SIGTERM signal\n", argv[0]);
			exit(8);
		}

		while (!FIN)
		{
			/* Meter en el conjunto de sockets los sockets UDP y TCP */
			FD_ZERO(&readmask);
			FD_SET(ls_TCP, &readmask);
			FD_SET(s_UDP, &readmask);
			/* 
            Seleccionar el descriptor del socket que ha cambiado. Deja una marca en 
            el conjunto de sockets (readmask)
            */
			if (ls_TCP > s_UDP)
				s_mayor = ls_TCP;
			else
				s_mayor = s_UDP;

			if ((numfds = select(s_mayor + 1, &readmask, (fd_set *)0, (fd_set *)0, NULL)) < 0)
			{
				if (errno == EINTR)
				{
					FIN = 1;
					close(ls_TCP);
					close(s_UDP);
					perror("\nFinalizando el servidor. Senal recibida en select\n ");
				}
			}
			else
			{

				/* Comprobamos si el socket seleccionado es el socket TCP */
				if (FD_ISSET(ls_TCP, &readmask))
				{
					/* Note that addrlen is passed as a pointer
                     * so that the accept call can return the
                     * size of the returned address.
                     */
					/* This call will block until a new
    				 * connection arrives.  Then, it will
    				 * return the address of the connecting
    				 * peer, and a new socket descriptor, s,
    				 * for that connection.
    				 */
					s_TCP = accept(ls_TCP, (struct sockaddr *)&clientaddr_in, &addrlen);
					if (s_TCP == -1)
						exit(9);
					switch (fork())
					{
					case -1: /* Can't fork, just exit. */

					case 0:			   /* Child process comes here. */
						close(ls_TCP); /* Close the listen socket inherited from the daemon. */
						serverTCP(s_TCP, clientaddr_in);
						exit(0);
					default: /* Daemon process comes here. */
							 /* The daemon needs to remember
        					 * to close the new accept socket
        					 * after forking the child.  This
        					 * prevents the daemon from running
        					 * out of file descriptor space.  It
        					 * also means that when the server
        					 * closes the socket, that it will
        					 * allow the socket to be destroyed
        					 * since it will be the last close.
        					 */
						close(s_TCP);
					}
				} /* De TCP*/
				/* Comprobamos si el socket seleccionado es el socket UDP */
				if (FD_ISSET(s_UDP, &readmask))
				{
					/* This call will block until a new
                * request arrives.  Then, it will
                * return the address of the client,
                * and a buffer containing its request.
                * BUFFERSIZE - 1 bytes are read so that
                * room is left at the end of the buffer
                * for a null character.
                */
					cc = recvfrom(s_UDP, buffer, BUFFERSIZE - 1, 0,
								  (struct sockaddr *)&clientaddr_in, &addrlen);
					if (cc == -1)
					{
						perror(argv[0]);
						printf("%s: recvfrom error\n", argv[0]);
						exit(11);
					}
					/* Make sure the message received is
                * null terminated.
                */
					buffer[cc] = '\0';
					serverUDP(s_UDP, buffer, clientaddr_in);
				}
			}
		} /* Fin del bucle infinito de atenci�n a clientes */
		/* Cerramos los sockets UDP y TCP */
		close(ls_TCP);
		close(s_UDP);

		printf("\nFin de programa servidor!\n");

	default: /* Parent process comes here. */
		exit(0);
	}
}

/*
 *				S E R V E R T C P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */

void serverTCP(int s, struct sockaddr_in clientaddr_in)
{
	int reqcnt = 0;							   // Tiene la cuenta del numero de peticiones
	char mensaje[BUFFERSIZE * 5] = "";		   // Donde se almacena el mensaje con el que luego se opera
	char mensajeOriginal[BUFFERSIZE * 5] = ""; // Donde se almacena el mensaje con el que NO se opera
	char hostname[MAXHOST];					   // Donde se almacena el nombre del cliente

	int len; // Para la comprobacion del RECV
	int status;
	long timevar; // Tiene la fecha y la hora que devuelve time()

	struct linger linger; //Allow a lingering, graceful close; Used when setting SO_LINGER

	//VARIABLES PARA NNTP
	char conexionRed[BUFFERSIZE] = "";	// Donde guardamos la información que va al log
	char caracteresRetorno[] = "\r\n";	// Variables de control de línea
	FILE *p, *g, *a;					// Punteros al diferentes archivos
	char respuesta[BUFFERSIZE * 6];		// Envio de respuesta al cliente
	char temp[BUFFERSIZE];				// Cadena auxiliar
	char temp2[50];						// Cadena auxiliar 2
	char grupoElegido[BUFFERSIZE] = ""; // Se utiliza para mantener el control del grupo elegido al hacer GROUP, para luego ARTICLE y demas

	char *aux;							// Puntero auxiliar para distintas operaciones
	char token[4][100];					// Donde se trocea el mensaje recibido para las comprobaciones, NO TOCAR EL 100 O DA ERROR
	char tokenTemp[BUFFERSIZE];			// Se utiliza para el POST, el anterior no sirve
	char envio[BUFFERSIZE];				// Mensaje entero que recibe el servidor
	char temporal[BUFFERSIZE * 6] = ""; // Variable para indicar la fecha y hora de una peticion
	int q = 0, i = 0;

	//PATH'S DE LOS FICHEROS
	char pathGrupos[] = "nntp/noticias/grupos";				 //Nombre del archivo group, para LIST
	char pathArticulos[] = "nntp/noticias/articulos/local/"; //Nombre del archivo group, para LIST
	char ficheroLog[] = "nntpd.log";						 //Nombre del archivo de peticiones

	/* Look up the host information for the remote host
	 * that we have connected with.  Its internet address
	 * was returned by the accept call, in the main
	 * daemon loop above.
	 */
	status = getnameinfo((struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in), hostname, MAXHOST, NULL, 0, 0);

	if (status)
	{
		/* The information is unavailable for the remote
			 * host.  Just format its internet address to be
			 * printed out in the logging information.  The
			 * address will be shown in "internet dot format".
			 */
		/* inet_ntop para interoperatividad con IPv6 */
		if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
			perror(" inet_ntop \n");
	}

	/* Log a startup message. */
	time(&timevar);
	/* The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */

	/////////////////////////////////////////////////////
	// AQUI EMPIEZA EL COMPORTAMIENTO DEL SERVIDOR TCP //
	/////////////////////////////////////////////////////
	printf("[S] Startup from %s port %u at %s", hostname, ntohs(clientaddr_in.sin_port), (char *)ctime(&timevar));
	sleep(5);
	/* Set the socket for a lingering, graceful close.
		 * This will cause a final close of this socket to wait until all of the
		 * data sent on it has been received by the remote host.
		 */
	linger.l_onoff = 1;
	linger.l_linger = 1;
	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger,
				   sizeof(linger)) == -1)
	{
		errout(hostname);
	}

	// CABECERA DEL FICHERO NNTP.LOG
	strcpy(temp, "");
	strcpy(temp2, "");

	strcat(temp, " ");
	strcat(temp, ctime(&timevar)); //Metemos la fecha y hora

	strcat(temp, " [C] ");
	strcat(temp, hostname);
	strcat(temp, " ");
	strcat(temp, inet_ntoa(clientaddr_in.sin_addr)); //Metemos la IP

	strcat(temp, " ");
	strcat(temp, "TCP"); //Metemos protocolo de transporte

	strcat(temp, " ");
	snprintf(temp2, sizeof(temp2), "%u", ntohs(clientaddr_in.sin_port)); //Este es el puerto efímero
	strcat(temp, temp2);
	strcat(temp, "\n"); //Metemos puerto efimero

	//Se guarda la información en el fichero nntp.log
	if (NULL == (p = (fopen(ficheroLog, "a"))))
		fprintf(stderr, "No se ha podido abrir el fichero");
	fputs(temp, p); //Ponemos en el fichero la cabecera
	fclose(p);

	//RESPUESTA AL CLIENTE CUANDO SE HA ESTABLECIDO CONEXION
	strcpy(conexionRed, "");
	strcat(conexionRed, "200 Servidor de noticias ");
	strcat(conexionRed, hostname);
	strcat(conexionRed, " ");
	strcat(conexionRed, inet_ntoa(clientaddr_in.sin_addr));
	strcat(conexionRed, " ");
	strcat(conexionRed, (char *)ctime(&timevar));

	//Se guarda el establecimiento de conexion
	if (NULL == (p = (fopen(ficheroLog, "a"))))
		fprintf(stderr, "No se ha podido abrir el fichero");
	fputs(conexionRed, p);
	fclose(p);

	// Enviamos al cliente un  codigo 200
	if (send(s, conexionRed, BUFFERSIZE, 0) != strlen(conexionRed))
		fprintf(stderr, "Servidor: Send error ");

	/* Go into a loop, receiving requests from the remote
		 * client.  After the client has sent the last request,
		 * it will do a shutdown for sending, which will cause
		 * an end-of-file condition to appear on this end of the
		 * connection.  After all of the client's requests have
		 * been received, the next recv call will return zero
		 * bytes, signalling an end-of-file condition.  This is
		 * how the server will know that no more requests will
		 * follow, and the loop will be exited.
		 */
	while ((len = recv(s, mensaje, BUFFERSIZE * 5, 0)) != -1)
	{
		strcpy(mensajeOriginal, mensaje); // Hacemos una copia de mensaje en mensajeOriginal
		reqcnt++;						  //Se aumenta el numero de peticiones

		// REGISTRO DE LA FECHA, HORA, HOST Y PUERTO DE LA PETICION RECIBIDA
		time(&timevar);
		strcpy(temporal, (char *)ctime(&timevar));
		aux = strtok(temporal, caracteresRetorno);
		strcpy(temp, aux);
		strcpy(temporal, "");
		sprintf(temporal, "[%s][%s:%u][TCP] - %s\n", temp, hostname, ntohs(clientaddr_in.sin_port), mensajeOriginal);
		printf("%s", temporal);
		if (NULL == (p = (fopen(ficheroLog, "a"))))
			fprintf(stderr, "No se ha podido abrir el fichero");
		fputs(temporal, p);
		fclose(p);

		//Inicializamos el vector token
		for (i = 0; i < 3; i++)
			strcpy(token[i], "");
		q = 0;

		//Separamos la linea leida en tokens delimitados por espacios
		aux = strtok(mensaje, " ");
		while (aux != NULL)
		{
			strcpy(token[q], aux);
			// Seguimos con el siguiente token
			aux = strtok(NULL, " ");
			q++;
		}

		//Si no existe algun dato que acompañe a la orden
		if (strcmp(token[1], "") == 0)
		{
			//Quitamos los caracteres finales a la pagina para que no de problemas
			aux = strtok(token[0], caracteresRetorno);
			strcpy(token[0], aux);
			strcpy(envio, token[0]);
			strcat(envio, caracteresRetorno);
		}
		else
		{
			strcpy(envio, token[0]);
			strcat(envio, " ");
			strcat(envio, token[1]);
			strcat(envio, " ");
			strcat(envio, token[2]);
			strcat(envio, " ");
			strcat(envio, token[3]);
		}

		//Eliminamos los caracteres de retorno que nos llegan del cliente
		strtok(mensaje, caracteresRetorno);
		strtok(envio, caracteresRetorno);

		//Se utiliza para POST
		strcpy(tokenTemp, token[0]);
		aux = strtok(tokenTemp, caracteresRetorno);
		if (aux != NULL)
			strcpy(tokenTemp, aux);

		memset(respuesta, 0, sizeof(respuesta));

		//COMPROBAR LA PETICION
		if (strcmp(token[0], "LIST") == 0)
		{
			list(respuesta, pathGrupos, g);

			sleep(5); //Tiempo de trabajo del servidor

			if (send(s, respuesta, BUFFERSIZE, 0) != BUFFERSIZE)
				errout(hostname);
		}
		else if (strcmp(token[0], "NEWGROUPS") == 0)
		{
			newgroups(respuesta, pathGrupos, g, token[1], token[2]);

			sleep(5); //Tiempo de trabajo del servidor

			if (send(s, respuesta, BUFFERSIZE, 0) != BUFFERSIZE)
				errout(hostname);
		}
		else if (strcmp(token[0], "NEWNEWS") == 0)
		{
			newnews(respuesta, pathGrupos, pathArticulos, g, token[1], token[2], token[3]);

			sleep(5); //Tiempo de trabajo del servidor

			if (send(s, respuesta, BUFFERSIZE, 0) != BUFFERSIZE)
				errout(hostname);
		}
		else if (strcmp(token[0], "GROUP") == 0)
		{
			strtok(token[1], caracteresRetorno);
			//Ahora tratamos el segundo trozo que llega del group
			if (group(respuesta, pathGrupos, g, token[1]))
			{
				sleep(5); //Tiempo de trabajo del servidor

				strcpy(grupoElegido, token[1]);
				if (send(s, respuesta, BUFFERSIZE, 0) != BUFFERSIZE)
					errout(hostname);
			}
			else
			{
				strcpy(respuesta, "");
				strcat(respuesta, "411 No such newgroups");
				strcat(respuesta, caracteresRetorno);

				sleep(5); //Tiempo de trabajo del servidor

				if (send(s, respuesta, BUFFERSIZE, 0) != BUFFERSIZE)
					errout(hostname);
			}
		}
		else if (strcmp(token[0], "ARTICLE") == 0)
		{
			if (strcmp(grupoElegido, "") != 0)
			{
				if (article(respuesta, pathArticulos, a, token[1], grupoElegido))
				{
					sleep(5); //Tiempo de trabajo del servidor

					if (send(s, respuesta, BUFFERSIZE, 0) != BUFFERSIZE)
						errout(hostname);
				}
				else
				{
					strcpy(respuesta, "");
					strcat(respuesta, "423  No article with that number");
					strcat(respuesta, caracteresRetorno);

					sleep(5); //Tiempo de trabajo del servidor

					if (send(s, respuesta, BUFFERSIZE, 0) != BUFFERSIZE)
						errout(hostname);
				}
			}
			else
			{
				strcpy(respuesta, "");
				strcat(respuesta, "412 No newgroups selected");
				strcat(respuesta, caracteresRetorno);

				sleep(5); //Tiempo de trabajo del servidor

				if (send(s, respuesta, BUFFERSIZE, 0) != BUFFERSIZE)
					errout(hostname);
			}
		}
		else if (strcmp(token[0], "HEAD") == 0)
		{
			if (strcmp(grupoElegido, "") != 0)
			{
				if (head(respuesta, pathArticulos, a, token[1], grupoElegido))
				{
					if (send(s, respuesta, BUFFERSIZE, 0) != BUFFERSIZE)
						errout(hostname);
				}
				else
				{
					strcpy(respuesta, "");
					strcat(respuesta, "423  No article with that number");
					strcat(respuesta, caracteresRetorno);

					sleep(5); //Tiempo de trabajo del servidor

					if (send(s, respuesta, BUFFERSIZE, 0) != BUFFERSIZE)
						errout(hostname);
				}
			}
			else
			{
				strcpy(respuesta, "");
				strcat(respuesta, "412 No newgroups selected");
				strcat(respuesta, caracteresRetorno);

				sleep(5); //Tiempo de trabajo del servidor

				if (send(s, respuesta, BUFFERSIZE, 0) != BUFFERSIZE)
					errout(hostname);
			}
		}
		else if (strcmp(token[0], "BODY") == 0)
		{
			if (strcmp(grupoElegido, "") != 0)
			{
				if (body(respuesta, pathArticulos, a, token[1], grupoElegido))
				{
					sleep(5); //Tiempo de trabajo del servidor

					if (send(s, respuesta, BUFFERSIZE, 0) != BUFFERSIZE)
						errout(hostname);
				}
				else
				{
					strcpy(respuesta, "");
					strcat(respuesta, "423  No article with that number");
					strcat(respuesta, caracteresRetorno);

					sleep(5); //Tiempo de trabajo del servidor

					if (send(s, respuesta, BUFFERSIZE, 0) != BUFFERSIZE)
						errout(hostname);
				}
			}
			else
			{
				strcpy(respuesta, "");
				strcat(respuesta, "412 No newgroups selected");
				strcat(respuesta, caracteresRetorno);

				sleep(5); //Tiempo de trabajo del servidor

				if (send(s, respuesta, BUFFERSIZE, 0) != BUFFERSIZE)
					errout(hostname);
			}
		}
		else if (strcmp(tokenTemp, "POST") == 0)
		{
			if (post(s, mensajeOriginal, pathGrupos, pathArticulos, g))
			{
				strcpy(respuesta, "240 Article received OK\n");

				sleep(5); //Tiempo de trabajo del servidor

				if (send(s, respuesta, strlen(respuesta), 0) != strlen(respuesta))
					errout(hostname);
			}
			else
			{
				strcpy(respuesta, "441 Posting failed");

				sleep(5); //Tiempo de trabajo del servidor

				if (send(s, respuesta, strlen(respuesta), 0) != strlen(respuesta))
					errout(hostname);
			}
		}
		else if (strcmp(token[0], "QUIT") == 0)
		{
			strcpy(respuesta, "\n205 Bye!\n");

			sleep(5); //Tiempo de trabajo del servidor

			if (send(s, respuesta, strlen(respuesta), 0) != strlen(respuesta))
				errout(hostname);
			break;
		}
		else
		{
			//Respuesta - 500 Command not recognized
			strcpy(respuesta, "500 Command not recognized");
			strcat(respuesta, caracteresRetorno);

			sleep(5); //Tiempo de trabajo del servidor

			if (send(s, respuesta, BUFFERSIZE, 0) != BUFFERSIZE)
				errout(hostname);
		}
		memset(respuesta, 0, sizeof respuesta);
		memset(mensaje, 0, sizeof mensaje);
		strcpy(mensaje,"");
		memset(envio, 0, BUFFERSIZE);
		memset(token[0], 0, sizeof token[0]);
		memset(token[1], 0, sizeof token[1]);
	}
	/* The loop has terminated, because there are no
		 * more requests to be serviced.  As mentioned above,
		 * this close will block until all of the sent replies
		 * have been received by the remote host.  The reason
		 * for lingering on the close is so that the server will
		 * have a better idea of when the remote has picked up
		 * all of the data.  This will allow the start and finish
		 * times printed in the log file to reflect more accurately
		 * the length of time this connection was used.
		 */
	close(s);
	/* Log a finishing message. */
	time(&timevar);
	/* The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */
	printf("\nCompleted %s port %u, %d requests, at %s\n", hostname, ntohs(clientaddr_in.sin_port), reqcnt, (char *)ctime(&timevar));
}

//This routine aborts the child process attending the client.
void errout(char *hostname)
{
	printf("\nConnection with %s aborted on error\n", hostname);
	exit(12);
}

/*
 *				S E R V E R U D P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverUDP(int s, char *buffer, struct sockaddr_in clientaddr_in)
{
	int reqcnt = 0;							   // Tiene la cuenta del numero de peticiones
	char mensaje[BUFFERSIZE * 5] = "";		   // Donde se almacena el mensaje con el que luego se opera
	char mensajeOriginal[BUFFERSIZE * 5] = ""; // Donde se almacena el mensaje con el que NO se opera
	char hostname[MAXHOST];					   // El nombre del host
	int nc, errcode;
	long timevar; // Contains time returned by time()

	int len, status;

	struct sigaction alarma; //Se crea el comportamiento para la señal al producirse el RETRY de la peticion
	struct addrinfo hints;
	struct linger linger; //Allow a lingering, graceful close; Used when setting SO_LINGER

	//VARIABLES PARA NNTP
	char caracteresRetorno[] = "\r\n";	// Variables de control de línea
	FILE *p, *g, *a;					// Punteros al diferentes archivos
	char respuesta[BUFFERSIZE * 6];		// Envio de respuesta al cliente
	char temp[BUFFERSIZE];				// Cadena auxiliar
	char temp2[50];						// Cadena auxiliar 2
	char grupoElegido[BUFFERSIZE] = ""; // Se utiliza para mantener el control del grupo elegido al hacer GROUP, para luego ARTICLE y demas
	char *aux;							// Puntero auxiliar para distintas operaciones
	char tokenTemp[BUFFERSIZE];			// Se utiliza para el POST, el anterior no sirve
	char envio[BUFFERSIZE];				// Mensaje entero que recibe el servidor
	char temporal[BUFFERSIZE * 6] = ""; // Variable para indicar la fecha y hora de una peticion
	char token[4][BUFFERSIZE];			// Donde se trocea el mensaje recibido para las comprobaciones, NO TOCAR EL 100 O DA ERROR
	int q = 0, i = 0;

	int addrlen;
	int flagSocket = 1; //Flag que contola el estado del socket(abierto:true/cerrado:false)

	//PATH'S DE LOS FICHEROS
	char pathGrupos[] = "nntp/noticias/grupos";				 //Nombre del archivo group, para LIST
	char pathArticulos[] = "nntp/noticias/articulos/local/"; //Nombre del archivo group, para LIST
	char ficheroLog[] = "nntpd.log";						 //Nombre del archivo de peticiones

	time(&timevar);

	// Limpiamos las variables y las inicializamos
	strcpy(mensaje, buffer);
	strcpy(mensajeOriginal, buffer);

	addrlen = sizeof(struct sockaddr_in);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;

	time(&timevar);

	printf("Startup port %u at %s", ntohs(clientaddr_in.sin_port), (char *)ctime(&timevar));

	linger.l_onoff = 1;
	linger.l_linger = 1;

	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == -1)
	{
		errout(hostname);
	}

	/* Registrar SIGALRM para no quedar bloqueados en los recvfrom */
	alarma.sa_handler = (void *)handler;
	alarma.sa_flags = 0;
	if (sigaction(SIGALRM, &alarma, (struct sigaction *)0) == -1)
	{
		perror(" sigaction(SIGALRM)");
		fprintf(stderr, "Unable to register the SIGALRM signal\n");
		exit(1);
	}

	status = getnameinfo((struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in), hostname, MAXHOST, NULL, 0, 0);

	if (status)
	{
		/* The information is unavailable for the remote
			 * host.  Just format its internet address to be
			 * printed out in the logging information.  The
			 * address will be shown in "internet dot format".
			 */
		/* inet_ntop para interoperatividad con IPv6 */
		if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
			perror(" inet_ntop \n");
	}

	//REGISTRAMOS LA CABECERA DE NNTP EN EL LOG
	strcpy(temp, "");
	strcpy(temp2, "");

	strcat(temp, " ");
	strcat(temp, ctime(&timevar)); //Metemos la fecha y hora

	strcat(temp, " [C] ");
	strcat(temp, hostname);
	strcat(temp, " ");
	strcat(temp, inet_ntoa(clientaddr_in.sin_addr)); //Metemos la IP

	strcat(temp, " ");
	strcat(temp, "UDP"); //Metemos protocolo de transporte

	strcat(temp, " ");
	snprintf(temp2, sizeof(temp2), "%u", ntohs(clientaddr_in.sin_port)); //Este es el puerto efímero
	strcat(temp, temp2);
	strcat(temp, "\n"); //Metemos puerto efimero

	//Se guarda la información en el fichero nntp.log
	if (NULL == (p = (fopen(ficheroLog, "a"))))
		fprintf(stderr, "No se ha podido abrir el fichero");
	fputs(temp, p); //Ponemos en el fichero la cabecera
	fclose(p);		//Cerramos el fichero

	for (i = 0; i < 3; i++)
		strcpy(token[i], "");

	do
	{
		flagSocket = 1;
		reqcnt++;

		// REGISTRO DE LA FECHA, HORA, HOST Y PUERTO DE LA PETICION RECIBIDA
		time(&timevar);
		strcpy(temporal, (char *)ctime(&timevar));
		aux = strtok(temporal, caracteresRetorno);
		strcpy(temp, aux);
		strcpy(temporal, "");
		sprintf(temporal, "[%s][%s:%u][UDP] - %s\n", temp, hostname, ntohs(clientaddr_in.sin_port), mensajeOriginal);
		printf("%s", temporal);
		if (NULL == (p = (fopen(ficheroLog, "a"))))
			fprintf(stderr, "No se ha podido abrir el fichero");
		fputs(temporal, p);
		fclose(p);

		q = 0;
		//Separamos la linea leida en tokens delimitados por espacios
		aux = strtok(mensaje, " ");
		while (aux != NULL && q != 3)
		{
			strcpy(token[q], aux);
			// Seguimos con el siguiente token
			aux = strtok(NULL, " ");
			q++;
		}

		//Si no existe algun dato que acompañe a la orden
		if (strcmp(token[1], "") == 0)
		{
			//Quitamos los caracteres finales a la pagina para que no de problemas
			aux = strtok(token[0], caracteresRetorno);
			strcpy(token[0], aux);
			strcpy(envio, token[0]);
			strcat(envio, caracteresRetorno);
		}
		else
		{
			strcpy(envio, token[0]);
			strcat(envio, " ");
			strcat(envio, token[1]);
			strcat(envio, " ");
			strcat(envio, token[2]);
			strcat(envio, " ");
			strcat(envio, token[3]);
		}

		//Eliminamos los caracteres de retorno que nos llegan del cliente
		strtok(mensaje, caracteresRetorno);
		strtok(envio, caracteresRetorno);

		strcpy(tokenTemp, token[0]);
		aux = strtok(tokenTemp, caracteresRetorno);
		if (aux != NULL)
			strcpy(tokenTemp, aux);

		strcpy(respuesta, "");
		//COMPROBAR LA PETICION
		if (strcmp(token[0], "LIST") == 0)
		{
			list(respuesta, pathGrupos, g);

			if (sendto(s, respuesta, BUFFERSIZE, 0, (struct sockaddr *)&clientaddr_in, addrlen) != BUFFERSIZE)
				errout(hostname);
		}
		else if (strcmp(token[0], "NEWGROUPS") == 0)
		{
			newgroups(respuesta, pathGrupos, g, token[1], token[2]);

			if (sendto(s, respuesta, BUFFERSIZE, 0, (struct sockaddr *)&clientaddr_in, addrlen) != BUFFERSIZE)
				errout(hostname);
		}
		else if (strcmp(token[0], "NEWNEWS") == 0)
		{
			newnews(respuesta, pathGrupos, pathArticulos, g, token[1], token[2], token[3]);

			if (sendto(s, respuesta, BUFFERSIZE, 0, (struct sockaddr *)&clientaddr_in, addrlen) != BUFFERSIZE)
				errout(hostname);
		}
		else if (strcmp(token[0], "GROUP") == 0)
		{
			strtok(token[1], caracteresRetorno);
			//Ahora tratamos el segundo trozo que llega del group
			if (group(respuesta, pathGrupos, g, token[1]))
			{
				strcpy(grupoElegido, token[1]);
				if (sendto(s, respuesta, BUFFERSIZE, 0, (struct sockaddr *)&clientaddr_in, addrlen) != BUFFERSIZE)
					errout(hostname);
			}
			else
			{
				strcpy(respuesta, "");
				strcat(respuesta, "411 No such newgroups");
				strcat(respuesta, caracteresRetorno);

				if (sendto(s, respuesta, BUFFERSIZE, 0, (struct sockaddr *)&clientaddr_in, addrlen) != BUFFERSIZE)
					errout(hostname);
			}
		}
		else if (strcmp(token[0], "ARTICLE") == 0)
		{
			if (strcmp(grupoElegido, "") != 0)
			{
				if (article(respuesta, pathArticulos, a, token[1], grupoElegido))
				{
					if (sendto(s, respuesta, BUFFERSIZE, 0, (struct sockaddr *)&clientaddr_in, addrlen) != BUFFERSIZE)
						errout(hostname);
				}
				else
				{
					strcpy(respuesta, "");
					strcat(respuesta, "423  No article with that number");
					strcat(respuesta, caracteresRetorno);

					if (sendto(s, respuesta, BUFFERSIZE, 0, (struct sockaddr *)&clientaddr_in, addrlen) != BUFFERSIZE)
						errout(hostname);
				}
			}
			else
			{
				strcpy(respuesta, "");
				strcat(respuesta, "412 No newgroups selected");
				strcat(respuesta, caracteresRetorno);

				if (sendto(s, respuesta, BUFFERSIZE, 0, (struct sockaddr *)&clientaddr_in, addrlen) != BUFFERSIZE)
					errout(hostname);
			}
		}
		else if (strcmp(token[0], "HEAD") == 0)
		{
			if (strcmp(grupoElegido, "") != 0)
			{
				if (head(respuesta, pathArticulos, a, token[1], grupoElegido))
				{
					if (sendto(s, respuesta, BUFFERSIZE, 0, (struct sockaddr *)&clientaddr_in, addrlen) != BUFFERSIZE)
						errout(hostname);
				}
				else
				{
					strcpy(respuesta, "");
					strcat(respuesta, "423  No article with that number");
					strcat(respuesta, caracteresRetorno);

					if (sendto(s, respuesta, BUFFERSIZE, 0, (struct sockaddr *)&clientaddr_in, addrlen) != BUFFERSIZE)
						errout(hostname);
				}
			}
			else
			{
				strcpy(respuesta, "");
				strcat(respuesta, "412 No newgroups selected");
				strcat(respuesta, caracteresRetorno);

				if (send(s, respuesta, BUFFERSIZE, 0) != BUFFERSIZE)
					errout(hostname);
			}
		}
		else if (strcmp(token[0], "BODY") == 0)
		{
			if (strcmp(grupoElegido, "") != 0)
			{
				if (body(respuesta, pathArticulos, a, token[1], grupoElegido))
				{
					if (sendto(s, respuesta, BUFFERSIZE, 0, (struct sockaddr *)&clientaddr_in, addrlen) != BUFFERSIZE)
						errout(hostname);
				}
				else
				{
					strcpy(respuesta, "");
					strcat(respuesta, "423  No article with that number");
					strcat(respuesta, caracteresRetorno);

					if (sendto(s, respuesta, BUFFERSIZE, 0, (struct sockaddr *)&clientaddr_in, addrlen) != BUFFERSIZE)
						errout(hostname);
				}
			}
			else
			{
				strcpy(respuesta, "");
				strcat(respuesta, "412 No newgroups selected");
				strcat(respuesta, caracteresRetorno);

				if (sendto(s, respuesta, BUFFERSIZE, 0, (struct sockaddr *)&clientaddr_in, addrlen) != BUFFERSIZE)
					errout(hostname);
			}
		}
		else if (strcmp(tokenTemp, "POST") == 0)
		{
			if (post(s, mensajeOriginal, pathGrupos, pathArticulos, g))
			{
				strcpy(respuesta, "240 Article received OK\n");

				if (sendto(s, respuesta, BUFFERSIZE, 0, (struct sockaddr *)&clientaddr_in, addrlen) != BUFFERSIZE)
					errout(hostname);
			}
			else
			{
				strcpy(respuesta, "441 Posting failed");

				if (sendto(s, respuesta, BUFFERSIZE, 0, (struct sockaddr *)&clientaddr_in, addrlen) != BUFFERSIZE)
					errout(hostname);
			}
			memset(mensaje, 0, sizeof mensaje);
		}
		else if (strcmp(token[0], "QUIT") == 0)
		{
			strcpy(respuesta, "\n205 Bye!\n");

			if (sendto(s, respuesta, BUFFERSIZE, 0, (struct sockaddr *)&clientaddr_in, addrlen) != BUFFERSIZE)
				errout(hostname);

			flagSocket = 0;
		}
		else
		{
			strcpy(respuesta, "");
			strcat(respuesta, "500 Command not recognized");
			strcat(respuesta, caracteresRetorno);

			if (sendto(s, respuesta, BUFFERSIZE, 0, (struct sockaddr *)&clientaddr_in, addrlen) != BUFFERSIZE)
				errout(hostname);
		}
		memset(mensaje, 0, sizeof mensaje);
		memset(envio, 0, sizeof envio);
		memset(token[0], 0, sizeof token[0]);
		memset(token[1], 0, sizeof token[1]);
		memset(respuesta, 0, sizeof respuesta);
		memset(mensajeOriginal, 0, sizeof(mensajeOriginal));
		memset(temporal, 0, sizeof temporal);
		memset(buffer, 0, BUFFERSIZE);
		strcpy(buffer,"");

		if (flagSocket)
		{
			alarm(TIMEOUT);
			//Recibe y vuelve a enviar al empezar el wh
			if (recvfrom(s, buffer, BUFFERSIZE, 0, (struct sockaddr *)&clientaddr_in, &addrlen) == -1)
			{
				if (errno == EINTR)
					break;
				else
				{
					printf("Unable to get response from");
					exit(1);
				}
			}
			strcpy(mensaje, buffer);
			strcpy(mensajeOriginal, buffer);
		}
		else
			break;

	} while (1);

	time(&timevar);
	printf("Completed port %u, %d requests, at %s\n", ntohs(clientaddr_in.sin_port), reqcnt, (char *)ctime(&timevar));
	close(s);
}

// Funcion que necesita el SIGACTION
void handler()
{
	printf("No more messages to receive\n");
}
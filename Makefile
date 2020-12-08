CC = gcc
CFLAGS = 
#Descomentar la siguiente linea para olivo
#LIBS = -lsocket -lnsl
#Descomentar la siguiente linea para linux
LIBS =

PROGS = servidor clientcp clientudp

all: ${PROGS}

servidor: servidor.o
	${CC} ${CFLAGS} -o $@ servidor.o ${LIBS}
	
clientcp: clientcp.o
	${CC} ${CFLAGS} -o $@ clientcp.o ${LIBS}

clientudp: clientudp.o
	${CC} ${CFLAGS} -o $@ clientudp.o ${LIBS}

clean:
	rm *.o ${PROGS}

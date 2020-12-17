CC = gcc
CFLAGS = 
#Descomentar la siguiente linea para olivo
#LIBS = -lsocket -lnsl
#Descomentar la siguiente linea para linux
LIBS = 

PROGS = servidor cliente

all: ${PROGS}

servidor: servidor.o utils.o
	${CC} ${CFLAGS} -o $@ servidor.o utils.o ${LIBS}

cliente: cliente.o utils.o
	${CC} ${CFLAGS} -o $@ cliente.o utils.o ${LIBS}

utils.o: utils.c utils.h
	${CC} ${CFLAGS} -c -g utils.c ${LIBS}

clean:
	rm *.o ${PROGS} nntpd.log envio200.txt

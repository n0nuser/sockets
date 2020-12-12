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

cliente: cliente.o
	${CC} ${CFLAGS} -o $@ cliente.o ${LIBS}

utils: utils.o utils.h
	${CC} ${CFLAGS} -o $@ utils.o ${LIBS}

clean:
	rm *.o ${PROGS} nntpd.log

all: thread

CC = gcc
CFLAGS = -g -c -Wall -D_REENTRANT -Wall -DLOGFILE
LINKS=-lpthread -lm -lstdc++
#CC = arm-linux-gcc
RM = /bin/rm


OBJECTS = client.o client_thr_socket_handler.o client_thr_cli.o ../functions.o



# COMPILING
.C.o:
	${CC} ${CFLAGS} -o $*.o $<

# LINKING
thread: $(OBJECTS)
	${CC}  -o client $(OBJECTS) $(LFLAGS) $(LINKS)




clean:
	$(RM) -f client $(OBJECTS) *~

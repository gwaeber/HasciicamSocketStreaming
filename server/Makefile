all: thread

CC = arm-linux-gnueabihf-gcc
CFLAGS = -g -c -Wall -D_REENTRANT -Wall -DLOGFILE
LINKS=-lpthread -lm -lstdc++
#CC = arm-linux-gcc
RM = /bin/rm


OBJECTS = server.o server_thr_send.o server_thr_receive.o server_thr_io.o ../functions.o



# COMPILING
.C.o:
	${CC} ${CFLAGS} -o $*.o $<

# LINKING
thread: $(OBJECTS)
	${CC}  -o server $(OBJECTS) $(LFLAGS) $(LINKS)




clean:
	$(RM) -f server $(OBJECTS) *~

CFLAGS = -Wall -g -pthread
CC	= gcc $(CFLAGS)

PROGRAMS = bl_server bl_client

all : $(PROGRAMS)


bl_server: server_funcs.c bl_server.c
	$(CC) -o bl_server server_funcs.c bl_server.c

bl_client: bl_client.c simpio.c
	$(CC) -o bl_client bl_client.c simpio.c




clean:
	rm -f *.o $(PROGRAMS)


include test_Makefile

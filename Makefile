CC := clang
CFLAGS := -g -lpthread

all: client server

clean:
	rm -rf *~ server client server.dSYM client.dSYM


server: server.c deps/socket.h deps/cJSON.h deps/cJSON.c deps/uthash.h game_structs.h
	$(CC) $(CFLAGS) -o server server.c deps/cJSON.c

client: client.c socket.h game_structs.h
	$(CC) $(CFLAGS) -o client client.c


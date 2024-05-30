

# Makefile for building server
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread

# Targets
TARGETS = server client server2 server_bind server_bind_simple server_bind_epoll client_bind 

all: clean $(TARGETS)

server: server.c
	$(CC) $(CFLAGS) -o server.o server.c

server2: server2.c
	$(CC) $(CFLAGS) -o server2.o server2.c

server_bind: server_bind.c
	$(CC) $(CFLAGS) -o server_bind.o server_bind.c

server_bind_simple: server_bind_simple.c
	$(CC) $(CFLAGS) -o server_bind_simple.o server_bind_simple.c

server_bind_epoll: server_bind_epoll.c
	$(CC) $(CFLAGS) -o server_bind_epoll.o server_bind_epoll.c

client_bind: client_bind.c
	$(CC) $(CFLAGS) -o client_bind.o client_bind.c

client_bind_simple: client_bind_simple.c
	$(CC) $(CFLAGS) -o client_bind_simple.o client_bind_simple.c

client: client.c
	$(CC) $(CFLAGS) -o client.o client.c

clean:
	rm -f $(TARGETS)

.PHONY: all clean


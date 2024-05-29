

# Makefile for building server
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread

# Targets
TARGETS = server client server2

all: clean $(TARGETS)

server: server.c
	$(CC) $(CFLAGS) -o server.o server.c

server2: server2.c
	$(CC) $(CFLAGS) -o server2.o server2.c

client: client.c
	$(CC) $(CFLAGS) -o client.o client.c

clean:
	rm -f $(TARGETS)

.PHONY: all clean


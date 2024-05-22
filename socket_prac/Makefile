# Define compilers and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2

# Define directories
SERVER_DIR = server
CLIENT_DIR = client

# Define targets
SERVER_TARGET = $(SERVER_DIR)/server
CLIENT_TARGET = $(CLIENT_DIR)/client

# Define source files
SERVER_SRC = $(SERVER_DIR)/server.c
CLIENT_SRC = $(CLIENT_DIR)/client.c

# Default target to build both server and client
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# Build server
$(SERVER_TARGET): $(SERVER_SRC)
	@echo "Building server..."
	$(CC) $(CFLAGS) -o $(SERVER_TARGET) $(SERVER_SRC)
	@echo "Server built successfully."

# Build client
$(CLIENT_TARGET): $(CLIENT_SRC)
	@echo "Building client..."
	$(CC) $(CFLAGS) -o $(CLIENT_TARGET) $(CLIENT_SRC)
	@echo "Client built successfully."

# Clean up
clean:
	@echo "Cleaning up..."
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)
	@echo "Clean up done."

.PHONY: all clean

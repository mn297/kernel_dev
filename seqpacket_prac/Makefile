# Makefile for building server and client applications
# Compiler to use
CC = gcc

# Compiler flags for warnings, C standard, and pthread support
CFLAGS = -Wall -Wextra -std=c11 -pthread

# Source files
SOURCES = \
    server.c \
    client.c \
    server2.c \
    server_bind.c \
    server_bind_simple.c \
    server_bind_epoll.c \
    server_bind_epoll_cb.c \
    client_bind.c \
    client_bind_simple.c \
    client_bind_epoll_cb.c

# Executable files
# This line creates a list of executables by substituting .c with .o in the SOURCES list
EXECUTABLES = $(SOURCES:.c=.o)

# The 'all' target depends on all executables defined in EXECUTABLES
all: $(EXECUTABLES)

# Pattern rule for compiling source files into executables with .o extension
# %.o: %.c is a pattern rule. It tells Make how to create a .o file from a .c file.
# % is a wildcard that matches any stem. In this context, it means that any .c file 
# can be transformed into a .o file using the specified recipe.
# $(CC) $(CFLAGS) -o $@ $< compiles the .c file into an executable with a .o extension.
# $@ represents the target file of the rule (the .o file).
# $< represents the first prerequisite (the .c file).
%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

# The 'clean' target removes all compiled executables
clean:
	rm -f $(EXECUTABLES)

# Declares 'all' and 'clean' as phony targets
.PHONY: all clean

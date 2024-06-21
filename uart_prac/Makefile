# Makefile for building server and client applications and kernel module
# Compiler to use
CC = gcc

# Compiler flags for warnings, C standard, and pthread support
CFLAGS = -Wall -Wextra -std=c11 -pthread

# Source files
SOURCES = \
	uart_mkfifo.c \
	uart_pty.c

# Executable files
EXECUTABLES = $(SOURCES:.c=.o)

# Kernel module sources
KERNEL_MODULE = uart_chardev

# The 'all' target depends on all executables defined in EXECUTABLES and the kernel module
# all: $(EXECUTABLES) $(KERNEL_MODULE).ko
all: $(EXECUTABLES)

# Pattern rule for compiling source files into executables with .o extension
%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

# Build the kernel module
$(KERNEL_MODULE).ko: $(KERNEL_MODULE).o
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

# Clean up all generated files
clean:
	rm -f $(EXECUTABLES) $(KERNEL_MODULE).o $(KERNEL_MODULE).mod.o $(KERNEL_MODULE).mod.c $(KERNEL_MODULE).ko $(KERNEL_MODULE).order $(KERNEL_MODULE).symvers

# Declares 'all' and 'clean' as phony targets
.PHONY: all clean

# Additional target to clean the kernel module artifacts
clean-kernel:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

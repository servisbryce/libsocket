#LIBRARY_NAME = socket
#SRC_FILES = src/main.c
#OBJ_FILES = $(SRC_FILES:.c=.o)
#CC = gcc
#CFLAGS = -Wall -fPIC
#all: $(LIBRARY_NAME).so

#%.o: %.c
#	$(CC) $(CFLAGS) -c %< -o $@

#$(LIBRARY_NAME).so: $(OBJ)
#	$(CC) -shared -Wl,-soname,$(LIBRARY_NAME).so -o $(LIBRARY_NAME).so $(OBJ)

#clean:
#	rm -f $(OBJ) $(LIBRARY_NAME).so*

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -Iinclude -I/usr/include
LDFLAGS = -L/usr/include -l ssl -l crypto

# Source files
SRCS = $(wildcard src/*/*.c) src/libsocket.c

# Object files
OBJS = $(SRCS:.c=.o)

# Executable name
EXEC = fwd

# Default target
all: $(EXEC)

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link object files to create the executable
$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC) ${LDFLAGS}

# Clean build files
clean:
	rm -f $(OBJS) $(EXEC)
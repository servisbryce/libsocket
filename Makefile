LIBRARY_NAME = socket
SRC_FILES = src/main.c
OBJ_FILES = $(SRC_FILES:.c=.o)
CC = gcc
CFLAGS = -Wall -fPIC
all: $(LIBRARY_NAME).so

%.o: %.c
	$(CC) $(CFLAGS) -c %< -o $@

$(LIBRARY_NAME).so: $(OBJ)
	$(CC) -shared -Wl,-soname,$(LIBRARY_NAME).so -o $(LIBRARY_NAME).so $(OBJ)

clean:
	rm -f $(OBJ) $(LIBRARY_NAME).so*
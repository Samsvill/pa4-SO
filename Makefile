# Makefile

CC = gcc
CFLAGS = -Wall -pthread
TARGETS = publicador desenfocador realzador

all: $(TARGETS)

publicador: publicador.c bmp.h
	$(CC) $(CFLAGS) -o publicador publicador.c

desenfocador: desenfocador.c bmp.h
	$(CC) $(CFLAGS) -o desenfocador desenfocador.c

realzador: realzador.c bmp.h
	$(CC) $(CFLAGS) -o realzador realzador.c

clean:
	rm -f $(TARGETS)


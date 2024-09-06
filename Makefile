# Makefile

# Nombre del compilador
CC = gcc

# Bandera para la depuración y las bibliotecas de hilos
CFLAGS = -g -lpthread

# Fuentes comunes
BMP_SRC = bmp.c

# Objetivos
all: publicador realzador desenfocador combinador

# Reglas para cada programa
publicador: publicador.c $(BMP_SRC)
	$(CC) -o publicador publicador.c $(BMP_SRC) $(CFLAGS)

realzador: realzador.c $(BMP_SRC) filter.c
	$(CC) -o realzador realzador.c $(BMP_SRC) filter.c $(CFLAGS)

desenfocador: desenfocador.c $(BMP_SRC) filter.c
	$(CC) -o desenfocador desenfocador.c $(BMP_SRC) filter.c $(CFLAGS)

combinador: combinador.c $(BMP_SRC)
	$(CC) -o combinador combinador.c $(BMP_SRC) $(CFLAGS)

# Limpieza
clean:
	rm -f publicador realzador desenfocador combinador

# Makefile TP2 - Tetris (Estructuras de Memoria Dinamica)

# Compilador de C a utilizar
CC = gcc

# Flags para el compilador:
# -g: Incluye informacion de debugging (para usar con gdb y valgrind)
# -Wall: Para que aparezcan todos los warnings
# `pkg-config ...`: Obtiene automaticamente las rutas de los headers de SDL2
CFLAGS = -g -Wall `pkg-config --cflags sdl2`

# Flags para el linker:
LIBS = `pkg-config --libs sdl2`

# Nombre del archivo ejecutable que se generara
EXEC = tetris

# Archivos fuente (.c) a compilar.
# Descomentar la linea correspondiente al archivo que se quiera compilar:

#SRCS = juego_base.c
#SRCS = solucion_esqueleto.c
SRCS = tp_resuelto.c

# --- Reglas de Compilacion ---

all: $(EXEC)

$(EXEC): $(SRCS)
	$(CC) $(CFLAGS) -o $(EXEC) $(SRCS) $(LIBS)

run: $(EXEC)
	./$(EXEC)

# juego_base_v2.c no tiene tests: estos targets son para el esqueleto y el resuelto.
test: $(EXEC)
	./$(EXEC) --test

valgrind: $(EXEC)
	valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./$(EXEC) --test

clean:
	rm -f $(EXEC)

.PHONY: all run test valgrind clean

UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)

ifeq ($(UNAME_S),Darwin)
    # macOS con Apple Silicon: gcc normal es un alias de Apple Clang
    # (no soporta OpenMP), por eso usamos el gcc real de Homebrew.
    CC = gcc-16
else ifeq ($(UNAME_S),Linux)
    CC = gcc
else
    # Windows via WSL o MSYS2/MinGW tambien entra por aca
    CC = gcc
endif

CFLAGS = -fopenmp -Wall -O2
TARGET = ecosistema
SRCS = main.c ecosistema.c especies.c resultados.c

all: $(TARGET)

$(TARGET): $(SRCS) ecosistema.h especies.h resultados.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean
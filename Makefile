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
HDRS = ecosistema.h especies.h resultados.h

# Binario aparte para medir eficiencia: cuadricula grande (con 10x10 el costo
# de crear los hilos domina y no hay speedup que medir) y schedule(runtime),
# que permite elegir static/dynamic/guided con OMP_SCHEDULE sin recompilar.
BENCH_TARGET = ecosistema_bench
BENCH_GRID  ?= 1000
BENCH_TICKS ?= 50
BENCH_FLAGS = -DGRID_SIZE=$(BENCH_GRID) -DTICKS=$(BENCH_TICKS) -DPLANIFICACION_RUNTIME

all: $(TARGET)

$(TARGET): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

run: $(TARGET)
	./$(TARGET)

$(BENCH_TARGET): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) $(BENCH_FLAGS) -o $(BENCH_TARGET) $(SRCS)

# Tabla de tiempos y speedup (hilos x schedule) -> benchmark.md
bench: $(BENCH_TARGET) $(TARGET)
	./benchmark.sh

clean:
	rm -f $(TARGET) $(BENCH_TARGET)

.PHONY: all run bench clean
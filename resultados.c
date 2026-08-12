#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include "ecosistema.h"
#include "resultados.h"

#define ARCHIVO_RESULTADOS "resultados.txt"

/* mostrar_estado corre FUERA de la region paralela: ecosistema.c la llama
 * despues del parallel for, o sea pasada la barrera implicita, asi que un
 * solo hilo toca este FILE*. Por eso no hace falta critical ni atomic aca. */
static FILE *archivo = NULL;

static void cerrar_archivo(void) {
    if (archivo != NULL) {
        fclose(archivo);
        archivo = NULL;
    }
}

/* para el benchmark: con ECO_SILENCIOSO=1 no se imprime ni se escribe nada,
 * asi omp_get_wtime cronometra el computo y no la consola. */
static int silencioso(void) {
    static int cache = -1;
    if (cache < 0) {
        cache = (getenv("ECO_SILENCIOSO") != NULL);
    }
    return cache;
}

static char simbolo(TipoEspecie especie) {
    switch (especie) {
        case PLANTA:    return 'P';
        case HERBIVORO: return 'H';
        case CARNIVORO: return 'C';
        default:        return '.';
    }
}

/* un solo formateador para consola y archivo: asi los dos no se despegan */
static void volcar_estado(FILE *salida, int tick) {
    fprintf(salida, "\n--- Tick %d ---\n", tick);
    fprintf(salida, "Plantas: %d | Herbivoros: %d | Carnivoros: %d\n",
            num_plantas, num_herbivoros, num_carnivoros);
    fprintf(salida, "Distribucion:\n");

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            fprintf(salida, "%c ", simbolo(grid_actual[i][j].especie));
        }
        fprintf(salida, "\n");
    }
}

void mostrar_estado(int tick) {
    if (silencioso()) return;

    volcar_estado(stdout, tick);

    /* se abre una sola vez (primer llamado) y queda abierto hasta el final;
     * mas barato que reabrir en modo append en cada uno de los TICKS. */
    if (archivo == NULL) {
        archivo = fopen(ARCHIVO_RESULTADOS, "w");
        if (archivo == NULL) {
            perror("No se pudo abrir " ARCHIVO_RESULTADOS);
            return;
        }
        atexit(cerrar_archivo);
        fprintf(archivo, "Simulacion de ecosistema con OpenMP\n");
        fprintf(archivo, "Cuadricula: %dx%d | Ticks: %d | Hilos: %d\n",
                GRID_SIZE, GRID_SIZE, TICKS, omp_get_max_threads());
        fprintf(archivo, "Leyenda: P=planta  H=herbivoro  C=carnivoro  .=vacio\n");
    }

    volcar_estado(archivo, tick);
}

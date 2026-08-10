#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define GRID_SIZE 10

typedef enum { VACIO, PLANTA, HERBIVORO, CARNIVORO } TipoEspecie;

typedef struct {
    TipoEspecie especie;
    int energia;
} Celda;

Celda grid_actual[GRID_SIZE][GRID_SIZE];
Celda grid_siguiente[GRID_SIZE][GRID_SIZE];

int num_plantas = 0, num_herbivoros = 0, num_carnivoros = 0;

void inicializar_ecosistema(void) {
    srand(42); /* semilla fija para que los 3 obtengan el mismo resultado */
    num_plantas = num_herbivoros = num_carnivoros = 0;

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            int r = rand() % 100;
            if (r < 30) {
                grid_actual[i][j] = (Celda){PLANTA, 5};
                num_plantas++;
            } else if (r < 45) {
                grid_actual[i][j] = (Celda){HERBIVORO, 10};
                num_herbivoros++;
            } else if (r < 50) {
                grid_actual[i][j] = (Celda){CARNIVORO, 15};
                num_carnivoros++;
            } else {
                grid_actual[i][j] = (Celda){VACIO, 0};
            }
        }
    }
    memcpy(grid_siguiente, grid_actual, sizeof(grid_actual));
}

void mostrar_estado(int tick) {
    printf("\n--- Tick %d ---\n", tick);
    printf("Plantas: %d | Herbivoros: %d | Carnivoros: %d\n",
           num_plantas, num_herbivoros, num_carnivoros);

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            char c = '.';
            switch (grid_actual[i][j].especie) {
                case PLANTA:     c = 'P'; break;
                case HERBIVORO:  c = 'H'; break;
                case CARNIVORO:  c = 'C'; break;
                default: break;
            }
            printf("%c ", c);
        }
        printf("\n");
    }
}

int main(void) {
    printf("Hilos disponibles: %d\n", omp_get_max_threads());

    inicializar_ecosistema();
    mostrar_estado(0);

    return 0;
}
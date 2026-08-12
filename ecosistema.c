#include <stdlib.h>
#include <string.h>
#include "ecosistema.h"
#include "especies.h"
#include "resultados.h"

Celda grid_actual[GRID_SIZE][GRID_SIZE];
Celda grid_siguiente[GRID_SIZE][GRID_SIZE];

int num_plantas = 0;
int num_herbivoros = 0;
int num_carnivoros = 0;

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

void simular_tick(int tick) {
    /* schedule(dynamic) porque el trabajo por celda no es uniforme:
     * una celda vacia es barata, una con carnivoro cazando es cara.
     *
     * Compilando con -DPLANIFICACION_RUNTIME se cambia a schedule(runtime),
     * que toma la planificacion de la variable OMP_SCHEDULE: asi el benchmark
     * compara static/dynamic/guided sin recompilar ni tocar esta linea. */
#ifdef PLANIFICACION_RUNTIME
    #pragma omp parallel for collapse(2) schedule(runtime)
#else
    #pragma omp parallel for collapse(2) schedule(dynamic)
#endif
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            actualizar_celda(i, j, tick);
        }
    }
    /* Barrera implicita: al salir del parallel for, todos los hilos
     * ya terminaron. No hace falta #pragma omp barrier explicito aqui. */

    memcpy(grid_actual, grid_siguiente, sizeof(grid_actual));
    mostrar_estado(tick);
}
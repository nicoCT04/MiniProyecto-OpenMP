#include <stdio.h>
#include "ecosistema.h"
#include "resultados.h"

/* TODO (compañero 3): además de imprimir en consola, guardar este
 * mismo estado en un archivo de resultados (formato del ejemplo del PDF). */
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
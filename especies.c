#include "ecosistema.h"
#include "especies.h"

void actualizar_celda(int i, int j) {
    Celda actual = grid_actual[i][j];

    switch (actual.especie) {
        case PLANTA:
            /* TODO: reglas de reproduccion (30% a celda vecina vacia) */
            grid_siguiente[i][j] = actual;
            break;
        case HERBIVORO:
            /* TODO: buscar planta adyacente -> comer -> ganar energia
             *       si no hay comida en 3 ticks -> muere
             *       reproducirse si hay energia y espacio */
            grid_siguiente[i][j] = actual;
            break;
        case CARNIVORO:
            /* TODO: cazar herbivoro adyacente -> ganar energia */
            grid_siguiente[i][j] = actual;
            break;
        case VACIO:
        default:
            grid_siguiente[i][j] = actual;
            break;
    }
}
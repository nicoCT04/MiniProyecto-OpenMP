#include <stdio.h>
#include <omp.h>
#include "ecosistema.h"
#include "resultados.h"

int main(void) {
    printf("Hilos disponibles: %d\n", omp_get_max_threads());

    inicializar_ecosistema();
    mostrar_estado(0);

    for (int t = 1; t <= TICKS; t++) {
        simular_tick(t);
    }

    return 0;
}
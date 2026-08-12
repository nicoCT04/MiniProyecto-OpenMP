#include <stdio.h>
#include <omp.h>
#include "ecosistema.h"
#include "resultados.h"

int main(void) {
    printf("Hilos disponibles: %d\n", omp_get_max_threads());

    inicializar_ecosistema();
    mostrar_estado(0);

    /* omp_get_wtime da tiempo de pared en segundos; se mide solo el bucle
     * de ticks (la inicializacion es secuencial y no aporta al speedup).
     * Para que el numero signifique algo hay que correr con ECO_SILENCIOSO=1,
     * si no se estaria cronometrando la consola y no el computo paralelo. */
    double t0 = omp_get_wtime();
    for (int t = 1; t <= TICKS; t++) {
        simular_tick(t);
    }
    double t1 = omp_get_wtime();

    printf("\nTiempo total: %.6f s\n", t1 - t0);
    return 0;
}

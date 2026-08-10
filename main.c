#include <stdio.h>
#include <omp.h>

int main(void) {
    printf("Hilos disponibles: %d\n", omp_get_max_threads());
    return 0;
}
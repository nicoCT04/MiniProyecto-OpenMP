#ifndef ECOSISTEMA_H
#define ECOSISTEMA_H

/* Valores del enunciado. Se dejan sobreescribibles con -DGRID_SIZE=... para
 * el benchmark de eficiencia: con 100 celdas el costo de crear los hilos
 * domina y no se puede medir speedup. La logica no cambia, solo el tamano. */
#ifndef GRID_SIZE
#define GRID_SIZE 10
#endif

#ifndef TICKS
#define TICKS 20
#endif

typedef enum { VACIO, PLANTA, HERBIVORO, CARNIVORO } TipoEspecie;

typedef struct {
    TipoEspecie especie;
    int energia;
} Celda;

/* Doble buffer: grid_actual es solo lectura durante el tick,
 * grid_siguiente es donde cada hilo escribe su resultado. */
extern Celda grid_actual[GRID_SIZE][GRID_SIZE];
extern Celda grid_siguiente[GRID_SIZE][GRID_SIZE];

/* Contadores globales de poblacion. Se actualizan desde especies.c
 * (usar #pragma omp atomic) y se leen desde resultados.c */
extern int num_plantas;
extern int num_herbivoros;
extern int num_carnivoros;

void inicializar_ecosistema(void);
void simular_tick(int tick);

#endif
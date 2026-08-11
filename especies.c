#include <stdlib.h>
#include "ecosistema.h"
#include "especies.h"

/* --- parametros del modelo ---
 * los del PDF son fijos (30%, +1, +2, 3 ticks sin comer).
 * el resto los elegimos nosotros y quedan aca para poder calibrar
 * si la poblacion colapsa o explota. */
#define PROB_REPRODUCCION_PLANTA 30
#define ENERGIA_CRIA_PLANTA       5
#define COSTO_POR_TICK            1

/* modelamos hambre y vejez sobre la energia para no meterle campos
 * nuevos a Celda: cada tick se resta COSTO_POR_TICK y comer la
 * recupera, asi que quedarse sin energia cubre los dos casos. */

/* lo unico que distingue a un herbivoro de un carnivoro son estos
 * valores; el algoritmo es el mismo, por eso va en una tabla y no
 * duplicado en dos funciones. agregar un omnivoro seria una fila mas. */
typedef struct {
    TipoEspecie presa;
    int ganancia_al_comer;
    int energia_para_reproducirse;
    int *contador;
} RasgosEspecie;

static const RasgosEspecie RASGOS[] = {
    [HERBIVORO] = { PLANTA,    1, 15, &num_herbivoros },
    [CARNIVORO] = { HERBIVORO, 2, 20, &num_carnivoros },
};

/* 8 vecinos: incluye diagonales */
static const int DI[8] = {-1, -1, -1,  0, 0,  1, 1, 1};
static const int DJ[8] = {-1,  0,  1, -1, 1, -1, 0, 1};

static int dentro_del_grid(int i, int j) {
    /* bordes con pared: las celdas de la orilla tienen menos vecinos */
    return i >= 0 && i < GRID_SIZE && j >= 0 && j < GRID_SIZE;
}

/* rand_r con semilla propia por celda y tick. rand() no es thread-safe
 * y ademas rompe la reproducibilidad del srand(42) al paralelizar. */
static unsigned int semilla_de(int i, int j, int tick) {
    return (unsigned int)((i * 73856093) ^ (j * 19349663) ^ (tick * 83492791));
}

/* busca un vecino del tipo pedido leyendo grid_actual (solo lectura
 * durante el tick). si hay varios elige uno al azar en una sola pasada
 * (reservoir sampling), asi no todos van siempre al mismo lado.
 * devuelve 1 y deja la posicion en *vi,*vj. */
static int buscar_vecino(int i, int j, TipoEspecie tipo,
                         unsigned int *semilla, int *vi, int *vj) {
    int vistos = 0;

    for (int k = 0; k < 8; k++) {
        int ni = i + DI[k];
        int nj = j + DJ[k];
        if (!dentro_del_grid(ni, nj) || grid_actual[ni][nj].especie != tipo) {
            continue;
        }
        vistos++;
        /* el k-esimo candidato se queda con probabilidad 1/k */
        if (rand_r(semilla) % vistos == 0) {
            *vi = ni;
            *vj = nj;
        }
    }
    return vistos > 0;
}

/* igual que buscar_vecino pero sin azar: se queda con el primero en el
 * orden de DI/DJ. lo usan depredador y presa para elegir comida, porque
 * ambos tienen que llegar a la misma conclusion sin hablarse (si el
 * depredador eligiera al azar, la presa no podria adivinar a quien). */
static int buscar_presa(int i, int j, TipoEspecie tipo, int *vi, int *vj) {
    for (int k = 0; k < 8; k++) {
        int ni = i + DI[k];
        int nj = j + DJ[k];
        if (dentro_del_grid(ni, nj) && grid_actual[ni][nj].especie == tipo) {
            *vi = ni;
            *vj = nj;
            return 1;
        }
    }
    return 0;
}

/* la presa se pregunta si la estan comiendo: recorre sus vecinos y ve si
 * alguno es depredador suyo y la eligio a ella con buscar_presa. como
 * ambos lados evaluan la misma regla sobre grid_actual, coinciden. */
static int me_estan_comiendo(int i, int j, TipoEspecie yo) {
    for (int k = 0; k < 8; k++) {
        int ni = i + DI[k];
        int nj = j + DJ[k];
        if (!dentro_del_grid(ni, nj)) continue;

        TipoEspecie vecino = grid_actual[ni][nj].especie;
        if (vecino != HERBIVORO && vecino != CARNIVORO) continue;
        if (RASGOS[vecino].presa != yo) continue;

        int pi, pj;
        if (buscar_presa(ni, nj, yo, &pi, &pj) && pi == i && pj == j) {
            return 1;
        }
    }
    return 0;
}

/* escribe en una celda vecina, que es de otro hilo. el critical serializa
 * esta parte pero es lo que evita que dos hilos pisen la misma celda.
 * gana el primero que entra: el segundo ve que ya esta ocupada y desiste,
 * y asi queda resuelta la competencia por recursos del PDF. */
static int ocupar_celda_vecina(int vi, int vj, TipoEspecie especie, int energia) {
    int ocupada = 0;

    #pragma omp critical (escritura_vecina)
    {
        if (grid_siguiente[vi][vj].especie == VACIO) {
            grid_siguiente[vi][vj].especie = especie;
            grid_siguiente[vi][vj].energia = energia;
            ocupada = 1;
        }
    }
    return ocupada;
}

static void morir(int i, int j, int *contador) {
    #pragma omp atomic
    (*contador)--;
    grid_siguiente[i][j] = (Celda){VACIO, 0};
}

static void actualizar_planta(int i, int j, Celda actual, unsigned int *semilla) {
    int vi, vj;

    /* si un herbivoro la eligio, la planta desaparece: es el otro lado
     * de la regla de consumo, el que la hace efectiva */
    if (me_estan_comiendo(i, j, PLANTA)) {
        morir(i, j, &num_plantas);
        return;
    }

    /* la planta no se mueve, solo intenta expandirse a una vecina vacia */
    if (buscar_vecino(i, j, VACIO, semilla, &vi, &vj) &&
        rand_r(semilla) % 100 < PROB_REPRODUCCION_PLANTA &&
        ocupar_celda_vecina(vi, vj, PLANTA, ENERGIA_CRIA_PLANTA)) {
        #pragma omp atomic
        num_plantas++;
    }

    grid_siguiente[i][j] = actual;
}

/* herbivoros y carnivoros comparten algoritmo; lo que cambia sale de RASGOS */
static void actualizar_animal(int i, int j, Celda actual, unsigned int *semilla) {
    const RasgosEspecie *rasgos = &RASGOS[actual.especie];
    int vi, vj, hi, hj;

    /* lo primero: si un carnivoro lo cazo, ya no come ni huye ni se
     * reproduce. solo aplica al herbivoro, nadie depreda al carnivoro. */
    if (me_estan_comiendo(i, j, actual.especie)) {
        morir(i, j, rasgos->contador);
        return;
    }

    /* huir tiene prioridad sobre comer. cada busqueda usa sus propias
     * coordenadas para no pisar las del otro candidato. */
    if (actual.especie == HERBIVORO &&
        buscar_vecino(i, j, CARNIVORO, semilla, &hi, &hj) &&
        buscar_vecino(i, j, VACIO, semilla, &vi, &vj)) {
        actual.energia -= COSTO_POR_TICK;
        if (actual.energia <= 0) {
            morir(i, j, rasgos->contador);
            return;
        }
        if (ocupar_celda_vecina(vi, vj, actual.especie, actual.energia)) {
            grid_siguiente[i][j] = (Celda){VACIO, 0};
            return;
        }
    }

    /* determinista a proposito: la presa aplica esta misma regla para
     * saber que la eligieron a ella */
    if (buscar_presa(i, j, rasgos->presa, &vi, &vj)) {
        actual.energia += rasgos->ganancia_al_comer;
    }
    actual.energia -= COSTO_POR_TICK;

    /* sin energia: da igual si fue por hambre o por vejez */
    if (actual.energia <= 0) {
        morir(i, j, rasgos->contador);
        return;
    }

    /* la cria se lleva la mitad de la energia del padre, no energia nueva */
    if (actual.energia >= rasgos->energia_para_reproducirse &&
        buscar_vecino(i, j, VACIO, semilla, &vi, &vj)) {
        int energia_cria = actual.energia / 2;
        if (ocupar_celda_vecina(vi, vj, actual.especie, energia_cria)) {
            actual.energia -= energia_cria;
            #pragma omp atomic
            (*rasgos->contador)++;
        }
    }

    grid_siguiente[i][j] = actual;
}

void actualizar_celda(int i, int j, int tick) {
    Celda actual = grid_actual[i][j];
    unsigned int semilla = semilla_de(i, j, tick);

    switch (actual.especie) {
        case PLANTA:
            actualizar_planta(i, j, actual, &semilla);
            break;
        case HERBIVORO:
        case CARNIVORO:
            actualizar_animal(i, j, actual, &semilla);
            break;
        case VACIO:
        default:
            /* la celda vacia no hace nada, pero hay que limpiarla porque
             * grid_siguiente arrastra el tick anterior. si un vecino ya
             * puso una cria aca, se respeta. */
            #pragma omp critical (escritura_vecina)
            {
                if (grid_siguiente[i][j].especie == VACIO) {
                    grid_siguiente[i][j].energia = 0;
                }
            }
            break;
    }
}

#ifndef ESPECIES_H
#define ESPECIES_H

/* el tick entra como semilla del rand_r por celda, asi la corrida
 * es reproducible aunque cambie el numero de hilos */
void actualizar_celda(int i, int j, int tick);

#endif
# Medicion de eficiencia

Mejor de 3 corridas, salida desactivada con `ECO_SILENCIOSO=1`.
Generado por `benchmark.sh`.

## Tiempo total (s), cuadricula grande

| Hilos | static | dynamic | guided |
|---|---|---|---|
| 1 | 1.433134 | 2.271407 | 1.757545 |
| 2 | 1.010899 | 2.728902 | 0.972457 |
| 4 | 0.876106 | 2.644216 | 0.885334 |
| 8 | 1.327017 | 3.167007 | 1.341981 |

## Speedup vs 1 hilo

| Hilos | static | dynamic | guided |
|---|---|---|---|
| 1 | 1.00x | 1.00x | 1.00x |
| 2 | 1.42x | 0.83x | 1.81x |
| 4 | 1.64x | 0.86x | 1.99x |
| 8 | 1.08x | 0.72x | 1.31x |

## Efecto del tamano de bloque (8 hilos)

| OMP_SCHEDULE | tiempo (s) |
|---|---|
| `dynamic` | 3.372360 |
| `dynamic,64` | 1.570583 |
| `dynamic,1000` | 1.592064 |
| `static` | 1.310825 |
| `static,1000` | 1.413849 |

## Cuadricula del enunciado (10x10, 20 ticks)

| Hilos | tiempo (s) |
|---|---|
| 1 | 0.000124 |
| 2 | 0.000448 |
| 4 | 0.000823 |
| 8 | 0.001392 |

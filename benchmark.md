# Medicion de eficiencia

Mejor de 3 corridas, salida desactivada con `ECO_SILENCIOSO=1`.
Generado por `benchmark.sh`.

## Tiempo total (s), cuadricula grande

| Hilos | static | dynamic | guided |
|---|---|---|---|
| 1 | 1.498272 | 1.873126 | 1.432794 |
| 2 | 1.059465 | 2.581817 | 1.026482 |
| 4 | 1.113235 | 3.140033 | 1.074087 |
| 8 | 1.513899 | 3.868240 | 1.526839 |

## Speedup vs 1 hilo

| Hilos | static | dynamic | guided |
|---|---|---|---|
| 1 | 1.00x | 1.00x | 1.00x |
| 2 | 1.41x | 0.73x | 1.40x |
| 4 | 1.35x | 0.60x | 1.33x |
| 8 | 0.99x | 0.48x | 0.94x |

## Efecto del tamano de bloque (8 hilos)

| OMP_SCHEDULE | tiempo (s) |
|---|---|
| `dynamic` | 3.828411 |
| `dynamic,64` | 1.870303 |
| `dynamic,1000` | 1.896471 |
| `static` | 1.515189 |
| `static,1000` | 1.586204 |

## Cuadricula del enunciado (10x10, 20 ticks)

| Hilos | tiempo (s) |
|---|---|
| 1 | 0.000131 |
| 2 | 0.000282 |
| 4 | 0.000309 |
| 8 | 0.000633 |

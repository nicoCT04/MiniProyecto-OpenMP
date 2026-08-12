# Simulación de Ecosistema con OpenMP

Simulación de un ecosistema en una cuadrícula 10x10 donde conviven plantas,
herbívoros y carnívoros durante 20 ticks. Cada celda se actualiza en paralelo
con OpenMP.

- Reglas y decisiones de diseño: [INFORME.md](INFORME.md)
- Tabla de tiempos y speedup: [benchmark.md](benchmark.md)
- Estado de la simulación tick por tick: `resultados.txt` (lo genera la corrida)

## Requisitos

Un `gcc` con soporte de OpenMP.

| Sistema | Compilador |
|---|---|
| Linux / WSL | `gcc` (viene con OpenMP) |
| macOS (Apple Silicon) | `gcc-16` de Homebrew: `brew install gcc` |
| Windows | MSYS2 / MinGW, o WSL |

En macOS el `gcc` del sistema es un alias de Apple Clang, que **no** trae
OpenMP; por eso el `Makefile` usa `gcc-16` cuando detecta Darwin.

## Compilar y ejecutar

```bash
git clone https://github.com/nicoCT04/MiniProyecto-OpenMP.git
cd MiniProyecto-OpenMP
make run
```

Esto imprime el estado inicial y los 20 ticks en consola, y deja los mismos
21 estados en `resultados.txt`.

Para compilar y ejecutar por separado:

```bash
make
./ecosistema
```

## Número de hilos

Se controla con la variable de entorno `OMP_NUM_THREADS`, sin recompilar:

```bash
OMP_NUM_THREADS=4 ./ecosistema
```

El resultado **no depende** del número de hilos: cada celda genera sus números
aleatorios con `rand_r` y una semilla derivada de su posición y del tick, así
que 1, 2, 4 u 8 hilos dan exactamente la misma simulación (Tick 20 = 64 plantas,
4 herbívoros, 0 carnívoros).

## Medición de eficiencia

```bash
make bench
```

Construye un binario aparte (`ecosistema_bench`, cuadrícula de 1000x1000 y
50 ticks) y corre `benchmark.sh`, que mide todas las combinaciones de hilos
(1/2/4/8) y schedule (static/dynamic/guided) y escribe `benchmark.md`.

Se usa una cuadrícula grande porque con las 100 celdas del enunciado el costo
de crear los hilos es mayor que la simulación entera, y no habría speedup que
medir. El binario normal no cambia.

Variables útiles:

| Variable | Para qué |
|---|---|
| `OMP_NUM_THREADS` | número de hilos |
| `OMP_SCHEDULE` | schedule del benchmark (`static`, `dynamic,64`, `guided`, …) |
| `ECO_SILENCIOSO=1` | apaga consola y archivo, para cronometrar solo el cómputo |

Se puede cambiar el tamaño del benchmark:

```bash
make bench BENCH_GRID=2000 BENCH_TICKS=100
```

## Estructura

| Archivo | Contenido |
|---|---|
| `main.c` | bucle de ticks y cronómetro |
| `ecosistema.c/.h` | cuadrícula de doble buffer, inicialización y `parallel for` |
| `especies.c/.h` | reglas de plantas, herbívoros y carnívoros |
| `resultados.c/.h` | impresión en consola y escritura de `resultados.txt` |
| `benchmark.sh` | mediciones de eficiencia |

## Problemas comunes

**`make` falla con "No such file or directory"** en algún `.h`. Los tres
encabezados tienen que llamarse exactamente `ecosistema.h`, `especies.h` y
`resultados.h`, en minúsculas y sin acentos: el `Makefile` los lista como
dependencias y en Linux/macOS los nombres distinguen mayúsculas.

**`unsupported option '-fopenmp'`** en macOS: se está usando Apple Clang.
Instalar `gcc` con Homebrew y verificar que la versión coincida con la del
`Makefile` (`gcc-16`); si no, ajustar ahí el nombre.

**`make clean`** borra los binarios. `resultados.txt` y `benchmark.md` se
conservan porque son entregables.

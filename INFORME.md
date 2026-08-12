# Informe — Simulación de Ecosistema con OpenMP

Simulación de un ecosistema de plantas, herbívoros y carnívoros sobre una
cuadrícula de 10x10 durante 20 ticks, paralelizada con OpenMP.

---

## 1. Diseño del sistema

### 1.1 Cuadrícula de doble buffer

La estructura central son dos matrices del mismo tamaño (`ecosistema.h`):

```c
extern Celda grid_actual[GRID_SIZE][GRID_SIZE];    /* solo lectura en el tick */
extern Celda grid_siguiente[GRID_SIZE][GRID_SIZE]; /* donde escriben los hilos */
```

Durante un tick, **nadie escribe en `grid_actual`**: todos los hilos lo leen para
consultar sus vecinos y escriben el resultado en `grid_siguiente`. Al cerrar la
región paralela se copia una sobre la otra con `memcpy` ([ecosistema.c:58](ecosistema.c#L58)).

Esta es la decisión que hace correcta la paralelización. Con un solo buffer, el
resultado de una celda dependería de si su vecina ya fue procesada o no, y eso
depende del reparto de hilos: la simulación daría un resultado distinto en cada
corrida. Con doble buffer, todas las celdas ven el **mismo** estado de partida y
el orden de ejecución deja de importar.

Cada `Celda` guarda solo dos campos, especie y energía:

```c
typedef struct {
    TipoEspecie especie;   /* VACIO, PLANTA, HERBIVORO, CARNIVORO */
    int energia;
} Celda;
```

### 1.2 Vecindad

Se usa vecindad de Moore (los 8 vecinos, incluidas las diagonales), recorrida con
las tablas `DI`/`DJ` de [especies.c:33-34](especies.c#L33-L34). Los bordes se
tratan como pared: una celda de la orilla simplemente tiene menos vecinos, no se
envuelve hacia el otro lado.

---

## 2. Decisiones de diseño

### 2.1 Un solo algoritmo para herbívoros y carnívoros

Herbívoro y carnívoro hacen exactamente lo mismo: buscar su presa, comerla,
gastar energía, reproducirse si les alcanza. Lo único que cambia es *qué* comen y
*cuánto* ganan. En vez de duplicar la función, esas diferencias viven en una
tabla ([especies.c:27-30](especies.c#L27-L30)):

```c
static const RasgosEspecie RASGOS[] = {
    [HERBIVORO] = { PLANTA,    1, 15, &num_herbivoros },
    [CARNIVORO] = { HERBIVORO, 2, 20, &num_carnivoros },
};
```

`actualizar_animal` sirve para los dos. Agregar un omnívoro sería una fila más,
no una función más.

### 2.2 Hambre y vejez sobre un solo campo

El enunciado pide que un animal muera si no come en 3 ticks, y también por vejez.
En vez de agregar contadores separados a `Celda`, ambas cosas se modelan sobre
`energia`: cada tick se resta `COSTO_POR_TICK` y comer la recupera. Quedarse sin
energía cubre los dos casos con una sola comparación, y la energía inicial fija
cuánto se aguanta sin comer.

### 2.3 Reproducibilidad: `rand_r` con semilla por celda

`rand()` no es *thread-safe* y, aunque lo fuera, el orden en que los hilos lo
llaman cambia de corrida en corrida: dos ejecuciones con distinto número de hilos
darían resultados distintos. La solución fue darle a cada celda su propia semilla,
derivada de su posición y del tick ([especies.c:43-45](especies.c#L43-L45)):

```c
static unsigned int semilla_de(int i, int j, int tick) {
    return (unsigned int)((i * 73856093) ^ (j * 19349663) ^ (tick * 83492791));
}
```

Como la secuencia aleatoria de una celda depende solo de dónde está y en qué tick
va, el reparto de hilos no la afecta.

**Verificación:** se corrió la simulación con 1, 2, 4 y 8 hilos y la salida
completa es idéntica byte a byte en los tres casos (Tick 20 = 64 plantas,
4 herbívoros, 0 carnívoros).

### 2.4 Competencia por recursos

Dos animales vecinos pueden querer moverse o reproducirse hacia la misma celda
vacía. La regla es **gana el primero que llega**: el segundo encuentra la celda
ocupada y desiste ([especies.c:113-125](especies.c#L113-L125)). No se pierde
consistencia — la celda queda con un solo ocupante — pero *cuál* de los dos gana
depende del orden de los hilos. Esto no rompe la reproducibilidad observada
porque el resto de las decisiones sí es determinista.

### 2.5 Depredador y presa sin comunicarse

Cuando un carnívoro se come a un herbívoro hay que hacer dos cosas: subirle la
energía al carnívoro y borrar al herbívoro. Pero cada celda la procesa un hilo
distinto y solo escribe en su propia posición de `grid_siguiente`.

La solución es que **ambos lados evalúen la misma regla** sobre `grid_actual`:
el depredador elige su presa con `buscar_presa`, que es determinista (se queda con
el primero en el orden de `DI`/`DJ`); la presa aplica esa misma función a cada
vecino depredador para ver si la eligió a ella (`me_estan_comiendo`). Como los dos
leen el mismo estado y aplican la misma regla, llegan a la misma conclusión sin
escribirse mutuamente. Por eso `buscar_presa` **no** usa azar, mientras que la
elección de celda vacía para moverse sí lo usa.

---

## 3. Mapa de OpenMP

| Directiva | Dónde | Qué hace |
|---|---|---|
| `parallel for collapse(2) schedule(dynamic)` | [ecosistema.c:48](ecosistema.c#L48) | reparte las celdas entre los hilos |
| barrera implícita | [ecosistema.c:55](ecosistema.c#L55) | sincroniza antes del `memcpy` |
| `critical (escritura_vecina)` | [especies.c:116](especies.c#L116), [especies.c:226](especies.c#L226) | protege la escritura a celdas vecinas |
| `atomic` | [especies.c:128](especies.c#L128), [147](especies.c#L147), [201](especies.c#L201) | actualiza los contadores de población |

**`collapse(2)`** fusiona los dos bucles anidados en un solo espacio de iteración.
Sin él solo se repartirían las 10 filas del bucle externo; con él se reparten las
100 celdas, lo que da mucho mejor balance.

**La barrera implícita** al cerrar el `parallel for` es la que garantiza que todos
los hilos terminaron de escribir `grid_siguiente` antes de copiarlo. No hace falta
un `#pragma omp barrier` explícito.

**`critical (escritura_vecina)`** protege el único caso en que un hilo escribe
fuera de su celda: cuando pone una cría o mueve un animal a una celda vecina, que
"pertenece" a otro hilo. Va con nombre para que ambas regiones compartan el mismo
candado — si tuvieran candados distintos no habría exclusión mutua entre ellas.

**`atomic`** se usa para los contadores globales porque un `++` sobre una variable
compartida no es una operación indivisible: sin protección, dos hilos pueden leer
el mismo valor y escribir el mismo resultado, perdiendo un incremento. Es más
barato que un `critical` porque se traduce a una instrucción atómica del
procesador en vez de un candado.

**Verificación:** en los 21 estados guardados, los contadores
`num_plantas / num_herbivoros / num_carnivoros` coinciden exactamente con el
conteo real de símbolos de la cuadrícula (21/21), lo que confirma que no se
pierden actualizaciones.

**Dónde *no* hay OpenMP:** `mostrar_estado` no lleva ninguna directiva. Se llama
desde `simular_tick` **después** del `parallel for`, o sea pasada la barrera, con
un solo hilo activo. No hay carrera posible al escribir el archivo, así que
agregar un `critical` solo costaría tiempo.

---

## 4. Análisis de resultados

Población por tick (de `resultados.txt`, semilla fija `srand(42)`):

| Tick | 0 | 5 | 10 | 12 | 15 | 17 | 20 |
|---|---|---|---|---|---|---|---|
| Plantas | 34 | 30 | 33 | 40 | 47 | 50 | 64 |
| Herbívoros | 17 | 14 | 12 | 6 | 4 | 4 | 4 |
| Carnívoros | 4 | 4 | 4 | 4 | 3 | 0 | 0 |

La simulación pasa por tres fases:

1. **Ticks 0-9, equilibrio.** Las plantas bajan de 34 a 25 y se recuperan; los
   herbívoros se mantienen en 14. Las plantas se reproducen aproximadamente al
   mismo ritmo al que se las comen.

2. **Ticks 10-16, caída de los herbívoros.** De 14 bajan a 4. Los que quedaban
   estaban agrupados cerca de los carnívoros y en zonas ya sin plantas, así que
   mueren por depredación y por hambre. Al haber menos herbívoros, las plantas
   dejan de ser podadas y empiezan a subir (33 → 50).

3. **Ticks 17-20, extinción de los carnívoros.** Los tres carnívoros que quedaban
   se extinguen en el tick 17: con solo 4 herbívoros dispersos en 100 celdas, la
   probabilidad de tener uno entre los 8 vecinos es baja, y sin comer la energía
   se agota. Sin depredadores, los 4 herbívoros sobrevivientes se estabilizan, y
   las plantas crecen sin freno hasta 64.

El estado final tiene sentido ecológico: **el nivel trófico superior es el
primero en caer**. Los carnívoros dependen de una presa que a su vez ya estaba
escasa, y en una cuadrícula tan chica no queda margen para que la población se
recupere. Es el mismo patrón de las cascadas tróficas reales, solo que acelerado
por el tamaño del tablero.

Una observación sobre los parámetros: los carnívoros arrancan con solo 4
individuos sobre 100 celdas (5% de probabilidad en la inicialización). Con esa
densidad, encontrar presas depende demasiado de la suerte inicial. Subir esa
proporción, o bajar `COSTO_POR_TICK`, daría un ciclo depredador-presa más largo
en vez de una extinción temprana.

---

## 5. Análisis de eficiencia

Medido en un Intel Core i9-13980HX (24 núcleos: 8 de rendimiento y 16 de
eficiencia, 32 hilos), GCC 16.1.1 con `-O2`, mejor de 3 corridas.

Como la cuadrícula del enunciado es de 100 celdas, se compiló un binario aparte
(`make bench`) con **1000x1000 celdas y 50 ticks** para que haya trabajo real que
repartir. La lógica es idéntica; solo cambian dos constantes. La salida se apaga
con `ECO_SILENCIOSO=1`, porque si no se estaría cronometrando la consola.

### 5.1 Tiempos (s) y speedup

| Hilos | static | dynamic | guided |
|---|---|---|---|
| 1 | 1.498 | 1.873 | 1.433 |
| 2 | 1.059 | 2.582 | 1.026 |
| 4 | 1.113 | 3.140 | 1.074 |
| 8 | 1.514 | 3.868 | 1.527 |

| Hilos | static | dynamic | guided |
|---|---|---|---|
| 2 | **1.41x** | 0.73x | **1.40x** |
| 4 | 1.35x | 0.60x | 1.33x |
| 8 | 0.99x | 0.48x | 0.94x |

El resultado es claro y no es el que esperábamos: **el speedup máximo es 1.41x
con 2 hilos, y a partir de ahí empeora.** Con 8 hilos ya no hay ganancia, y con
`dynamic` la simulación llega a tardar **el doble** que en secuencial.

### 5.2 Por qué `dynamic` sale último

`schedule(dynamic)` se eligió por una razón razonable: el trabajo por celda es
desigual (una celda vacía es barata, un carnívoro cazando es caro). Pero el
**chunk por defecto de `dynamic` es 1**, es decir que los hilos se reparten las
celdas de a una, y cada entrega cuesta una operación atómica sobre un contador
compartido. Con un millón de celdas por tick, ese reparto cuesta más que el
trabajo que reparte.

La medición lo confirma (8 hilos):

| `OMP_SCHEDULE` | tiempo (s) |
|---|---|
| `dynamic` | 3.828 |
| `dynamic,64` | 1.870 |
| `dynamic,1000` | 1.896 |
| `static` | 1.515 |

Solo con subir el bloque a 64 celdas, `dynamic` pasa de 3.83s a 1.87s: **más de
la mitad del tiempo se iba en repartir trabajo, no en simular**. La conclusión
práctica es que el desbalance entre celdas es real pero pequeño (todas hacen a lo
sumo un recorrido de 8 vecinos), así que no compensa un reparto tan fino;
`static`, que reparte una sola vez y sin sincronización, gana.

### 5.3 Por qué se estanca en 8 hilos

Aun con el mejor schedule, pasar de 2 a 8 hilos no mejora. Hay dos causas:

- **El `critical` es un único candado global.** Toda escritura a una celda vecina
  —cada cría de planta, cada movimiento de animal— pasa por la misma región
  `escritura_vecina`, así que se ejecuta **una a la vez** aunque las celdas
  involucradas estén en extremos opuestos de la cuadrícula. Con más hilos, más
  competencia por ese candado y más tiempo esperando. Es la parte serial que, por
  la ley de Amdahl, le pone techo al speedup.
- **La simulación está limitada por memoria.** Cada celda lee sus 8 vecinos, o
  sea que se recorren dos matrices de 8 MB por tick haciendo muy poca aritmética
  por dato. A partir de unos pocos hilos el cuello de botella es el ancho de banda
  de memoria, no el cálculo.

### 5.4 La cuadrícula del enunciado

Con el tamaño original (10x10, 20 ticks), paralelizar es contraproducente:

| Hilos | 1 | 2 | 4 | 8 |
|---|---|---|---|---|
| Tiempo (s) | 0.000131 | 0.000282 | 0.000309 | 0.000633 |

Con 8 hilos tarda **casi 5 veces más** que con uno. Las 100 celdas se procesan en
microsegundos y crear y sincronizar los hilos cuesta más que la simulación
entera. Es el caso de libro en que el problema es demasiado chico para
paralelizarlo.

### 5.5 Qué haría falta para que escale

Sin cambiar la lógica del modelo:

1. **Aumentar el chunk** o pasar a `schedule(static)`. Es un cambio de una palabra
   y recupera más de la mitad del tiempo con 8 hilos.
2. **Reemplazar el `critical` global por candados por región** (por ejemplo uno
   por franja de filas), para que dos escrituras lejanas no se bloqueen entre sí.
3. **Eliminar el `critical` de las celdas vacías.** Hoy toda celda `VACIO` entra a
   la región crítica solo para limpiar su energía ([especies.c:226](especies.c#L226)),
   aunque escribe en *su propia* celda. Como al final de la simulación más de la
   mitad del tablero está vacío, es una fracción grande del tráfico del candado.
4. **Abrir la región paralela una sola vez** fuera del bucle de ticks, con un
   `barrier` explícito por tick, en vez de crear y destruir el equipo de hilos 50
   veces.

Se dejó `schedule(dynamic)` como configuración por defecto, que es la decisión de
diseño original del equipo; el binario de medición permite cambiarla con
`OMP_SCHEDULE` sin recompilar.

---

## 6. Conclusiones

- El **doble buffer** es lo que hace correcta la paralelización: al separar
  lectura de escritura, el orden de los hilos deja de importar.
- La **reproducibilidad** se logró dándole a cada celda su propia semilla en vez
  de compartir el generador. Verificado: 1, 2, 4 y 8 hilos dan salidas idénticas.
- El uso de `atomic` para contadores es correcto: coinciden con el conteo real de
  la cuadrícula en los 21 estados.
- **Paralelizar no garantiza ir más rápido.** Con la cuadrícula del enunciado,
  usar 8 hilos es 5 veces más lento; con una grande, el mejor caso es 1.41x. El
  costo de sincronización y el candado global pesan más que el trabajo repartido.
- La lección más útil del proyecto fue que **medir cambia las conclusiones**: la
  elección de `schedule(dynamic)`, que parecía la más justificada en teoría,
  resultó ser la peor en la práctica por el tamaño de bloque por defecto.

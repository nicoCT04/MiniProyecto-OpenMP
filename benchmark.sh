#!/usr/bin/env bash
# Mide la eficiencia de la simulacion variando numero de hilos y schedule.
#
# Usa dos binarios, los dos los construye "make bench", que ya llama a este
# script:
#   ecosistema_bench -> cuadricula grande + schedule(runtime), para medir
#   ecosistema       -> la cuadricula del enunciado (10x10), como contraste
#
#   ECO_SILENCIOSO=1  -> apaga consola y archivo, para cronometrar solo el
#                        computo y no la escritura de la grilla.
#   OMP_SCHEDULE      -> la lee schedule(runtime), asi no hay que recompilar
#                        para pasar de static a dynamic o guided.
#
# Salida: benchmark.md con las tablas de tiempos y speedup.

set -euo pipefail

BIN=./ecosistema_bench
BIN_CHICO=./ecosistema
SALIDA=benchmark.md
HILOS=(1 2 4 8)
SCHEDULES=(static dynamic guided)
REPETICIONES=3

for binario in "$BIN" "$BIN_CHICO"; do
    if [ ! -x "$binario" ]; then
        echo "Falta $binario. Corre: make bench" >&2
        exit 1
    fi
done

# mejor_tiempo <binario> <hilos> <schedule>
# Corre REPETICIONES veces y se queda con el menor tiempo: el minimo es el
# menos contaminado por otros procesos de la maquina.
mejor_tiempo() {
    local binario=$1 hilos=$2 sched=$3 mejor="" t
    for _ in $(seq "$REPETICIONES"); do
        t=$(OMP_NUM_THREADS="$hilos" OMP_SCHEDULE="$sched" ECO_SILENCIOSO=1 "$binario" \
            | awk '/Tiempo total:/ {print $3}')
        if [ -z "$mejor" ] || awk "BEGIN{exit !($t < $mejor)}"; then
            mejor=$t
        fi
    done
    echo "$mejor"
}

declare -A TIEMPO
for sched in "${SCHEDULES[@]}"; do
    for h in "${HILOS[@]}"; do
        printf 'Midiendo schedule=%-8s hilos=%s ... ' "$sched" "$h" >&2
        TIEMPO[$sched,$h]=$(mejor_tiempo "$BIN" "$h" "$sched")
        printf '%s s\n' "${TIEMPO[$sched,$h]}" >&2
    done
done

echo 'Midiendo experimentos extra ...' >&2

{
    echo "# Medicion de eficiencia"
    echo
    echo "Mejor de $REPETICIONES corridas, salida desactivada con \`ECO_SILENCIOSO=1\`."
    echo "Generado por \`benchmark.sh\`."
    echo
    echo "## Tiempo total (s), cuadricula grande"
    echo
    echo "| Hilos | static | dynamic | guided |"
    echo "|---|---|---|---|"
    for h in "${HILOS[@]}"; do
        printf '| %s |' "$h"
        for sched in "${SCHEDULES[@]}"; do
            printf ' %s |' "${TIEMPO[$sched,$h]}"
        done
        echo
    done
    echo
    echo "## Speedup vs 1 hilo"
    echo
    echo "| Hilos | static | dynamic | guided |"
    echo "|---|---|---|---|"
    for h in "${HILOS[@]}"; do
        printf '| %s |' "$h"
        for sched in "${SCHEDULES[@]}"; do
            awk "BEGIN{printf \" %.2fx |\", ${TIEMPO[$sched,1]} / ${TIEMPO[$sched,$h]}}"
        done
        echo
    done

    # Explica por que dynamic sale ultimo: su chunk por defecto es 1, o sea
    # un reparto atomico por celda. Subiendo el bloque el costo desaparece.
    echo
    echo "## Efecto del tamano de bloque (8 hilos)"
    echo
    echo "| OMP_SCHEDULE | tiempo (s) |"
    echo "|---|---|"
    for sched in dynamic dynamic,64 dynamic,1000 static static,1000; do
        printf '| `%s` | %s |\n' "$sched" "$(mejor_tiempo "$BIN" 8 "$sched")"
    done

    # El otro extremo: la cuadricula del enunciado tiene tan poco trabajo
    # que crear los hilos cuesta mas que la simulacion entera.
    echo
    echo "## Cuadricula del enunciado (10x10, 20 ticks)"
    echo
    echo "| Hilos | tiempo (s) |"
    echo "|---|---|"
    for h in "${HILOS[@]}"; do
        printf '| %s | %s |\n' "$h" "$(mejor_tiempo "$BIN_CHICO" "$h" dynamic)"
    done
} > "$SALIDA"

echo "Tabla escrita en $SALIDA" >&2

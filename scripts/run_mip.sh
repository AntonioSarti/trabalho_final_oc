#!/usr/bin/env bash
# Resolve todas as instâncias com o modelo PLIM no HiGHS.
# Uso: scripts/run_mip.sh [limite_de_tempo_s] [execuções_em_paralelo]
set -euo pipefail
cd "$(dirname "$0")/.."
LIMIT=${1:-1800}
JOBS=${2:-3}
PY=${PYTHON:-.venv/bin/python}
OUT=results/mip
mkdir -p "$OUT"
echo "instance,n,m,cols,rows,nnz,status,value,valid,bound,gap,nodes,time_build,time_total,time_limit" > "$OUT/mip.csv"
# Ordem intercalada para distribuir as instâncias grandes entre os lotes.
for i in 04 08 12 03 07 11 02 06 10 01 05 09; do echo "$i"; done |
  xargs -P "$JOBS" -I{} sh -c "$PY solver/jifc_mip.py $OUT/jifc{}.sol --time-limit $LIMIT \
      --threads 1 --log $OUT/jifc{}.log --name jifc{} --csv $OUT/mip.csv \
      < instances/jifc{}.dat > /dev/null"

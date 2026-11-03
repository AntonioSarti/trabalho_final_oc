#!/usr/bin/env bash
# Experimentos finais do SA em todas as instâncias.
#   calibrada: configuração padrão do ./jifc, sementes 1..10
#   proposta:  parâmetros da proposta original (p0=0.8, pmin=1e-3, 40/30/30), sementes 1..5
# Uso: scripts/run_experiments.sh [execuções_em_paralelo]
set -euo pipefail
cd "$(dirname "$0")/.."
JOBS=${1:-5}
make -s
ALL="01 02 03 04 05 06 07 08 09 10 11 12"
python3 scripts/run_sa.py --instances $ALL --seeds 1 2 3 4 5 6 7 8 9 10 --jobs "$JOBS" \
    --config "calibrada:" --out results/sa_calibrada.csv \
    --sol-dir results/sa_calibrada/sol --trace-dir results/sa_calibrada/trace
python3 scripts/run_sa.py --instances $ALL --seeds 1 2 3 4 5 --jobs "$JOBS" \
    --config "proposta:--p0 0.8 --pmin 1e-3 --padd 0.4 --premove 0.3" \
    --out results/sa_proposta.csv --sol-dir results/sa_proposta/sol

#!/usr/bin/env bash
# Calibração dos parâmetros do SA (um fator por vez) nas instâncias jifc01,
# jifc05 e jifc09 (n = 500, uma de cada família), com sementes distintas das
# usadas nos experimentos finais. Os resultados estão em results/tuning/.
# Uso: scripts/run_tuning.sh [execuções_em_paralelo]
set -euo pipefail
cd "$(dirname "$0")/.."
JOBS=${1:-5}
make -s
OUT=results/tuning
INST="01 05 09"

# Etapa 1: temperaturas inicial (p0) e final (pmin), operadores da proposta.
C=()
for p0 in 0.8 0.5 0.3; do
  for pm in 1e-3 1e-10 1e-20 1e-40; do
    C+=(--config "p0=${p0},pmin=${pm}:--p0 $p0 --pmin $pm --kmax 1000 --padd 0.4 --premove 0.3")
  done
done
python3 scripts/run_sa.py --instances $INST --seeds 101 102 103 "${C[@]}" --jobs "$JOBS" \
    --out $OUT/stage1.csv

# Etapa 2: resfriamento (alpha) e tamanho do patamar (L), com p0 = 0.5 e pmin = 1e-20.
C=()
for a in 0.90 0.95 0.98; do
  for L in 25 50 100; do
    C+=(--config "alpha=${a},L=${L}:--p0 0.5 --pmin 1e-20 --kmax 1000 --alpha $a --L $L --padd 0.4 --premove 0.3")
  done
done
python3 scripts/run_sa.py --instances $INST --seeds 101 102 103 "${C[@]}" --jobs "$JOBS" \
    --out $OUT/stage2.csv

# Etapa 3: probabilidades dos operadores Add/Remove/Swap.
B="--p0 0.5 --pmin 1e-20 --kmax 1000"
C=()
for mix in 40/30/30 33/33/34 50/25/25 25/25/50 20/20/60 15/15/70 10/10/80 25/15/60 15/25/60; do
  IFS=/ read -r pa pr _ <<< "$mix"
  C+=(--config "$mix:$B --padd 0.$pa --premove 0.$pr")
done
python3 scripts/run_sa.py --instances $INST --seeds 101 102 103 "${C[@]}" --jobs "$JOBS" \
    --out $OUT/stage3.csv

# Etapa 3b: confirmação das melhores misturas com 8 sementes.
C=()
for mix in 40/30/30 25/25/50 20/20/60 15/25/60 10/10/80 10/20/70; do
  IFS=/ read -r pa pr _ <<< "$mix"
  C+=(--config "$mix:$B --padd 0.$pa --premove 0.$pr")
done
python3 scripts/run_sa.py --instances $INST --seeds 101 102 103 104 105 106 107 108 "${C[@]}" \
    --jobs "$JOBS" --out $OUT/stage3b.csv

for s in stage1 stage2 stage3 stage3b; do
  echo "== $s"
  python3 scripts/summarize.py tuning $OUT/$s.csv
done

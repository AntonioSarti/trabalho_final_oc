# Trabalho Final de OC
Repositório destinado ao desenvolvimento do Trabalho Final da disciplina de Otimização Combinatória (INF05010) - UFRGS - 2026/2

Problema: **O Jardim Imperial das Flores Caprichosas (JIFC)**. Meta-heurística: **Simulated Annealing**.

# Integrantes
- Antonio Carlos Gonçalves Sarti
- Giovanni Rosell Paredes dos Santos

## Estrutura

| Caminho | Conteúdo |
|---|---|
| `src/` | Simulated Annealing em C++17 (sem dependências externas) |
| `solver/jifc_mip.py` | Modelo PLIM resolvido com o HiGHS |
| `scripts/` | Execução dos experimentos, verificação de soluções e tabelas |
| `instances/` | Instâncias `jifc01`–`jifc12` ([origem](https://github.com/mrpritt/co-datasets/tree/main/jifc)) e descrição do formato |
| `results/` | Dados brutos: CSVs, soluções, protocolos do solver e da calibração |
| `Report/` | Relatório |

## Simulated Annealing

Compilação (g++ ≥ 7 ou clang++ com C++17):

```sh
make
```

Seguindo as convenções da disciplina, a instância é lida da entrada padrão, a
melhor solução é impressa na saída padrão e o primeiro argumento é o arquivo
onde ela também é gravada:

```sh
./jifc solucao.txt --seed 1 < instances/jifc01.dat
```

A solução tem, na primeira linha, o esplendor e o número `k` de setores
cultivados e, na segunda, os `k` setores (1-indexados). Um resumo da execução
vai para a saída de erro. Parâmetros principais (`./jifc` sem argumentos lista
todos):

| Opção | Padrão | Significado |
|---|---|---|
| `--seed` | 1 | semente do gerador (mt19937_64) |
| `--alpha` | 0.95 | resfriamento geométrico |
| `--L` | 50 | iterações por nível = L·\|S\| |
| `--p0` | 0.5 | probabilidade de aceitar uma piora de magnitude *d* em T₀ |
| `--pmin` | 1e-20 | idem em T_min |
| `--kmax` | 100 | número máximo de níveis de temperatura |
| `--padd`, `--premove` | 0.1, 0.2 | probabilidades de Add e Remove (Swap = restante) |
| `--ncal` | 100 | tentativas da calibração de *d* |
| `--time-limit` | 0 | limite de tempo opcional (s) |
| `--csv`, `--trace` | | estatísticas da execução e evolução por nível |

Os parâmetros da proposta original correspondem a
`--p0 0.8 --pmin 1e-3 --padd 0.4 --premove 0.3`.

## Modelo matemático (HiGHS)

```sh
python3 -m venv .venv && .venv/bin/pip install highspy==1.15.1 numpy
.venv/bin/python solver/jifc_mip.py solucao.txt --time-limit 1800 < instances/jifc01.dat
```

## Reprodução dos experimentos

```sh
scripts/run_mip.sh 1800 3          # solver: 30 min por instância, 3 em paralelo
scripts/run_experiments.sh 5       # SA: calibrada (10 sementes) e proposta (5)
python3 scripts/summarize.py final results/sa_calibrada.csv results/mip/mip.csv
python3 scripts/verify.py instances/jifc01.dat results/sa_calibrada/sol/jifc01_calibrada_s1.sol
```

A calibração de parâmetros (`results/tuning/`) usou as sementes 101–108, e os
experimentos finais as sementes 1–10.

## Melhores valores conhecidos
| Instância | BKV    |
|-----------|-------:|
| jifc01    | 88063  |
| jifc02    | 136257 |
| jifc03    | 186418 |
| jifc04    | 209722 |
| jifc05    | 62273  |
| jifc06    | 103748 |
| jifc07    | 118878 |
| jifc08    | 141621 |
| jifc09    | 45345  |
| jifc10    | 59368  |
| jifc11    | 76498  |
| jifc12    | 88180  |

(BKV: melhor valor conhecido (ingl. best known value).)

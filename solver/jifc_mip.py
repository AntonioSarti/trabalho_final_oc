#!/usr/bin/env python3
"""Modelo PLIM do JIFC resolvido com o HiGHS.

Uso: jifc_mip.py <arquivo_solucao> [--time-limit S] [--threads K]
                 [--log ARQ] [--name NOME] [--csv ARQ] < instancia.dat

Segue as convenções da disciplina: lê a instância da entrada padrão, imprime a
melhor solução na saída padrão (primeira linha "esplendor k", segunda linha os
k setores) e a grava no arquivo dado como primeiro argumento.

O modelo é o da Seção 1.4 do relatório (fluxo para a conectividade, com
big-M = n - 1), exceto pela família (PLI-3). Escrita explicitamente, ela tem
1 + n + C(n, 2) restrições densas, o que para n = 2000 dá cerca de 4·10^9
coeficientes. Ela é substituída pela linearização compacta equivalente

    sum_i a_i x_i + 2 t + sum_i s_i <= H,
    s_i >= f_i x_i - t,   s_i >= 0,   t >= 0,

pois, para x fixo, min_{t >= 0} 2t + sum_i max(0, f_i x_i - t) é exatamente a
soma dos dois maiores valores f_i x_i (mínimo em t = segundo maior valor).
"""
import argparse
import sys
import time

import highspy
import numpy as np


def read_instance(stream):
    tok = stream.read().split()
    n, m, p, H = map(int, tok[:4])
    vals = np.array(tok[4:4 + 3 * n], dtype=np.int64).reshape(n, 3)
    edges = np.array(tok[4 + 3 * n:4 + 3 * n + 2 * m], dtype=np.int64).reshape(m, 2) - 1
    return n, m, p - 1, H, vals[:, 0], vals[:, 1], vals[:, 2], edges


def build_model(h, n, p, H, e, a, f, edges):
    inf = highspy.kHighsInf
    m = len(edges)
    # Arcos dirigidos A = {(i,j), (j,i) : {i,j} in E}.
    tail = np.concatenate([edges[:, 0], edges[:, 1]])
    head = np.concatenate([edges[:, 1], edges[:, 0]])
    na = 2 * m
    X, Y, T, S = 0, n, n + na, n + na + 1  # deslocamentos das colunas
    ncols = n + na + 1 + n

    lower = np.zeros(ncols)
    upper = np.full(ncols, inf)
    upper[X:X + n] = 1.0
    lower[X + p] = 1.0                       # (PLI-2) x_p = 1
    h.addVars(ncols, lower, upper)
    h.changeColsIntegrality(n, np.arange(X, X + n, dtype=np.int32),
                            np.full(n, highspy.HighsVarType.kInteger))
    h.changeColsCost(n, np.arange(X, X + n, dtype=np.int32), e.astype(float))
    h.changeObjectiveSense(highspy.ObjSense.kMaximize)

    rows_lo, rows_hi, starts, idx, val = [], [], [], [], []

    def add_row(lo, hi, cols, coefs):
        rows_lo.append(lo)
        rows_hi.append(hi)
        starts.append(len(idx))
        idx.extend(cols)
        val.extend(coefs)

    # Capacidade hídrica (PLI-3 compacta).
    add_row(-inf, float(H),
            list(range(X, X + n)) + [T] + list(range(S, S + n)),
            list(a.astype(float)) + [2.0] + [1.0] * n)
    # s_i - f_i x_i + t >= 0.
    for i in range(n):
        add_row(0.0, inf, [S + i, X + i, T], [1.0, -float(f[i]), 1.0])

    # Conservação de fluxo: (PLI-4) para i != p e (PLI-5) para p.
    out_arcs = [[] for _ in range(n)]
    in_arcs = [[] for _ in range(n)]
    for k in range(na):
        out_arcs[tail[k]].append(Y + k)
        in_arcs[head[k]].append(Y + k)
    for i in range(n):
        cols = in_arcs[i] + out_arcs[i]
        coefs = [1.0] * len(in_arcs[i]) + [-1.0] * len(out_arcs[i])
        if i == p:
            # sum_out y - sum_in y - sum_{j != p} x_j = 0
            coefs = [-c for c in coefs]
            cols = cols + [X + j for j in range(n) if j != p]
            coefs = coefs + [-1.0] * (n - 1)
        else:
            cols = cols + [X + i]
            coefs = coefs + [-1.0]
        add_row(0.0, 0.0, cols, coefs)

    # (PLI-6) e (PLI-7): y_ij <= (n-1) x_i e y_ij <= (n-1) x_j.
    M = float(n - 1)
    for k in range(na):
        add_row(-inf, 0.0, [Y + k, X + tail[k]], [1.0, -M])
        add_row(-inf, 0.0, [Y + k, X + head[k]], [1.0, -M])

    h.addRows(len(rows_lo), np.array(rows_lo), np.array(rows_hi), len(idx),
              np.array(starts, dtype=np.int32), np.array(idx, dtype=np.int32),
              np.array(val))
    return ncols, len(rows_lo), len(idx)


def verify(n, p, H, e, a, f, edges, chosen):
    """Verificação independente: Palácio, conectividade e capacidade."""
    inside = np.zeros(n, dtype=bool)
    inside[chosen] = True
    if not inside[p]:
        return False, 0
    adj = [[] for _ in range(n)]
    for i, j in edges:
        if inside[i] and inside[j]:
            adj[i].append(j)
            adj[j].append(i)
    seen, stack = {p}, [p]
    while stack:
        x = stack.pop()
        for w in adj[x]:
            if w not in seen:
                seen.add(w)
                stack.append(w)
    water = int(a[chosen].sum()) + int(np.sort(f[chosen])[::-1][:2].sum())
    return len(seen) == len(chosen) and water <= H, int(e[chosen].sum())


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("solution")
    ap.add_argument("--time-limit", type=float, default=3600.0)
    ap.add_argument("--threads", type=int, default=1)
    ap.add_argument("--log", default="")
    ap.add_argument("--name", default="stdin")
    ap.add_argument("--csv", default="")
    args = ap.parse_args()

    n, m, p, H, e, a, f, edges = read_instance(sys.stdin)
    start = time.time()
    h = highspy.Highs()
    h.setOptionValue("output_flag", bool(args.log))
    if args.log:
        h.setOptionValue("log_file", args.log)
        h.setOptionValue("log_to_console", False)
    h.setOptionValue("time_limit", args.time_limit)
    h.setOptionValue("threads", args.threads)
    h.setOptionValue("mip_rel_gap", 0.0)
    h.setOptionValue("random_seed", 0)
    ncols, nrows, nnz = build_model(h, n, p, H, e, a, f, edges)
    build_time = time.time() - start

    h.run()
    total_time = time.time() - start
    info = h.getInfo()
    status = h.modelStatusToString(h.getModelStatus())
    bound = info.mip_dual_bound
    has_sol = info.primal_solution_status == 2  # kSolutionStatusFeasible

    value, chosen, valid = 0, np.array([], dtype=np.int64), False
    if has_sol:
        x = np.array(h.getSolution().col_value[:n])
        chosen = np.flatnonzero(x > 0.5)
        valid, value = verify(n, p, H, e, a, f, edges, chosen)
    gap = (bound - value) / bound * 100 if has_sol and bound > 0 else float("nan")

    text = f"{value} {len(chosen)}\n" + " ".join(str(i + 1) for i in chosen) + "\n"
    sys.stdout.write(text)
    with open(args.solution, "w") as out:
        out.write(text)

    if args.csv:
        import os
        new = not os.path.exists(args.csv) or os.path.getsize(args.csv) == 0
        with open(args.csv, "a") as out:
            if new:
                out.write("instance,n,m,cols,rows,nnz,status,value,valid,bound,gap,"
                          "nodes,time_build,time_total,time_limit\n")
            out.write(f"{args.name},{n},{m},{ncols},{nrows},{nnz},{status},{value},"
                      f"{int(valid)},{bound:.4f},{gap:.4f},{info.mip_node_count},"
                      f"{build_time:.2f},{total_time:.2f},{args.time_limit:g}\n")

    print(f"{args.name}: status={status} valor={value} limite={bound:.2f} "
          f"gap={gap:.3f}% tempo={total_time:.1f}s ({'válida' if valid else 'INVÁLIDA'})",
          file=sys.stderr)
    return 0 if (valid or not has_sol) else 3


if __name__ == "__main__":
    sys.exit(main())

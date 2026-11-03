#!/usr/bin/env python3
"""Verificador independente de soluções do JIFC.

Uso: verify.py <instancia.dat> <solucao.txt>

O arquivo de solução tem na primeira linha "esplendor k" e na segunda os k
setores (1-indexados). Verifica: Palácio presente, G[C] conexo e
sum(a_i, i em C) + top2(f_i, i em C) <= H. Imprime o esplendor recalculado.
"""
import sys
from collections import deque


def read_instance(path):
    tok = open(path).read().split()
    n, m, p, H = map(int, tok[:4])
    vals = list(map(int, tok[4:4 + 3 * n]))
    e, a, f = vals[0::3], vals[1::3], vals[2::3]
    adj = [[] for _ in range(n + 1)]
    rest = list(map(int, tok[4 + 3 * n:4 + 3 * n + 2 * m]))
    for i, j in zip(rest[0::2], rest[1::2]):
        adj[i].append(j)
        adj[j].append(i)
    return n, p, H, [0] + e, [0] + a, [0] + f, adj


def main():
    n, p, H, e, a, f, adj = read_instance(sys.argv[1])
    tok = open(sys.argv[2]).read().split()
    claimed, k = int(tok[0]), int(tok[1])
    C = list(map(int, tok[2:2 + k]))
    S = set(C)
    ok = True
    if len(S) != len(C) or any(not 1 <= x <= n for x in C):
        print("setores repetidos ou fora do intervalo"); ok = False
    if p not in S:
        print("Palácio ausente"); ok = False
    seen, q = {p}, deque([p])
    while q:
        x = q.popleft()
        for w in adj[x]:
            if w in S and w not in seen:
                seen.add(w); q.append(w)
    if seen != S:
        print("G[C] desconexo"); ok = False
    W = sum(a[x] for x in C) + sum(sorted((f[x] for x in C), reverse=True)[:2])
    if W > H:
        print(f"capacidade violada: W={W} > H={H}"); ok = False
    value = sum(e[x] for x in C)
    if value != claimed:
        print(f"valor informado {claimed} difere do recalculado {value}"); ok = False
    print(f"{'VALIDA' if ok else 'INVALIDA'} esplendor={value} |C|={len(C)} W={W} H={H}")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()

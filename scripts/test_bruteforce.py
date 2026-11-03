#!/usr/bin/env python3
"""Teste de correção em instâncias pequenas aleatórias (n = 12..16).

Compara o ótimo por força bruta (todos os subconjuntos) com o modelo PLIM
(HiGHS), que deve coincidir, e com o SA, que não pode superá-lo.
Uso: scripts/test_bruteforce.py [python_com_highspy]   (padrão: .venv/bin/python)
"""
import os
import random
import subprocess
import sys
import tempfile

root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
py = sys.argv[1] if len(sys.argv) > 1 else os.path.join(root, ".venv", "bin", "python")
S = tempfile.mkdtemp()
def gen(seed, n):
    r = random.Random(seed)
    edges = set()
    for v in range(2, n+1): edges.add((r.randint(1, v-1), v))          # árvore => conexo
    while len(edges) < n + n//2:
        i, j = sorted(r.sample(range(1, n+1), 2)); edges.add((i, j))
    e = [r.randint(0, 50) for _ in range(n)]; a = [r.randint(0, 30) for _ in range(n)]; f = [r.randint(0, 40) for _ in range(n)]
    p = r.randint(1, n); H = a[p-1] + f[p-1] + r.randint(20, 150)
    return n, sorted(edges), p, H, e, a, f
def brute(n, edges, p, H, e, a, f):
    adj = {i: set() for i in range(1, n+1)}
    for i, j in edges: adj[i].add(j); adj[j].add(i)
    best = -1
    for mask in range(1 << n):
        C = [i+1 for i in range(n) if mask >> i & 1]
        if p not in C: continue
        W = sum(a[i-1] for i in C) + sum(sorted((f[i-1] for i in C), reverse=True)[:2])
        if W > H: continue
        Cs = set(C); seen = {p}; st = [p]
        while st:
            x = st.pop()
            for w in adj[x]:
                if w in Cs and w not in seen: seen.add(w); st.append(w)
        if len(seen) == len(C): best = max(best, sum(e[i-1] for i in C))
    return best
bad = 0
for seed in range(40):
    n = 12 + seed % 5
    n, edges, p, H, e, a, f = inst = gen(seed, n)
    txt = f"{n} {len(edges)} {p} {H}\n" + "".join(f"{e[i]} {a[i]} {f[i]}\n" for i in range(n)) + "".join(f"{i} {j}\n" for i, j in edges)
    opt = brute(*inst)
    mip = subprocess.run([py, f"{root}/solver/jifc_mip.py", f"{S}/m.sol", "--time-limit", "60"], input=txt, capture_output=True, text=True)
    mv = int(mip.stdout.split()[0])
    sa = subprocess.run([f"{root}/jifc", f"{S}/s.sol", "--seed", str(seed), "--quiet"], input=txt, capture_output=True, text=True)
    sv = int(sa.stdout.split()[0])
    ok = mv == opt and sv <= opt and sa.returncode == 0
    bad += not ok
    print(f"seed={seed:2d} n={n} ótimo={opt:4d} HiGHS={mv:4d} SA={sv:4d} {'ok' if ok else 'ERRO'}")
print("falhas:", bad)
sys.exit(1 if bad else 0)

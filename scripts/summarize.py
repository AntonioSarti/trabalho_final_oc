#!/usr/bin/env python3
"""Resumos dos experimentos.

  summarize.py tuning <csv> [--latex]
      Desvio médio para o BKV e tempo médio por configuração.
  summarize.py final <sa.csv> <mip.csv> [--latex]
      Tabela por instância exigida no trabalho (SI, SF, BKV, melhoria, desvio,
      tempos, valor/limite/gap do solver).

Problema de maximização: melhoria(SF, SI) = 100 (SF - SI) / SI e
desvio(SF, BKV) = 100 (BKV - SF) / BKV (negativo quando SF supera o BKV).
"""
import argparse
import csv
import statistics as st
from collections import defaultdict

BKV = {
    "jifc01": 88063, "jifc02": 136257, "jifc03": 186418, "jifc04": 209722,
    "jifc05": 62273, "jifc06": 103748, "jifc07": 118878, "jifc08": 141621,
    "jifc09": 45345, "jifc10": 59368, "jifc11": 76498, "jifc12": 88180,
}


def dev(value, inst):
    return 100.0 * (BKV[inst] - value) / BKV[inst]


def improvement(sf, si):
    return 100.0 * (sf - si) / si


def tuning(path, latex):
    rows = list(csv.DictReader(open(path)))
    order, groups = [], defaultdict(list)
    for r in rows:
        if r["config"] not in groups:
            order.append(r["config"])
        groups[r["config"]].append(r)
    insts = sorted({r["instance"] for r in rows})
    head = ["config"] + [f"dev {i}" for i in insts] + ["dev médio", "tempo médio (s)"]
    lines = []
    for c in order:
        g = groups[c]
        per = [st.mean(dev(int(r["best"]), i) for r in g if r["instance"] == i) for i in insts]
        lines.append([c] + [f"{x:.2f}" for x in per]
                     + [f"{st.mean(dev(int(r['best']), r['instance']) for r in g):.3f}",
                        f"{st.mean(float(r['time_total']) for r in g):.1f}"])
    emit(head, lines, latex)


def final(sa_path, mip_path, latex):
    sa = defaultdict(list)
    for r in csv.DictReader(open(sa_path)):
        sa[r["instance"]].append(r)
    mip = {r["instance"]: r for r in csv.DictReader(open(mip_path))} if mip_path else {}
    head = ["instância", "n", "SI", "SF médio", "desv. pad.", "SF melhor", "BKV",
            "melhoria (%)", "desvio médio (%)", "desvio melhor (%)", "tempo SA (s)",
            "solver valor", "solver limite", "gap (%)", "tempo solver (s)"]
    lines = []
    for inst in sorted(sa):
        g = sa[inst]
        si = st.mean(int(r["initial"]) for r in g)
        sf = [int(r["best"]) for r in g]
        t = st.mean(float(r["time_total"]) for r in g)
        m = mip.get(inst)
        lines.append([
            inst, g[0]["n"], f"{si:.0f}", f"{st.mean(sf):.1f}",
            f"{st.stdev(sf):.1f}" if len(sf) > 1 else "-", str(max(sf)), str(BKV[inst]),
            f"{improvement(st.mean(sf), si):.2f}", f"{dev(st.mean(sf), inst):.2f}",
            f"{dev(max(sf), inst):.2f}", f"{t:.1f}",
            m["value"] if m else "-", f"{float(m['bound']):.0f}" if m else "-",
            f"{float(m['gap']):.2f}" if m else "-",
            f"{float(m['time_total']):.0f}" if m else "-",
        ])
    emit(head, lines, latex)


def emit(head, lines, latex):
    if latex:
        print(" & ".join(head) + r" \\ \midrule")
        for l in lines:
            print(" & ".join(l) + r" \\")
        return
    width = [max(len(h), *(len(l[k]) for l in lines)) for k, h in enumerate(head)]
    print("  ".join(h.rjust(w) for h, w in zip(head, width)))
    for l in lines:
        print("  ".join(x.rjust(w) for x, w in zip(l, width)))


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    a = sub.add_parser("tuning")
    a.add_argument("csv")
    a.add_argument("--latex", action="store_true")
    b = sub.add_parser("final")
    b.add_argument("sa")
    b.add_argument("mip", nargs="?", default="")
    b.add_argument("--latex", action="store_true")
    args = ap.parse_args()
    if args.cmd == "tuning":
        tuning(args.csv, args.latex)
    else:
        final(args.sa, args.mip, args.latex)


if __name__ == "__main__":
    main()

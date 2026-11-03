#!/usr/bin/env python3
"""Gera as tabelas LaTeX do relatório a partir dos CSVs em results/.

Uso: scripts/make_tables.py [diretório_de_saída]   (padrão: Report/final/tabelas)
"""
import csv
import math
import os
import statistics as st
import sys
from collections import defaultdict

from summarize import BKV, dev, improvement

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")


def load(name):
    path = os.path.join(RES, name)
    return list(csv.DictReader(open(path))) if os.path.exists(path) else []


def num(x, d=2):
    """Número com vírgula decimal e sinal de menos tipográfico."""
    s = f"{x:.{d}f}".replace(".", "{,}")
    return s.replace("-", "$-$") if x < 0 else s


def thousands(x):
    return f"{int(round(x)):,}".replace(",", "\\,")


def by_config(rows):
    g = defaultdict(list)
    for r in rows:
        g[r["config"]].append(r)
    return g


def mean_dev(rows):
    return st.mean(dev(int(r["best"]), r["instance"]) for r in rows)


def write(out_dir, name, text):
    with open(os.path.join(out_dir, name), "w") as f:
        f.write(text)
    print(f"gerado {name}", file=sys.stderr)


def tab_temperatures(out_dir):
    g = by_config(load("tuning/stage1.csv"))
    if not g:
        return
    p0s, pmins = ["0.8", "0.5", "0.3"], ["1e-3", "1e-10", "1e-20", "1e-40"]
    lines = [r"\begin{tabular}{l" + "r" * len(pmins) + "}", r"\toprule",
             r"& \multicolumn{4}{c}{$p_{\min}$} \\ \cmidrule(l){2-5}",
             r"$p_0$ & $10^{-3}$ & $10^{-10}$ & $10^{-20}$ & $10^{-40}$ \\ \midrule"]
    for p0 in p0s:
        cells = []
        for pm in pmins:
            rows = g[f"p0={p0},pmin={pm}"]
            levels = st.mean(int(r["levels"]) for r in rows)
            cells.append(f"{num(mean_dev(rows))} ({levels:.0f})")
        lines.append(num(float(p0), 1) + " & " + " & ".join(cells) + r" \\")
    lines += [r"\bottomrule", r"\end{tabular}", ""]
    write(out_dir, "calibracao_temperatura.tex", "\n".join(lines))


def tab_cooling(out_dir):
    g = by_config(load("tuning/stage2.csv"))
    if not g:
        return
    Ls = ["25", "50", "100"]
    lines = [r"\begin{tabular}{lrrrr}", r"\toprule",
             r"& & \multicolumn{3}{c}{$L/|S|$} \\ \cmidrule(l){3-5}",
             r"$\alpha$ & níveis & 25 & 50 & 100 \\ \midrule"]
    for a in ["0.90", "0.95", "0.98"]:
        rows0 = g[f"alpha={a},L=25"]
        levels = st.mean(int(r["levels"]) for r in rows0)
        cells = [num(mean_dev(g[f"alpha={a},L={L}"])) for L in Ls]
        lines.append(num(float(a)) + f" & {levels:.0f} & " + " & ".join(cells) + r" \\")
    lines += [r"\bottomrule", r"\end{tabular}", ""]
    write(out_dir, "calibracao_resfriamento.tex", "\n".join(lines))


def tab_operators(out_dir):
    g = by_config(load("tuning/stage3b.csv"))
    if not g:
        return
    insts = ["jifc01", "jifc05", "jifc09"]
    lines = [r"\begin{tabular}{lrrrrr}", r"\toprule",
             r"Add/Remove/Swap (\%) & jifc01 & jifc05 & jifc09 & média & sem vizinho (\%) \\ \midrule"]
    for cfg, rows in g.items():
        per = [mean_dev([r for r in rows if r["instance"] == i]) for i in insts]
        idle = 100 * st.mean(int(r["no_move"]) / int(r["iterations"]) for r in rows)
        lines.append(cfg + " & " + " & ".join(num(x) for x in per)
                     + f" & {num(mean_dev(rows))} & {num(idle, 1)}" + r" \\")
    lines += [r"\bottomrule", r"\end{tabular}", ""]
    write(out_dir, "calibracao_operadores.tex", "\n".join(lines))


def tab_results(out_dir):
    sa = defaultdict(list)
    for r in load("sa_calibrada.csv"):
        sa[r["instance"]].append(r)
    if not sa:
        return
    mip = {r["instance"]: r for r in load("mip/mip.csv")}
    lines = [r"\begin{tabular}{lrrrrrrrrrrrr}", r"\toprule",
             r"& \multicolumn{8}{c}{Simulated Annealing (10 replicações)} & "
             r"\multicolumn{4}{c}{HiGHS (1800\,s)} \\ \cmidrule(lr){2-9} \cmidrule(l){10-13}",
             r"Inst. & SI & SF méd. & dp & SF máx. & BKV & mel.\,(\%) & desv.\,(\%) & t\,(s) "
             r"& valor & limite & gap\,(\%) & t\,(s) \\ \midrule"]
    devs, devs_best, imps = [], [], []
    for inst in sorted(sa):
        g = sa[inst]
        si = st.mean(int(r["initial"]) for r in g)
        sf = [int(r["best"]) for r in g]
        t = st.mean(float(r["time_total"]) for r in g)
        devs.append(dev(st.mean(sf), inst))
        devs_best.append(dev(max(sf), inst))
        imps.append(improvement(st.mean(sf), si))
        m = mip.get(inst)
        if m:
            solver = [thousands(int(m["value"])), thousands(float(m["bound"])),
                      num(float(m["gap"])) if not math.isnan(float(m["gap"])) else "--",
                      f"{float(m['time_total']):.0f}"]
        else:
            solver = ["--"] * 4
        best = thousands(max(sf))
        if max(sf) > BKV[inst]:
            best = r"\textbf{" + best + "}"
        lines.append(" & ".join([inst, thousands(si), thousands(st.mean(sf)),
                                 num(st.stdev(sf), 0), best, thousands(BKV[inst]),
                                 num(imps[-1]), num(devs[-1]), num(t, 1)] + solver) + r" \\")
    lines += [r"\midrule",
              r"Média & & & & & & " + f"{num(st.mean(imps))} & {num(st.mean(devs))} & & & & & "
              + r"\\", r"\bottomrule", r"\end{tabular}", ""]
    write(out_dir, "resultados.tex", "\n".join(lines))


def tab_proposal(out_dir):
    prop, cal = defaultdict(list), defaultdict(list)
    for r in load("sa_proposta.csv"):
        prop[r["instance"]].append(r)
    for r in load("sa_calibrada.csv"):
        if int(r["seed"]) <= 5:
            cal[r["instance"]].append(r)
    if not prop:
        return
    lines = [r"\begin{tabular}{lrrrrrr}", r"\toprule",
             r"& \multicolumn{3}{c}{Proposta} & \multicolumn{3}{c}{Calibrada} \\ "
             r"\cmidrule(lr){2-4} \cmidrule(l){5-7}",
             r"Inst. & SF méd. & desv.\,(\%) & t\,(s) & SF méd. & desv.\,(\%) & t\,(s) \\ \midrule"]
    dp, dc = [], []
    for inst in sorted(prop):
        a, b = prop[inst], cal.get(inst, [])
        if not b:
            continue
        sa_, sb = st.mean(int(r["best"]) for r in a), st.mean(int(r["best"]) for r in b)
        dp.append(dev(sa_, inst))
        dc.append(dev(sb, inst))
        lines.append(" & ".join([
            inst, thousands(sa_), num(dp[-1]), num(st.mean(float(r["time_total"]) for r in a), 1),
            thousands(sb), num(dc[-1]), num(st.mean(float(r["time_total"]) for r in b), 1)]) + r" \\")
    lines += [r"\midrule", f"Média & & {num(st.mean(dp))} & & & {num(st.mean(dc))} & " + r"\\",
              r"\bottomrule", r"\end{tabular}", ""]
    write(out_dir, "proposta_vs_calibrada.tex", "\n".join(lines))


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "Report", "final", "tabelas")
    os.makedirs(out_dir, exist_ok=True)
    tab_temperatures(out_dir)
    tab_cooling(out_dir)
    tab_operators(out_dir)
    tab_results(out_dir)
    tab_proposal(out_dir)


if __name__ == "__main__":
    main()

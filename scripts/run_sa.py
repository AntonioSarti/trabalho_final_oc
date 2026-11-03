#!/usr/bin/env python3
"""Executa o SA (./jifc) em lote: instâncias x sementes x configurações.

Exemplo:
  scripts/run_sa.py --instances 01 05 09 --seeds 101 102 103 \\
      --config base: --config pmin20:"--pmin 1e-20 --kmax 1000" \\
      --jobs 5 --out results/tuning/stage1.csv --sol-dir results/tuning/sol

Cada configuração é "rótulo:opções extras do ./jifc". As linhas CSV do
./jifc recebem uma coluna adicional "config" com o rótulo. Execuções já
presentes no CSV de saída (mesmo rótulo, instância e semente) são puladas, o
que permite retomar um lote interrompido.
"""
import argparse
import csv
import os
import shlex
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor, as_completed

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def run_one(tag, extra, inst, seed, sol_dir, trace_dir):
    name = f"jifc{inst}"
    sol = os.path.join(sol_dir, f"{name}_{tag}_s{seed}.sol") if sol_dir else os.devnull
    with tempfile.NamedTemporaryFile("r", suffix=".csv") as tmp:
        cmd = [os.path.join(ROOT, "jifc"), sol, "--seed", str(seed), "--name", name,
               "--csv", tmp.name, "--quiet"] + shlex.split(extra)
        if trace_dir:
            cmd += ["--trace", os.path.join(trace_dir, f"{name}_{tag}_s{seed}.csv")]
        with open(os.path.join(ROOT, "instances", f"{name}.dat")) as fin:
            res = subprocess.run(cmd, stdin=fin, stdout=subprocess.DEVNULL,
                                 stderr=subprocess.PIPE, text=True)
        if res.returncode != 0:
            raise RuntimeError(f"{' '.join(cmd)} falhou ({res.returncode}): {res.stderr}")
        rows = list(csv.DictReader(open(tmp.name)))
    row = rows[-1]
    row["config"] = tag
    return row


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--instances", nargs="+", required=True)
    ap.add_argument("--seeds", nargs="+", type=int, required=True)
    ap.add_argument("--config", action="append", required=True,
                    help='"rótulo:opções", p.ex. "a95:--alpha 0.95"')
    ap.add_argument("--jobs", type=int, default=os.cpu_count())
    ap.add_argument("--out", required=True)
    ap.add_argument("--sol-dir", default="")
    ap.add_argument("--trace-dir", default="")
    args = ap.parse_args()

    for d in (os.path.dirname(args.out), args.sol_dir, args.trace_dir):
        if d:
            os.makedirs(d, exist_ok=True)
    done = set()
    if os.path.exists(args.out):
        for r in csv.DictReader(open(args.out)):
            done.add((r["config"], r["instance"], r["seed"]))

    configs = [c.split(":", 1) for c in args.config]
    tasks = [(tag, extra, inst, seed) for tag, extra in configs
             for inst in args.instances for seed in args.seeds
             if (tag, f"jifc{inst}", str(seed)) not in done]
    print(f"{len(tasks)} execuções ({len(done)} já feitas)", file=sys.stderr)

    writer, fout = None, None
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = [pool.submit(run_one, *t, args.sol_dir, args.trace_dir) for t in tasks]
        for k, fut in enumerate(as_completed(futures), 1):
            row = fut.result()
            if writer is None:
                new = not os.path.exists(args.out) or os.path.getsize(args.out) == 0
                fout = open(args.out, "a", newline="")
                writer = csv.DictWriter(fout, fieldnames=["config"] + [k for k in row if k != "config"])
                if new:
                    writer.writeheader()
            writer.writerow(row)
            fout.flush()
            print(f"[{k}/{len(tasks)}] {row['config']} {row['instance']} s{row['seed']}: "
                  f"{row['best']} ({float(row['time_total']):.1f}s)", file=sys.stderr)
    if fout:
        fout.close()


if __name__ == "__main__":
    main()

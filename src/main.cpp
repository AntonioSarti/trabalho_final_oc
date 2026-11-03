// Simulated Annealing para o JIFC.
//
// Convenções da disciplina: a instância é lida da entrada padrão, a melhor
// solução é impressa na saída padrão e o primeiro argumento é o arquivo onde
// ela também é gravada. O resumo da execução vai para a saída de erro.
//
// Formato da solução: na primeira linha, o esplendor e o número k de setores
// cultivados; na segunda, os k setores (1-indexados) em ordem crescente.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "instance.hpp"
#include "sa.hpp"

namespace {

void usage(const char* prog) {
    std::fprintf(stderr,
        "Uso: %s <arquivo_solucao> [opções] < instancia.dat\n"
        "  --seed S         semente (padrão 1)\n"
        "  --alpha A        fator de resfriamento (0.95)\n"
        "  --L F            L = F * |S| iterações por nível (50)\n"
        "  --ncal N         tentativas de calibração N_cal (100)\n"
        "  --p0 P           probabilidade-alvo inicial (0.5)\n"
        "  --pmin P         probabilidade-alvo final (1e-20)\n"
        "  --kmax K         número máximo de níveis (100)\n"
        "  --padd P         probabilidade do Add (0.1)\n"
        "  --premove P      probabilidade do Remove (0.2); Swap = 1 - Add - Remove\n"
        "  --time-limit T   limite de tempo em segundos (0 = sem limite)\n"
        "  --name NOME      rótulo da instância no CSV (padrão: stdin)\n"
        "  --csv ARQ        acrescenta uma linha com as estatísticas ao CSV\n"
        "  --trace ARQ      grava a evolução por nível de temperatura (CSV)\n"
        "  --quiet          não imprime o resumo na saída de erro\n",
        prog);
}

const char* kCsvHeader =
    "instance,n,m,seed,alpha,L,p0,pmin,kmax,padd,premove,initial,best,size,valid,"
    "water,H,d,T0,Tmin,levels,iterations,accepted,no_move,iter_to_best,"
    "time_initial,time_to_best,time_total";

std::string solutionText(const Result& res) {
    std::vector<int> sorted = res.bestSectors;
    std::sort(sorted.begin(), sorted.end());
    std::ostringstream out;
    out << res.bestValue << ' ' << sorted.size() << '\n';
    for (std::size_t i = 0; i < sorted.size(); ++i)
        out << sorted[i] + 1 << (i + 1 == sorted.size() ? '\n' : ' ');
    return out.str();
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2 || argv[1][0] == '-') { usage(argv[0]); return 1; }
    const std::string solPath = argv[1];

    Params prm;
    std::string name = "stdin", csvPath, tracePath;
    bool quiet = false;
    for (int i = 2; i < argc; ++i) {
        std::string opt = argv[i];
        auto next = [&]() -> const char* {
            if (i + 1 >= argc) { usage(argv[0]); std::exit(1); }
            return argv[++i];
        };
        if (opt == "--seed") prm.seed = std::strtoull(next(), nullptr, 10);
        else if (opt == "--alpha") prm.alpha = std::atof(next());
        else if (opt == "--L") prm.lFactor = std::atof(next());
        else if (opt == "--ncal") prm.nCal = std::atoi(next());
        else if (opt == "--p0") prm.p0 = std::atof(next());
        else if (opt == "--pmin") prm.pMin = std::atof(next());
        else if (opt == "--kmax") prm.kMax = std::atoi(next());
        else if (opt == "--padd") prm.probAdd = std::atof(next());
        else if (opt == "--premove") prm.probRemove = std::atof(next());
        else if (opt == "--time-limit") prm.timeLimit = std::atof(next());
        else if (opt == "--name") name = next();
        else if (opt == "--csv") csvPath = next();
        else if (opt == "--trace") tracePath = next();
        else if (opt == "--quiet") quiet = true;
        else { usage(argv[0]); return 1; }
    }

    Instance inst;
    try {
        std::ios::sync_with_stdio(false);
        inst = Instance::read(std::cin);
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "Erro na leitura da instância: %s\n", ex.what());
        return 1;
    }

    Result res = simulatedAnnealing(inst, prm);
    if (!res.feasibleInstance) {
        std::fprintf(stderr, "Instância inviável: a_p + f_p > H\n");
        return 2;
    }

    // Verificação independente da melhor solução.
    int64 checkedValue = 0, water = 0;
    std::string error;
    bool valid = verifySolution(inst, res.bestSectors, checkedValue, water, error);
    if (valid && checkedValue != res.bestValue) { valid = false; error = "valor divergente"; }
    if (!valid) std::fprintf(stderr, "ERRO: solução inválida (%s)\n", error.c_str());

    const std::string text = solutionText(res);
    std::cout << text << std::flush;
    std::ofstream(solPath) << text;

    if (!tracePath.empty()) {
        std::ofstream out(tracePath);
        out << "level,temperature,current,best,acceptance\n";
        for (const auto& t : res.trace)
            out << t.level << ',' << t.temperature << ',' << t.current << ','
                << t.best << ',' << t.acceptance << '\n';
    }

    if (!csvPath.empty()) {
        bool empty = !std::ifstream(csvPath).good() || std::ifstream(csvPath).peek() == EOF;
        FILE* out = std::fopen(csvPath.c_str(), "a");
        if (!out) {
            std::fprintf(stderr, "Erro: não foi possível abrir %s\n", csvPath.c_str());
            return 1;
        }
        if (empty) std::fprintf(out, "%s\n", kCsvHeader);
        std::fprintf(out,
                     "%s,%d,%d,%llu,%g,%g,%g,%g,%d,%g,%g,%lld,%lld,%zu,%d,%lld,%lld,"
                     "%.6f,%.6f,%.6f,%d,%lld,%lld,%lld,%lld,%.4f,%.4f,%.4f\n",
                     name.c_str(), inst.n, inst.m, static_cast<unsigned long long>(prm.seed),
                     prm.alpha, prm.lFactor, prm.p0, prm.pMin, prm.kMax, prm.probAdd,
                     prm.probRemove, static_cast<long long>(res.initialValue),
                     static_cast<long long>(res.bestValue), res.bestSectors.size(),
                     valid ? 1 : 0, static_cast<long long>(water),
                     static_cast<long long>(inst.H), res.scale, res.t0, res.tMin, res.levels,
                     res.iterations, res.accepted, res.noMove, res.iterToBest,
                     res.timeInitial, res.timeToBest, res.totalTime);
        std::fclose(out);
    }

    if (!quiet) {
        std::fprintf(stderr, "Instância:        %s (n=%d, m=%d, p=%d, H=%lld)\n", name.c_str(),
                     inst.n, inst.m, inst.p + 1, static_cast<long long>(inst.H));
        std::fprintf(stderr, "Semente:          %llu\n", static_cast<unsigned long long>(prm.seed));
        std::fprintf(stderr, "Solução gulosa:   %lld (%d setores, %.3f s)\n",
                     static_cast<long long>(res.initialValue), res.initialSize, res.timeInitial);
        std::fprintf(stderr, "Escala d:         %.4f  (T0=%.4f, Tmin=%.4f)\n", res.scale, res.t0,
                     res.tMin);
        std::fprintf(stderr, "Níveis:           %d  (iterações=%lld, aceitas=%lld, sem vizinho=%lld)\n",
                     res.levels, res.iterations, res.accepted, res.noMove);
        std::fprintf(stderr, "Melhor solução:   %lld (%zu setores, W=%lld, %s)\n",
                     static_cast<long long>(res.bestValue), res.bestSectors.size(),
                     static_cast<long long>(water), valid ? "válida" : "INVÁLIDA");
        std::fprintf(stderr, "Tempo até melhor: %.3f s (iteração %lld)\n", res.timeToBest,
                     res.iterToBest);
        std::fprintf(stderr, "Tempo total:      %.3f s%s\n", res.totalTime,
                     res.hitTimeLimit ? " (limite de tempo atingido)" : "");
    }
    return valid ? 0 : 3;
}

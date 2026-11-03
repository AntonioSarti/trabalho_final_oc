#include "sa.hpp"

#include <chrono>
#include <cmath>
#include <optional>

namespace {

enum class Op { Add, Remove, Swap };

struct Move {
    Op op;
    int u = -1;  // setor que sai (Remove/Swap)
    int v = -1;  // setor que entra (Add/Swap)
    int64 delta = 0;
};

// Número de sorteios por rejeição antes de enumerar todos os candidatos.
// Sortear uniformemente num superconjunto e rejeitar os inviáveis produz um
// movimento uniforme entre os viáveis; a enumeração de reserva também é
// uniforme, então a distribuição final é exatamente a da Seção 4.
constexpr int kTriesAdd = 64;
constexpr int kTriesRemove = 64;
constexpr int kTriesSwap = 4096;

int uniformIndex(Rng& rng, int n) {
    return std::uniform_int_distribution<int>(0, n - 1)(rng);
}

double uniformReal(Rng& rng) {
    return std::uniform_real_distribution<double>(0.0, 1.0)(rng);
}

using Clock = std::chrono::steady_clock;

double secondsSince(Clock::time_point start) {
    return std::chrono::duration<double>(Clock::now() - start).count();
}

// Add: v ∉ C com vizinho em C (isto é, v na fronteira) e W(C ∪ {v}) <= H.
// A conectividade é garantida pela adjacência.
bool addFeasible(const Solution& s, int v) {
    return s.waterAfterAdd(v) <= s.instance().H;
}

// Remove: u ∈ C \ {p} que não seja ponto de articulação de G[C].
// W nunca aumenta numa remoção, pois a, f >= 0.
bool removeFeasible(Solution& s, int u) {
    return u != s.instance().p && !s.isArticulation(u);
}

// Swap: u ∈ C \ {p}, v na fronteira. (C \ {u}) ∪ {v} é conexo se v tem
// vizinho em C \ {u} e u não é articulação; se u for articulação, v precisa
// reconectar pelo menos duas componentes, o que é testado por BFS.
bool swapFeasible(Solution& s, int u, int v) {
    const Instance& inst = s.instance();
    if (u == inst.p) return false;
    if (s.waterAfterSwap(u, v) > inst.H) return false;
    int links = s.neighborsInside(v) - (inst.adjacent(u, v) ? 1 : 0);
    if (links == 0) return false;
    if (!s.isArticulation(u)) return true;
    if (links < 2) return false;
    return s.connectedAfterSwap(u, v);
}

std::optional<Move> sampleAdd(Solution& s, Rng& rng) {
    const auto& fr = s.frontier();
    if (fr.size() == 0) return std::nullopt;
    const auto& e = s.instance().e;
    for (int t = 0; t < kTriesAdd; ++t) {
        int v = fr[uniformIndex(rng, fr.size())];
        if (addFeasible(s, v)) return Move{Op::Add, -1, v, e[v]};
    }
    std::vector<int> ok;
    for (int v : fr.items())
        if (addFeasible(s, v)) ok.push_back(v);
    if (ok.empty()) return std::nullopt;
    int v = ok[uniformIndex(rng, static_cast<int>(ok.size()))];
    return Move{Op::Add, -1, v, e[v]};
}

std::optional<Move> sampleRemove(Solution& s, Rng& rng) {
    const auto& mem = s.members();
    if (mem.size() < 2) return std::nullopt;
    const auto& e = s.instance().e;
    for (int t = 0; t < kTriesRemove; ++t) {
        int u = mem[uniformIndex(rng, mem.size())];
        if (removeFeasible(s, u)) return Move{Op::Remove, u, -1, -e[u]};
    }
    std::vector<int> ok;
    for (int u : mem.items())
        if (removeFeasible(s, u)) ok.push_back(u);
    if (ok.empty()) return std::nullopt;
    int u = ok[uniformIndex(rng, static_cast<int>(ok.size()))];
    return Move{Op::Remove, u, -1, -e[u]};
}

std::optional<Move> sampleSwap(Solution& s, Rng& rng) {
    const auto& mem = s.members();
    const auto& fr = s.frontier();
    if (mem.size() < 2 || fr.size() == 0) return std::nullopt;
    const auto& e = s.instance().e;
    for (int t = 0; t < kTriesSwap; ++t) {
        int u = mem[uniformIndex(rng, mem.size())];
        int v = fr[uniformIndex(rng, fr.size())];
        if (swapFeasible(s, u, v)) return Move{Op::Swap, u, v, e[v] - e[u]};
    }
    std::vector<std::pair<int, int>> ok;
    for (int u : mem.items())
        for (int v : fr.items())
            if (swapFeasible(s, u, v)) ok.emplace_back(u, v);
    if (ok.empty()) return std::nullopt;
    auto [u, v] = ok[uniformIndex(rng, static_cast<int>(ok.size()))];
    return Move{Op::Swap, u, v, e[v] - e[u]};
}

// GerarVizinho: sorteia o operador (40/30/30) e, dentro dele, um movimento
// viável uniformemente. Sem movimento viável, não há novo sorteio.
std::optional<Move> generateNeighbor(Solution& s, Rng& rng, const Params& prm) {
    double r = uniformReal(rng);
    if (r < prm.probAdd) return sampleAdd(s, rng);
    if (r < prm.probAdd + prm.probRemove) return sampleRemove(s, rng);
    return sampleSwap(s, rng);
}

void apply(Solution& s, const Move& mv) {
    if (mv.u >= 0) s.remove(mv.u);
    if (mv.v >= 0) s.add(mv.v);
}

bool hasFeasibleMove(Solution& s) {
    for (int v : s.frontier().items())
        if (addFeasible(s, v)) return true;
    for (int u : s.members().items())
        if (removeFeasible(s, u)) return true;
    for (int u : s.members().items())
        for (int v : s.frontier().items())
            if (swapFeasible(s, u, v)) return true;
    return false;
}

// Seção 3: construção gulosa a partir de C = {p}, adicionando o candidato
// adjacente de maior r(u) = e_u / (ΔW(u|C) + ε); empates sorteados.
Solution greedyInitial(const Instance& inst, Rng& rng, double eps) {
    Solution s(inst);
    std::vector<int> ties;
    while (true) {
        double best = -1.0;
        ties.clear();
        int64 w = s.water();
        for (int v : s.frontier().items()) {
            int64 wv = s.waterAfterAdd(v);
            if (wv > inst.H) continue;
            double r = static_cast<double>(inst.e[v]) / (static_cast<double>(wv - w) + eps);
            if (r > best) { best = r; ties.assign(1, v); }
            else if (r == best) ties.push_back(v);
        }
        if (ties.empty()) break;
        s.add(ties[uniformIndex(rng, static_cast<int>(ties.size()))]);
    }
    return s;
}

// Algoritmo 2: caminhada exploratória que aceita todo movimento viável e
// estima a escala d das variações do esplendor.
double calibrateScale(const Solution& start, Rng& rng, const Params& prm) {
    Solution s = start;
    std::vector<int64> deltas;
    const int nMax = prm.nMaxFactor * prm.nCal;
    bool hasWorse = false;
    for (int t = 0; t < nMax && (t < prm.nCal || !hasWorse);) {
        ++t;
        auto mv = generateNeighbor(s, rng, prm);
        if (!mv) continue;
        deltas.push_back(mv->delta);
        hasWorse = hasWorse || mv->delta < 0;
        apply(s, *mv);
    }
    double sum = 0.0;
    int count = 0;
    for (int64 d : deltas)
        if (hasWorse ? d < 0 : d != 0) { sum += std::fabs(static_cast<double>(d)); ++count; }
    return count > 0 ? sum / count : 1.0;
}

}  // namespace

// Algoritmo 1.
Result simulatedAnnealing(const Instance& inst, const Params& prm) {
    const auto start = Clock::now();
    Result res;
    Rng rng(prm.seed);

    if (inst.a[inst.p] + inst.f[inst.p] > inst.H) {
        res.feasibleInstance = false;
        res.totalTime = secondsSince(start);
        return res;
    }

    Solution cur = greedyInitial(inst, rng, prm.epsilon);
    res.timeInitial = secondsSince(start);
    res.initialValue = res.bestValue = cur.splendor();
    res.initialSize = cur.size();
    res.bestSectors = cur.members().items();
    res.timeToBest = res.timeInitial;

    if (!hasFeasibleMove(cur)) {
        res.totalTime = secondsSince(start);
        return res;
    }

    res.scale = calibrateScale(cur, rng, prm);
    res.t0 = -res.scale / std::log(prm.p0);
    res.tMin = -res.scale / std::log(prm.pMin);
    const long long L = std::llround(prm.lFactor * inst.n);

    double T = res.t0;
    int k = 0;
    while (T > res.tMin && k < prm.kMax) {
        long long acceptedLevel = 0, movesLevel = 0;
        for (long long it = 0; it < L; ++it) {
            ++res.iterations;
            auto mv = generateNeighbor(cur, rng, prm);
            if (!mv) { ++res.noMove; continue; }
            ++movesLevel;
            if (mv->delta >= 0 || uniformReal(rng) < std::exp(mv->delta / T)) {
                apply(cur, *mv);
                ++acceptedLevel;
                if (cur.splendor() > res.bestValue) {
                    res.bestValue = cur.splendor();
                    res.bestSectors = cur.members().items();
                    res.iterToBest = res.iterations;
                    res.timeToBest = secondsSince(start);
                }
            }
        }
        res.accepted += acceptedLevel;
        res.trace.push_back({k, T, cur.splendor(), res.bestValue,
                             movesLevel ? static_cast<double>(acceptedLevel) / movesLevel : 0.0});
        T *= prm.alpha;
        ++k;
        if (prm.timeLimit > 0 && secondsSince(start) >= prm.timeLimit) {
            res.hitTimeLimit = true;
            break;
        }
    }
    res.levels = k;
    res.totalTime = secondsSince(start);
    return res;
}

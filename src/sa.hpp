#pragma once

#include <cstdint>
#include <ostream>
#include <random>
#include <vector>

#include "instance.hpp"
#include "solution.hpp"

using Rng = std::mt19937_64;

// Parâmetros do Simulated Annealing. Os padrões são os da configuração
// calibrada (results/tuning); a proposta original usava p0 = 0.8,
// pMin = 1e-3 e operadores 40/30/30.
struct Params {
    std::uint64_t seed = 1;
    double alpha = 0.95;        // resfriamento geométrico
    double lFactor = 50.0;      // L = lFactor * |S|
    int nCal = 100;             // tentativas de calibração
    int nMaxFactor = 10;        // N_max = nMaxFactor * N_cal
    double p0 = 0.5;            // probabilidade-alvo inicial
    double pMin = 1e-20;        // probabilidade-alvo final
    int kMax = 100;             // número máximo de níveis de temperatura
    double probAdd = 0.1;
    double probRemove = 0.2;    // Swap: 1 - probAdd - probRemove
    double epsilon = 1e-6;      // constante da razão gulosa r(u)
    double timeLimit = 0.0;     // segundos; 0 = sem limite
};

struct LevelTrace {
    int level;
    double temperature;
    int64 current;
    int64 best;
    double acceptance;  // movimentos aceitos / iterações com vizinho viável
};

struct Result {
    bool feasibleInstance = true;
    int64 initialValue = 0;
    int initialSize = 0;
    int64 bestValue = 0;
    std::vector<int> bestSectors;  // 0-indexados
    double scale = 0.0;            // d
    double t0 = 0.0, tMin = 0.0;
    int levels = 0;
    long long iterations = 0;
    long long accepted = 0;
    long long noMove = 0;          // iterações sem vizinho viável
    long long iterToBest = 0;
    double timeInitial = 0.0;      // segundos até a solução gulosa
    double timeToBest = 0.0;
    double totalTime = 0.0;
    bool hitTimeLimit = false;
    std::vector<LevelTrace> trace;
};

Result simulatedAnnealing(const Instance& inst, const Params& prm);

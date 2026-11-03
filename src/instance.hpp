#pragma once

#include <cstdint>
#include <istream>
#include <string>
#include <vector>

using int64 = std::int64_t;

// Instância do JIFC. Internamente os setores são indexados de 0 a n-1
// (no arquivo e nas saídas, de 1 a n).
struct Instance {
    int n = 0;
    int m = 0;
    int p = 0;      // setor do Palácio (0-indexado)
    int64 H = 0;    // capacidade do aqueduto
    std::vector<int64> e, a, f;
    std::vector<std::vector<int>> adj;  // listas de adjacência ordenadas

    // Lê no formato descrito em instances/Readme_origem.md.
    static Instance read(std::istream& in);

    bool adjacent(int u, int v) const;
};

// Verifica uma solução a partir do zero (sem estruturas incrementais):
// presença do Palácio, conectividade de G[C] e W(C) <= H.
// Retorna também o esplendor e o consumo W(C) calculados.
bool verifySolution(const Instance& inst, const std::vector<int>& sectors,
                    int64& splendor, int64& water, std::string& error);

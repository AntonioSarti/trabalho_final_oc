#include "instance.hpp"

#include <algorithm>
#include <queue>
#include <stdexcept>

Instance Instance::read(std::istream& in) {
    Instance inst;
    if (!(in >> inst.n >> inst.m >> inst.p >> inst.H))
        throw std::runtime_error("cabeçalho inválido");
    if (inst.n <= 0 || inst.p < 1 || inst.p > inst.n)
        throw std::runtime_error("valores de n/p inválidos");
    inst.p -= 1;

    inst.e.resize(inst.n);
    inst.a.resize(inst.n);
    inst.f.resize(inst.n);
    for (int i = 0; i < inst.n; ++i)
        if (!(in >> inst.e[i] >> inst.a[i] >> inst.f[i]))
            throw std::runtime_error("setor inválido");

    inst.adj.assign(inst.n, {});
    for (int k = 0; k < inst.m; ++k) {
        int i, j;
        if (!(in >> i >> j) || i < 1 || i > inst.n || j < 1 || j > inst.n || i == j)
            throw std::runtime_error("portão inválido");
        inst.adj[i - 1].push_back(j - 1);
        inst.adj[j - 1].push_back(i - 1);
    }
    for (auto& list : inst.adj) std::sort(list.begin(), list.end());
    return inst;
}

bool Instance::adjacent(int u, int v) const {
    const auto& list = adj[u].size() <= adj[v].size() ? adj[u] : adj[v];
    int target = adj[u].size() <= adj[v].size() ? v : u;
    return std::binary_search(list.begin(), list.end(), target);
}

bool verifySolution(const Instance& inst, const std::vector<int>& sectors,
                    int64& splendor, int64& water, std::string& error) {
    std::vector<char> in(inst.n, 0);
    for (int u : sectors) {
        if (u < 0 || u >= inst.n) { error = "setor fora do intervalo"; return false; }
        if (in[u]) { error = "setor repetido"; return false; }
        in[u] = 1;
    }
    if (!in[inst.p]) { error = "Palácio ausente"; return false; }

    // Conectividade por BFS a partir do Palácio.
    std::vector<char> seen(inst.n, 0);
    std::queue<int> q;
    q.push(inst.p);
    seen[inst.p] = 1;
    std::size_t reached = 0;
    while (!q.empty()) {
        int u = q.front();
        q.pop();
        ++reached;
        for (int w : inst.adj[u])
            if (in[w] && !seen[w]) { seen[w] = 1; q.push(w); }
    }
    if (reached != sectors.size()) { error = "G[C] desconexo"; return false; }

    splendor = 0;
    int64 sumA = 0, f1 = 0, f2 = 0;
    for (int u : sectors) {
        splendor += inst.e[u];
        sumA += inst.a[u];
        if (inst.f[u] > f1) { f2 = f1; f1 = inst.f[u]; }
        else if (inst.f[u] > f2) f2 = inst.f[u];
    }
    water = sumA + f1 + f2;
    if (water > inst.H) { error = "capacidade hídrica violada"; return false; }
    return true;
}

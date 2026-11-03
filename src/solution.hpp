#pragma once

#include <array>
#include <vector>

#include "instance.hpp"

// Conjunto de inteiros em [0, n) com inserção, remoção e acesso por posição
// em O(1), o que permite sortear um elemento uniformemente.
class IndexedSet {
public:
    explicit IndexedSet(int n = 0) : pos_(n, -1) {}

    void insert(int x) {
        if (pos_[x] >= 0) return;
        pos_[x] = static_cast<int>(items_.size());
        items_.push_back(x);
    }
    void erase(int x) {
        int i = pos_[x];
        if (i < 0) return;
        int last = items_.back();
        items_[i] = last;
        pos_[last] = i;
        items_.pop_back();
        pos_[x] = -1;
    }
    bool contains(int x) const { return pos_[x] >= 0; }
    int size() const { return static_cast<int>(items_.size()); }
    int operator[](int i) const { return items_[i]; }
    const std::vector<int>& items() const { return items_; }

private:
    std::vector<int> items_;
    std::vector<int> pos_;
};

// Solução C ⊆ S do JIFC com as estruturas auxiliares da Seção 2.1:
// vetor in_solution, E(C), A(C) e os maiores valores de f em C. Mantém
// também a fronteira N(C) = {v ∉ C : v tem vizinho em C} e, sob demanda,
// os pontos de articulação de G[C], usados para testar conectividade.
class Solution {
public:
    explicit Solution(const Instance& inst);  // C = {p}

    const Instance& instance() const { return *inst_; }
    bool contains(int u) const { return in_[u]; }
    int size() const { return members_.size(); }
    const IndexedSet& members() const { return members_; }
    const IndexedSet& frontier() const { return frontier_; }
    int neighborsInside(int v) const { return inside_[v]; }

    int64 splendor() const { return E_; }
    int64 water() const { return A_ + top2With(-1, -1); }  // W(C)

    // W da solução resultante de cada movimento, em O(1).
    int64 waterAfterAdd(int v) const { return A_ + inst_->a[v] + top2With(-1, v); }
    int64 waterAfterRemove(int u) const { return A_ - inst_->a[u] + top2With(u, -1); }
    int64 waterAfterSwap(int u, int v) const {
        return A_ - inst_->a[u] + inst_->a[v] + top2With(u, v);
    }

    // u é ponto de articulação de G[C]? (recalculado após mudanças)
    bool isArticulation(int u);
    // (C \ {u}) ∪ {v} é conexo? Requer u ∈ C \ {p} e v na fronteira.
    bool connectedAfterSwap(int u, int v);

    void add(int v);
    void remove(int u);

private:
    int64 top2With(int skip, int extra) const;
    void topInsert(int x);
    void topRebuild();
    void computeArticulation();

    const Instance* inst_;
    std::vector<char> in_;
    std::vector<int> inside_;  // número de vizinhos em C
    IndexedSet members_;
    IndexedSet frontier_;
    int64 E_ = 0;
    int64 A_ = 0;
    // Os três maiores valores de f em C (índices, -1 se ausente). Três em vez
    // de dois para obter em O(1) o top_2 após remover um setor (Swap/Remove).
    std::array<int, 3> top_{{-1, -1, -1}};

    // Árvore DFS de G[C] (Tarjan): tempos de descoberta, low-link, maior
    // tempo da subárvore, filhos em ordem de descoberta e número de filhos
    // c com low[c] >= disc[u], isto é, subárvores que se separam sem u.
    bool apDirty_ = true;
    std::vector<char> isAP_;
    std::vector<int> disc_, low_, last_, parent_;
    std::vector<int> order_;
    std::vector<int> childBegin_, childCount_, children_;
    std::vector<int> separated_;
    std::vector<int> mark_;
    int stamp_ = 0;
};

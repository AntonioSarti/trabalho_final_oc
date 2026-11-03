#include "solution.hpp"

#include <algorithm>

Solution::Solution(const Instance& inst)
    : inst_(&inst),
      in_(inst.n, 0),
      inside_(inst.n, 0),
      members_(inst.n),
      frontier_(inst.n),
      isAP_(inst.n, 0),
      disc_(inst.n, -1),
      low_(inst.n, 0),
      last_(inst.n, 0),
      parent_(inst.n, -1),
      childBegin_(inst.n, 0),
      childCount_(inst.n, 0),
      separated_(inst.n, 0),
      mark_(inst.n, 0) {
    add(inst.p);
}

int64 Solution::top2With(int skip, int extra) const {
    int64 b1 = 0, b2 = 0;  // f >= 0, então 0 representa "ausente"
    auto push = [&](int x) {
        if (x < 0 || x == skip) return;
        int64 v = inst_->f[x];
        if (v > b1) { b2 = b1; b1 = v; }
        else if (v > b2) b2 = v;
    };
    for (int x : top_) push(x);
    push(extra);
    return b1 + b2;
}

void Solution::topInsert(int x) {
    for (int k = 0; k < 3; ++k) {
        if (top_[k] < 0) { top_[k] = x; return; }
        if (inst_->f[x] > inst_->f[top_[k]]) std::swap(x, top_[k]);
    }
}

void Solution::topRebuild() {
    top_ = {{-1, -1, -1}};
    for (int x : members_.items()) topInsert(x);
}

void Solution::add(int v) {
    in_[v] = 1;
    members_.insert(v);
    frontier_.erase(v);
    for (int w : inst_->adj[v])
        if (++inside_[w] == 1 && !in_[w]) frontier_.insert(w);
    E_ += inst_->e[v];
    A_ += inst_->a[v];
    topInsert(v);
    apDirty_ = true;
}

void Solution::remove(int u) {
    in_[u] = 0;
    members_.erase(u);
    for (int w : inst_->adj[u])
        if (--inside_[w] == 0 && !in_[w]) frontier_.erase(w);
    if (inside_[u] > 0) frontier_.insert(u);
    E_ -= inst_->e[u];
    A_ -= inst_->a[u];
    if (std::find(top_.begin(), top_.end(), u) != top_.end()) topRebuild();
    apDirty_ = true;
}

bool Solution::isArticulation(int u) {
    if (apDirty_) computeArticulation();
    return isAP_[u];
}

// Algoritmo de Tarjan (DFS iterativa) restrito a G[C], com raiz no Palácio.
void Solution::computeArticulation() {
    struct Frame { int u; std::size_t next; };
    for (int x : members_.items()) {
        disc_[x] = -1;
        isAP_[x] = 0;
        childCount_[x] = 0;
        separated_[x] = 0;
    }

    const int root = inst_->p;
    int timer = 0;
    order_.clear();
    std::vector<Frame> stack;
    stack.reserve(members_.size());
    disc_[root] = low_[root] = timer++;
    parent_[root] = -1;
    order_.push_back(root);
    stack.push_back({root, 0});
    while (!stack.empty()) {
        Frame& fr = stack.back();
        int u = fr.u;
        if (fr.next < inst_->adj[u].size()) {
            int w = inst_->adj[u][fr.next++];
            if (!in_[w]) continue;
            if (disc_[w] < 0) {
                disc_[w] = low_[w] = timer++;
                parent_[w] = u;
                order_.push_back(w);
                stack.push_back({w, 0});
            } else if (w != parent_[u]) {
                low_[u] = std::min(low_[u], disc_[w]);
            }
        } else {
            stack.pop_back();
            last_[u] = timer - 1;
            int par = parent_[u];
            if (par < 0) continue;
            low_[par] = std::min(low_[par], low_[u]);
            ++childCount_[par];
            if (low_[u] >= disc_[par]) ++separated_[par];
        }
    }

    // Filhos de cada vértice em ordem de descoberta, num vetor contíguo.
    int offset = 0;
    for (int x : order_) {
        childBegin_[x] = offset;
        offset += childCount_[x];
        childCount_[x] = 0;
    }
    children_.resize(offset);
    for (std::size_t t = 1; t < order_.size(); ++t) {
        int c = order_[t];
        int par = parent_[c];
        children_[childBegin_[par] + childCount_[par]++] = c;
    }

    for (int x : order_)
        isAP_[x] = x == root ? childCount_[x] >= 2 : separated_[x] >= 1;
    apDirty_ = false;
}

// Sem u (não raiz), G[C] se divide na componente que contém a raiz e em uma
// componente por filho c de u com low[c] >= disc[u]. O conjunto
// (C \ {u}) ∪ {v} é conexo se, e somente se, v tem vizinho em todas elas.
// A componente de cada vizinho w é identificada pelo intervalo de tempos de
// descoberta da subárvore DFS que o contém, sem percorrer G[C].
bool Solution::connectedAfterSwap(int u, int v) {
    if (apDirty_) computeArticulation();
    const int needed = separated_[u] + 1;
    const int* first = children_.data() + childBegin_[u];
    const int* end = first + childCount_[u];
    ++stamp_;
    int covered = 0;
    bool rootSide = false;
    for (int w : inst_->adj[v]) {
        if (!in_[w] || w == u) continue;
        int comp = -1;  // -1: componente da raiz
        if (disc_[w] > disc_[u] && disc_[w] <= last_[u]) {
            // Último filho c de u com disc[c] <= disc[w].
            const int* it = std::upper_bound(first, end, disc_[w],
                [&](int d, int c) { return d < disc_[c]; });
            int c = *(it - 1);
            if (low_[c] >= disc_[u]) comp = c;
        }
        if (comp < 0) {
            if (rootSide) continue;
            rootSide = true;
        } else {
            if (mark_[comp] == stamp_) continue;
            mark_[comp] = stamp_;
        }
        if (++covered == needed) return true;
    }
    return false;
}

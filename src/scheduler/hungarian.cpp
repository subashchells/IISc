#include "hungarian.h"
#include <limits>
#include <stdexcept>
#include <vector>

// Jonker–Volgenant with row/column potentials. 1-indexed internally
// (row 0 / col 0 are sentinels). Each outer iteration adds one row by
// finding a shortest augmenting path through unmatched columns.
std::vector<int>
hungarian_min(const std::vector<std::vector<double>>& a)
{
    const int n = static_cast<int>(a.size());
    if (n == 0) return {};
    for (const auto& row : a)
        if (static_cast<int>(row.size()) != n)
            throw std::invalid_argument("hungarian_min: matrix must be square");

    const double INF = std::numeric_limits<double>::infinity();
    std::vector<double> u(n + 1, 0.0), v(n + 1, 0.0);
    std::vector<int>    p(n + 1, 0),   way(n + 1, 0);

    for (int i = 1; i <= n; ++i) {
        p[0] = i;
        int j0 = 0;
        std::vector<double> minv(n + 1, INF);
        std::vector<char>   used(n + 1, 0);

        do {
            used[j0] = 1;
            const int i0 = p[j0];
            double delta = INF;
            int    j1    = -1;

            for (int j = 1; j <= n; ++j) {
                if (used[j]) continue;
                const double cur = a[i0 - 1][j - 1] - u[i0] - v[j];
                if (cur < minv[j]) { minv[j] = cur; way[j] = j0; }
                if (minv[j] < delta) { delta = minv[j]; j1 = j; }
            }

            for (int j = 0; j <= n; ++j) {
                if (used[j]) { u[p[j]] += delta; v[j] -= delta; }
                else         { minv[j] -= delta; }
            }
            j0 = j1;
        } while (p[j0] != 0);

        // Reverse the augmenting path.
        do {
            const int j1 = way[j0];
            p[j0] = p[j1];
            j0    = j1;
        } while (j0 != 0);
    }

    std::vector<int> assignment(n, -1);
    for (int j = 1; j <= n; ++j)
        if (p[j] != 0) assignment[p[j] - 1] = j - 1;
    return assignment;
}

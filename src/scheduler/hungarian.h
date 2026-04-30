#pragma once
#include <vector>

// Min-cost assignment on a square cost matrix. Jonker–Volgenant variant
// of Hungarian with row & column potentials, O(n³).
//
// Tolerates large finite sentinels for "forbidden" cells. Tie-break is
// stable: equal-cost matchings prefer ascending column index.
//
// Input  : non-empty n × n matrix.
// Output : A[i] = j means row i is paired with column j; each j once.
std::vector<int> hungarian_min(const std::vector<std::vector<double>>& cost);

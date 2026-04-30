#include "topological.h"
#include <queue>
#include <stdexcept>

using namespace std;

// Kahn's algorithm. O(V + E). Throws on cycle.
vector<string> topological_sort(const DirectedGraph& graph) {
    auto deg = graph.in_degrees();
    queue<string> ready;
    for (const auto& [node, count] : deg)
        if (count == 0) ready.push(node);

    vector<string> order;
    while (!ready.empty()) {
        string node = ready.front();
        ready.pop();
        order.push_back(node);
        for (const auto& next : graph.neighbors(node)) {
            deg[next]--;
            if (deg[next] == 0) ready.push(next);
        }
    }

    if (static_cast<int>(order.size()) != graph.node_count())
        throw runtime_error("Cycle in task dependencies");
    return order;
}

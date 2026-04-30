#include "graph.h"

using namespace std;

void DirectedGraph::add_node(const string& id) {
    if (adj_.find(id) == adj_.end())
        adj_[id] = {};
}
void DirectedGraph::add_edge(const string& from, const string& to) {
    add_node(from);
    add_node(to);
    adj_[from].push_back(to);
}
const vector<string>& DirectedGraph::neighbors(const string& id) const {
    return adj_.at(id);
}
unordered_map<string, int> DirectedGraph::in_degrees() const {
    unordered_map<string, int> deg;
    for (const auto& [node, _] : adj_)
        deg[node] = 0;
    for (const auto& [node, edges] : adj_)
        for (const auto& to : edges)
            deg[to]++;
    return deg;
}
bool DirectedGraph::has_node(const string& id) const {
    return adj_.find(id) != adj_.end();
}
int DirectedGraph::node_count() const {
    return static_cast<int>(adj_.size());
}

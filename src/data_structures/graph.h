#pragma once
#include <string>
#include <unordered_map>
#include <vector>
class DirectedGraph {
public:
  void add_node(const std::string &id);
  void add_edge(const std::string &from, const std::string &to);

  const std::vector<std::string> &neighbors(const std::string &id) const;
  std::unordered_map<std::string, int> in_degrees() const;

  bool has_node(const std::string &id) const;
  int node_count() const;
private:
  std::unordered_map<std::string, std::vector<std::string>> adj_;
};

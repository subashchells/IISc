#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "../data_structures/graph.h"
#include "../models/models.h"
struct DepEdge {
  std::string from;
  std::string to;
};
struct WardData {
  std::vector<Patient> patients;
  std::vector<Worker>  workers;
};
std::unordered_map<std::string, Disease> load_diseases(const std::string &path);
std::unordered_map<std::string, Task>    load_tests   (const std::string &path);
std::vector<DepEdge>                     load_dependencies(const std::string &path);
WardData                                 load_ward    (const std::string &path);
void build_ward(WardData &ward,
                const std::unordered_map<std::string, Disease> &diseases,
                const std::unordered_map<std::string, Task> &test_templates,
                const std::vector<DepEdge> &dep_edges,
                std::unordered_map<std::string, Task> &out_tasks,
                DirectedGraph &out_graph,
                std::unordered_map<std::string, std::string> &task_to_patient);

void build_patient_tasks(
    Patient &patient,
    const std::vector<std::string> &chosen_test_ids,
    const std::vector<std::string> &done_test_ids,
    int base_priority,
    const std::unordered_map<std::string, Task> &test_templates,
    const std::vector<DepEdge> &dep_edges,
    std::unordered_map<std::string, Task> &out_tasks,
    DirectedGraph &out_graph,
    std::unordered_map<std::string, std::string> &task_to_patient);

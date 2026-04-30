#include "loader.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
using namespace std;
static string read_file(const string &path) {
  ifstream ifs(path);
  if (!ifs) throw runtime_error("Cannot open file: " + path);
  ostringstream oss;
  oss << ifs.rdbuf();
  return oss.str();
}
static Role parse_role(const string &s) {
  if (s == "PGY1")   return Role::PGY1;
  if (s == "INTERN") return Role::INTERN;
  if (s == "NURSE")  return Role::NURSE;
  throw runtime_error("Unknown role: " + s + " (PGY1, INTERN, NURSE only)");
}
static Acuity parse_acuity(const string &s) {
  if (s == "CRITICAL") return Acuity::CRITICAL;
  if (s == "HIGH")     return Acuity::HIGH;
  if (s == "MEDIUM")   return Acuity::MEDIUM;
  if (s == "LOW")      return Acuity::LOW;
  throw runtime_error("Unknown acuity: " + s);
}
unordered_map<string, Disease> load_diseases(const string &path) {
  nlohmann::json root = nlohmann::json::parse(read_file(path));
  unordered_map<string, Disease> diseases;
  for (const auto &item : root) {
    Disease d;
    d.id            = item.at("id").get<string>();
    d.name          = item.at("name").get<string>();
    d.base_priority = item.at("base_priority").get<int>();
    for (const auto &tid : item.at("test_ids"))
      d.test_ids.push_back(tid.get<string>());
    diseases[d.id] = move(d);
  }
  return diseases;
}
unordered_map<string, Task> load_tests(const string &path) {
  nlohmann::json root = nlohmann::json::parse(read_file(path));
  unordered_map<string, Task> tests;
  for (const auto &item : root) {
    Task t;
    t.id               = item.at("id").get<string>();
    t.name             = item.at("name").get<string>();
    t.required_role    = parse_role(item.at("required_role").get<string>());
    t.duration_minutes = item.at("duration_minutes").get<int>();
    t.priority         = 0;  // set per-patient by build_ward
    t.status           = Status::PENDING;
    tests[t.id] = move(t);
  }
  return tests;
}
vector<DepEdge> load_dependencies(const string &path) {
  nlohmann::json root = nlohmann::json::parse(read_file(path));
  vector<DepEdge> edges;
  for (const auto &item : root)
    edges.push_back({item.at("from").get<string>(),
                     item.at("to").get<string>()});
  return edges;
}
WardData load_ward(const string &path) {
  nlohmann::json root = nlohmann::json::parse(read_file(path));
  WardData ward;
  for (const auto &p : root.at("patients")) {
    Patient patient;
    patient.id         = p.at("id").get<string>();
    patient.name       = p.at("name").get<string>();
    patient.age        = p.at("age").get<int>();
    patient.complaint  = p.at("complaint").get<string>();
    patient.acuity     = parse_acuity(p.at("acuity").get<string>());
    patient.disease_id = p.at("disease_id").get<string>();
    if (p.contains("test_overrides")) {
      for (const auto &tid : p.at("test_overrides"))
        patient.test_overrides.push_back(tid.get<string>());
    }
    ward.patients.push_back(move(patient));
  }
  for (const auto &w : root.at("workers")) {
    Worker worker;
    worker.id                 = w.at("id").get<string>();
    worker.name               = w.at("name").get<string>();
    worker.role               = parse_role(w.at("role").get<string>());
    worker.shift_minutes_left = w.at("shift_minutes_left").get<int>();
    worker.available_at       = 0;
    ward.workers.push_back(move(worker));
  }
  return ward;
}
void build_patient_tasks(
    Patient &patient, const vector<string> &chosen_test_ids,
    const vector<string> &done_test_ids, int base_priority,
    const unordered_map<string, Task> &test_templates,
    const vector<DepEdge> &dep_edges,
    unordered_map<string, Task> &out_tasks, DirectedGraph &out_graph,
    unordered_map<string, string> &task_to_patient) {
  unordered_set<string> needed(chosen_test_ids.begin(), chosen_test_ids.end());
  unordered_set<string> done  (done_test_ids.begin(),   done_test_ids.end());
  const string suffix = "_" + patient.id;
  for (const auto &test_id : chosen_test_ids) {
    auto tit = test_templates.find(test_id);
    if (tit == test_templates.end()) {
      cerr << "[WARN] Unknown test " << test_id << " — skipping\n";
      continue;
    }
    Task task = tit->second;
    task.id              = test_id + suffix;
    task.priority        = base_priority;
    task.status          = done.count(test_id) ? Status::DONE : Status::PENDING;
    task.assigned_worker = "";

    out_tasks[task.id] = move(task);
    out_graph.add_node(test_id + suffix);
    task_to_patient[test_id + suffix] = patient.id;
  }
  for (const auto &edge : dep_edges)
    if (needed.count(edge.from) && needed.count(edge.to))
      out_graph.add_edge(edge.from + suffix, edge.to + suffix);
}
void build_ward(WardData &ward,
                const unordered_map<string, Disease> &diseases,
                const unordered_map<string, Task> &test_templates,
                const vector<DepEdge> &dep_edges,
                unordered_map<string, Task> &out_tasks,
                DirectedGraph &out_graph,
                unordered_map<string, string> &task_to_patient) {
  for (auto &patient : ward.patients) {
    if (patient.disease_id.empty()) continue;
    auto dit = diseases.find(patient.disease_id);
    if (dit == diseases.end()) {
      cerr << "[WARN] Unknown disease " << patient.disease_id
           << " for patient " << patient.id << " — skipping\n";
      continue;
    }
    const Disease &disease = dit->second;
    const vector<string> &test_ids = patient.test_overrides.empty()
        ? disease.test_ids : patient.test_overrides;
    build_patient_tasks(patient, test_ids, {}, disease.base_priority,
                        test_templates, dep_edges, out_tasks, out_graph,
                        task_to_patient);
  }
}

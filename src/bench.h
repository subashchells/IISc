#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "data/loader.h"
#include "data_structures/graph.h"
#include "models/models.h"

// Default OT budget; positive overtime makes a run infeasible.
constexpr int kDefaultOvertimeThresholdMinutes = 0;

// Each bench runs end-to-end on a deep copy of ward state and prints to stdout.

void run_legacy_bench(
    const WardData &                                    ward,
    const std::unordered_map<std::string, Task> &       tasks,
    const DirectedGraph &                               dep_graph,
    const std::unordered_map<std::string, std::string> &task_to_patient,
    int                                                 overtime_threshold_minutes);

void run_2x2_bench(
    const WardData &                                    ward,
    const std::unordered_map<std::string, Task> &       tasks,
    const DirectedGraph &                               dep_graph,
    const std::unordered_map<std::string, std::string> &task_to_patient,
    int                                                 overtime_threshold_minutes);

void run_pareto_sweep(
    const WardData &                                    ward,
    const std::unordered_map<std::string, Task> &       tasks,
    const DirectedGraph &                               dep_graph,
    const std::unordered_map<std::string, std::string> &task_to_patient,
    int                                                 overtime_threshold_minutes);

void run_fairness_bench(
    const WardData &                                    ward,
    const std::unordered_map<std::string, Task> &       tasks,
    const DirectedGraph &                               dep_graph,
    const std::unordered_map<std::string, std::string> &task_to_patient,
    int                                                 overtime_threshold_minutes);

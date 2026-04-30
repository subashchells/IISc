#pragma once
#include <string>
#include <unordered_map>
#include "data/loader.h"
#include "data_structures/graph.h"
#include "models/models.h"

// Per-acuity-tier timing analysis across 5 matchers (Greedy, Hungarian,
// Fair α=0.75/0.50/0.25/0.00). For each tier (CRIT/HIGH/MED/LOW), reports
// first-task-start and last-task-finish as mean / p50 / p90 / max, names
// the worst-served CRIT patient, and prints a Δ-vs-Greedy delta table on
// the urgent-care axes. Priority ordering held fixed across all matchers.
void run_urgency_bench(
    const WardData &                                    ward,
    const std::unordered_map<std::string, Task> &       tasks,
    const DirectedGraph &                               dep_graph,
    const std::unordered_map<std::string, std::string> &task_to_patient,
    int                                                 overtime_threshold_minutes);

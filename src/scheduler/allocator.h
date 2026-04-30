#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include "../data_structures/graph.h"
#include "../models/models.h"
#include "strategies.h"

struct ScheduleEntry {
    std::string task_id;
    std::string task_name;
    std::string worker_id;
    std::string worker_name;
    std::string patient_id;
    int         start_time;
    int         finish_time;
    int         effective_priority;
    bool        over_capacity = false;  // task pushed worker past shift
};

// Surfaces tasks dropped because no roster worker has the required role —
// otherwise they (and their dependents) vanish silently.
struct UnscheduledTask {
    std::string task_id;
    std::string task_name;
    std::string patient_id;
    std::string reason;
};

struct CapacityViolation {
    std::string task_id;
    std::string task_name;
    std::string worker_id;
    std::string worker_name;
    std::string patient_id;
    int         shift_remaining_before;
    int         task_duration;
    int         overtime_added;
};

// `threshold_minutes` is input; the rest are filled by run_engine.
struct CapacityReport {
    int  threshold_minutes        = 0;
    bool capacity_feasible        = true;
    int  total_overtime_minutes   = 0;
    int  over_capacity_task_count = 0;
    std::vector<CapacityViolation>            violations;
    std::unordered_map<std::string, int>      per_worker_overtime;
};

// Cross-strategy benchmarking signal; populated when caller passes a non-null
// pointer to run_engine.
struct EngineMetrics {
    long long scheduler_runtime_ns    = 0;
    int       makespan_minutes        = 0;
    int       max_per_worker_overtime = 0;
    double    stdev_overtime_minutes  = 0.0;
    int       deferred_event_count    = 0;
    int       tick_count              = 0;
};

// Builds a LexKey for one ready task. See policies.h.
using LexPolicyFn = std::function<LexKey(
    const Task&, Acuity, int /*age*/, std::uint32_t /*seed*/)>;

// `policy` may be empty → engine falls back to default_lex_policy.
// Pointers must be non-null and outlive the run_engine call.
struct EngineConfig {
    OrderingStrategy* ordering;
    MatchingStrategy* matching;
    LexPolicyFn       policy;
    std::uint32_t     policy_seed = 0;
    std::string       label;
};

// Loop: drain ≤ |workers| from ordering → match → commit ASSIGNED in order
// (Greedy may stack on one worker; Hungarian is 1-to-1) → re-enqueue
// DEFERRED, surface NO_ROLE_MATCH → unlock dependents → repeat.
std::vector<ScheduleEntry> run_engine(
    std::vector<Patient>&                               patients,
    std::vector<Worker>&                                workers,
    std::unordered_map<std::string, Task>&              tasks,
    const DirectedGraph&                                dep_graph,
    const std::unordered_map<std::string, std::string>& task_to_patient,
    EngineConfig                                        config,
    std::vector<UnscheduledTask>*                       unscheduled     = nullptr,
    CapacityReport*                                     capacity_report = nullptr,
    EngineMetrics*                                      metrics         = nullptr);

// Backward-compatible wrappers — Cell C and Cell A of the 2×2 matrix.
std::vector<ScheduleEntry> run_simulation(
    std::vector<Patient>&                               patients,
    std::vector<Worker>&                                workers,
    std::unordered_map<std::string, Task>&              tasks,
    const DirectedGraph&                                dep_graph,
    const std::unordered_map<std::string, std::string>& task_to_patient,
    std::vector<UnscheduledTask>*                       unscheduled     = nullptr,
    CapacityReport*                                     capacity_report = nullptr);

std::vector<ScheduleEntry> run_simulation_fcfs(
    std::vector<Patient>&                               patients,
    std::vector<Worker>&                                workers,
    std::unordered_map<std::string, Task>&              tasks,
    const DirectedGraph&                                dep_graph,
    const std::unordered_map<std::string, std::string>& task_to_patient,
    std::vector<UnscheduledTask>*                       unscheduled     = nullptr,
    CapacityReport*                                     capacity_report = nullptr);

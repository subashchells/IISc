#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include "../models/models.h"
#include "strategies.h"

// LexPolicy: (task, acuity, age, seed) → LexKey. `seed` is folded into the
// last lex dim as a per-(task, seed) FNV-1a hash for deterministic
// tie-breaking that's stable per run and varied across runs.
using LexPolicyFn = std::function<LexKey(
    const Task&, Acuity, int /*age*/, std::uint32_t /*seed*/)>;

struct NamedPolicy {
    std::string  name;
    std::string  description;
    LexPolicyFn  fn;
};

// Legacy {acuity*10 + task.priority, hash} as a single-dim key.
LexKey default_lex_policy(const Task& t, Acuity a, int age, std::uint32_t seed);

// Hand-picked catalog; each policy has a different *dominant* dim.
std::vector<NamedPolicy> policy_catalog();

constexpr int kPolicySeedCount = 5;
constexpr int kFrontierTopK    = 6;

struct Objectives {
    double acuity_metric;        // mean last-finish for CRIT+HIGH (smaller = better)
    int    makespan_minutes;
    int    max_overtime_minutes;
};

struct ScheduleEntry;
struct CapacityReport;

Objectives compute_objectives(
    const std::vector<ScheduleEntry>& schedule,
    const std::vector<Patient>&       patients,
    const std::vector<Worker>&        workers,
    const CapacityReport&             capacity_report);

// Indices of non-dominated points (lower-better on every axis).
std::vector<int> pareto_frontier(const std::vector<Objectives>& points);

// NSGA-II 3D crowding distance. Retains axis-extreme corners; fills
// remaining slots with the most spread-out interior points. ≤ k indices.
std::vector<int> top_k_by_crowding(
    const std::vector<Objectives>& points,
    const std::vector<int>&        frontier_indices,
    int                            k);

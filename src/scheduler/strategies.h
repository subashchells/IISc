#pragma once
#include <deque>
#include <string>
#include <vector>
#include "../models/models.h"

// Engine loop:
//   drain a batch from OrderingStrategy → match via MatchingStrategy →
//   commit (engine) → unlock dependents → push back into ordering.

// Lex ordering key — each dim: smaller = more urgent. Comparison via
// std::vector::operator<. Dimensions and meaning are set by the active
// LexPolicy (see policies.h). Lex orderings reach Pareto points that no
// scalar weighted sum can.
using LexKey = std::vector<int>;

struct ReadyTask {
    Task*        task;             // non-owning, into engine's tasks map
    std::string  patient_id;
    LexKey       key;              // policy-dependent ordering key
    int          legacy_priority;  // stable acuity*10+priority for display & Hungarian
};

// ASSIGNED      : engine commits (task_idx, worker_idx).
// DEFERRED      : matcher couldn't place this tick; re-enter ordering.
// NO_ROLE_MATCH : permanently unscheduled; surface to caller.
enum class MatchOutcome { ASSIGNED, DEFERRED, NO_ROLE_MATCH };

struct MatchResult {
    int          task_idx;
    int          worker_idx;  // -1 when not ASSIGNED
    MatchOutcome outcome;
};

// ── Ordering ─────────────────────────────────────────────────────────────────

class OrderingStrategy {
public:
    virtual ~OrderingStrategy() = default;
    virtual void                   seed(const std::vector<ReadyTask>& roots) = 0;
    virtual void                   push(const ReadyTask& r) = 0;
    virtual bool                   empty() const = 0;
    virtual std::vector<ReadyTask> drain_up_to(std::size_t k) = 0;
    virtual std::string            name() const = 0;
};

class FcfsOrdering : public OrderingStrategy {
public:
    void                   seed(const std::vector<ReadyTask>& roots) override;
    void                   push(const ReadyTask& r) override;
    bool                   empty() const override;
    std::vector<ReadyTask> drain_up_to(std::size_t k) override;
    std::string            name() const override { return "FCFS"; }
private:
    std::deque<ReadyTask> q_;
};

class PriorityOrdering : public OrderingStrategy {
public:
    void                   seed(const std::vector<ReadyTask>& roots) override;
    void                   push(const ReadyTask& r) override;
    bool                   empty() const override;
    std::vector<ReadyTask> drain_up_to(std::size_t k) override;
    std::string            name() const override { return "Priority"; }
private:
    std::vector<ReadyTask> heap_;
};

// ── Matching ─────────────────────────────────────────────────────────────────

class MatchingStrategy {
public:
    virtual ~MatchingStrategy() = default;
    // Decide one assignment per batch task; does NOT mutate `workers`.
    // Greedy may return the same worker_idx multiple times (engine commits
    // in order, advancing available_at). Hungarian is 1-to-1; overflow tasks
    // come back as DEFERRED.
    virtual std::vector<MatchResult> match(
        const std::vector<ReadyTask>& batch,
        const std::vector<Worker>&    workers) = 0;
    virtual std::string name() const = 0;
    // Engine calls once before the first match() of a run. Default no-op.
    virtual void prepare(const std::vector<Worker>& initial_workers) {
        (void)initial_workers;
    }
};

// Earliest-free, with stack-within-tick.
class GreedyMatcher : public MatchingStrategy {
public:
    std::vector<MatchResult> match(
        const std::vector<ReadyTask>& batch,
        const std::vector<Worker>&    workers) override;
    std::string name() const override { return "Greedy"; }
};

// Score = alpha·available_at + (1-alpha)·(-shift_minutes_left).
//   alpha=1 → pure greedy; alpha=0 → max-min water-filling.
class FairGreedyMatcher : public MatchingStrategy {
public:
    explicit FairGreedyMatcher(double alpha = 0.0) : alpha_(alpha) {}
    std::vector<MatchResult> match(
        const std::vector<ReadyTask>& batch,
        const std::vector<Worker>&    workers) override;
    std::string name() const override;
private:
    double alpha_;
};

// 1-to-1 Hungarian on a (T+W)×(T+W) padded cost matrix:
//   cost[i][j] = importance(task_i) × max(0, worker_j.available_at) when
//   role matches, +∞ on role mismatch, finite penalty on dummy padding.
// Importance comes from legacy_priority (stable across LexPolicies).
class HungarianMatcher : public MatchingStrategy {
public:
    std::vector<MatchResult> match(
        const std::vector<ReadyTask>& batch,
        const std::vector<Worker>&    workers) override;
    std::string name() const override { return "Hungarian"; }
};

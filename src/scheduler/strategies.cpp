#include "strategies.h"
#include "hungarian.h"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <string>

using namespace std;

// ── FCFS ─────────────────────────────────────────────────────────────────────

void FcfsOrdering::seed(const vector<ReadyTask>& roots) {
    // Lex root order keeps FCFS deterministic across hash-map iteration.
    vector<ReadyTask> sorted = roots;
    sort(sorted.begin(), sorted.end(),
         [](const ReadyTask& a, const ReadyTask& b) {
             return a.task->id < b.task->id;
         });
    for (const auto& r : sorted) q_.push_back(r);
}

void FcfsOrdering::push(const ReadyTask& r) { q_.push_back(r); }
bool FcfsOrdering::empty() const             { return q_.empty(); }

vector<ReadyTask> FcfsOrdering::drain_up_to(size_t k) {
    vector<ReadyTask> out;
    out.reserve(min(k, q_.size()));
    while (!q_.empty() && out.size() < k) {
        out.push_back(q_.front());
        q_.pop_front();
    }
    return out;
}

// ── Priority (min-heap on LexKey) ────────────────────────────────────────────

// std::*_heap is max-heap; invert with "greater". task_id breaks key ties.
static bool prio_greater(const ReadyTask& a, const ReadyTask& b) {
    if (a.key != b.key) return a.key > b.key;
    return a.task->id > b.task->id;
}

void PriorityOrdering::seed(const vector<ReadyTask>& roots) {
    heap_.assign(roots.begin(), roots.end());
    make_heap(heap_.begin(), heap_.end(), prio_greater);
}

void PriorityOrdering::push(const ReadyTask& r) {
    heap_.push_back(r);
    push_heap(heap_.begin(), heap_.end(), prio_greater);
}

bool PriorityOrdering::empty() const { return heap_.empty(); }

vector<ReadyTask> PriorityOrdering::drain_up_to(size_t k) {
    vector<ReadyTask> out;
    out.reserve(min(k, heap_.size()));
    while (!heap_.empty() && out.size() < k) {
        pop_heap(heap_.begin(), heap_.end(), prio_greater);
        out.push_back(heap_.back());
        heap_.pop_back();
    }
    return out;
}

// ── Greedy ───────────────────────────────────────────────────────────────────

vector<MatchResult>
GreedyMatcher::match(const vector<ReadyTask>& batch,
                     const vector<Worker>&    workers)
{
    // Local available_at copy — engine commits authoritatively afterwards.
    vector<int> sim_avail;
    sim_avail.reserve(workers.size());
    for (const auto& w : workers) sim_avail.push_back(w.available_at);

    vector<MatchResult> out;
    out.reserve(batch.size());
    for (size_t ti = 0; ti < batch.size(); ++ti) {
        const ReadyTask& rt = batch[ti];
        int best = -1;
        for (size_t wi = 0; wi < workers.size(); ++wi) {
            if (workers[wi].role != rt.task->required_role) continue;
            if (best == -1 || sim_avail[wi] < sim_avail[best])
                best = static_cast<int>(wi);
        }
        if (best == -1) {
            out.push_back({static_cast<int>(ti), -1, MatchOutcome::NO_ROLE_MATCH});
        } else {
            sim_avail[best] += rt.task->duration_minutes;
            out.push_back({static_cast<int>(ti), best, MatchOutcome::ASSIGNED});
        }
    }
    return out;
}

// ── Fair (max-min water-filling) ─────────────────────────────────────────────

vector<MatchResult>
FairGreedyMatcher::match(const vector<ReadyTask>& batch,
                         const vector<Worker>&    workers)
{
    vector<int> sim_avail;
    vector<int> sim_shift_left;  // smaller = more burned
    sim_avail.reserve(workers.size());
    sim_shift_left.reserve(workers.size());
    for (const auto& w : workers) {
        sim_avail.push_back(w.available_at);
        sim_shift_left.push_back(w.shift_minutes_left);
    }

    vector<MatchResult> out;
    out.reserve(batch.size());
    for (size_t ti = 0; ti < batch.size(); ++ti) {
        const ReadyTask& rt = batch[ti];
        int    best       = -1;
        double best_score = 0.0;
        for (size_t wi = 0; wi < workers.size(); ++wi) {
            if (workers[wi].role != rt.task->required_role) continue;
            const double score =
                alpha_         * static_cast<double>(sim_avail[wi]) +
                (1.0 - alpha_) * static_cast<double>(-sim_shift_left[wi]);
            if (best == -1 || score < best_score) {
                best = static_cast<int>(wi);
                best_score = score;
            }
        }
        if (best == -1) {
            out.push_back({static_cast<int>(ti), -1, MatchOutcome::NO_ROLE_MATCH});
        } else {
            sim_avail[best]      += rt.task->duration_minutes;
            sim_shift_left[best] -= rt.task->duration_minutes;
            out.push_back({static_cast<int>(ti), best, MatchOutcome::ASSIGNED});
        }
    }
    return out;
}

string FairGreedyMatcher::name() const {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "Fair(α=%.2f)", alpha_);
    return std::string(buf);
}

// ── Hungarian ────────────────────────────────────────────────────────────────
//
// Cost matrix N=T+W. Real cells: imp×available_at if role matches, else +∞.
// Dummy padding cells: kLargeDummy. Dummy rows: 0. Square padding ensures
// every real task has T dummy cols, so overflow lands on finite-cost dummies
// rather than being forced through +∞ role-mismatch cells.
//
// importance = max(1, kImportanceOffset − legacy_priority).
// legacy_priority ∈ [11,45] → importance ∈ [5,39].

namespace {

constexpr int    kImportanceOffset = 50;
constexpr double kLargeDummy       = 1e10;
constexpr double kForbiddenCost    = 1e15;

int importance_of(int legacy_priority) {
    int imp = kImportanceOffset - legacy_priority;
    return imp < 1 ? 1 : imp;
}

bool any_role_matched(Role r, const vector<Worker>& workers) {
    for (const auto& w : workers)
        if (w.role == r) return true;
    return false;
}

} // namespace

vector<MatchResult>
HungarianMatcher::match(const vector<ReadyTask>& batch,
                        const vector<Worker>&    workers)
{
    const size_t T = batch.size();
    const size_t W = workers.size();
    if (T == 0) return {};
    if (W == 0) {
        vector<MatchResult> out;
        out.reserve(T);
        for (size_t i = 0; i < T; ++i)
            out.push_back({static_cast<int>(i), -1, MatchOutcome::NO_ROLE_MATCH});
        return out;
    }

    const size_t N = T + W;
    vector<vector<double>> cost(N, vector<double>(N, 0.0));

    // Real tasks × real workers — importance from legacy_priority (stable
    // across LexPolicies so matching cost is invariant under ordering).
    for (size_t i = 0; i < T; ++i) {
        const Task* t = batch[i].task;
        const double imp = static_cast<double>(importance_of(batch[i].legacy_priority));
        for (size_t j = 0; j < W; ++j) {
            if (workers[j].role != t->required_role) {
                cost[i][j] = kForbiddenCost;
            } else {
                int avail = workers[j].available_at;
                if (avail < 0) avail = 0;
                cost[i][j] = imp * static_cast<double>(avail);
            }
        }
    }
    // Real tasks × dummy worker columns
    for (size_t i = 0; i < T; ++i)
        for (size_t j = W; j < N; ++j)
            cost[i][j] = kLargeDummy;
    // Dummy rows already 0.

    vector<int> assign = hungarian_min(cost);

    vector<MatchResult> out;
    out.reserve(T);
    constexpr double kRealThreshold = kLargeDummy / 2.0;
    for (size_t i = 0; i < T; ++i) {
        const int j = assign[i];
        const bool real_col  = (j >= 0) && (static_cast<size_t>(j) < W);
        const bool real_cost = real_col && (cost[i][j] < kRealThreshold);
        if (real_cost) {
            out.push_back({static_cast<int>(i), j, MatchOutcome::ASSIGNED});
        } else {
            const Role required = batch[i].task->required_role;
            const bool has_role = any_role_matched(required, workers);
            out.push_back({static_cast<int>(i), -1,
                           has_role ? MatchOutcome::DEFERRED
                                    : MatchOutcome::NO_ROLE_MATCH});
        }
    }
    return out;
}

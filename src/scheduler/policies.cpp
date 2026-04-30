#include "policies.h"
#include "allocator.h"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <unordered_set>

using namespace std;

namespace {

// Seeded FNV-1a — last-resort tie-break, stable per (seed, task_id).
uint32_t tie_hash(uint32_t seed, const string& task_id) {
    uint32_t h = 0x811c9dc5u ^ seed;
    for (char c : task_id) {
        h ^= static_cast<uint8_t>(c);
        h *= 0x01000193u;
    }
    return h;
}

int acuity_int(Acuity a) { return static_cast<int>(a); }

} // namespace

// Each LexKey: smaller value = more urgent on every dimension. Negate where
// a "longer/older = first" semantics is wanted. Final dim is always tie_hash.

LexKey default_lex_policy(const Task& t, Acuity a, int /*age*/, uint32_t seed) {
    return {acuity_int(a) * 10 + t.priority,
            static_cast<int>(tie_hash(seed, t.id))};
}

static LexKey acuity_first(const Task& t, Acuity a, int /*age*/, uint32_t seed) {
    return {acuity_int(a), t.priority, static_cast<int>(tie_hash(seed, t.id))};
}

static LexKey priority_first(const Task& t, Acuity a, int /*age*/, uint32_t seed) {
    return {t.priority, acuity_int(a), static_cast<int>(tie_hash(seed, t.id))};
}

static LexKey acuity_short_first(const Task& t, Acuity a, int /*age*/, uint32_t seed) {
    return {acuity_int(a), t.duration_minutes, t.priority,
            static_cast<int>(tie_hash(seed, t.id))};
}

static LexKey acuity_long_first(const Task& t, Acuity a, int /*age*/, uint32_t seed) {
    return {acuity_int(a), -t.duration_minutes, t.priority,
            static_cast<int>(tie_hash(seed, t.id))};
}

static LexKey acuity_old_first(const Task& t, Acuity a, int age, uint32_t seed) {
    return {acuity_int(a), -age, t.priority,
            static_cast<int>(tie_hash(seed, t.id))};
}

static LexKey acuity_young_first(const Task& t, Acuity a, int age, uint32_t seed) {
    return {acuity_int(a), age, t.priority,
            static_cast<int>(tie_hash(seed, t.id))};
}

static LexKey duration_first(const Task& t, Acuity a, int /*age*/, uint32_t seed) {
    return {t.duration_minutes, acuity_int(a), t.priority,
            static_cast<int>(tie_hash(seed, t.id))};
}

vector<NamedPolicy> policy_catalog() {
    return {
        {"acuity_first",       "(acuity, task.priority)",                acuity_first},
        {"priority_first",     "(task.priority, acuity)",                priority_first},
        {"acuity_short_first", "(acuity, +duration, task.priority)",     acuity_short_first},
        {"acuity_long_first",  "(acuity, -duration, task.priority)",     acuity_long_first},
        {"acuity_old_first",   "(acuity, -age, task.priority)",          acuity_old_first},
        {"acuity_young_first", "(acuity, +age, task.priority)",          acuity_young_first},
        {"duration_first",     "(+duration, acuity, task.priority)",     duration_first},
    };
}

Objectives compute_objectives(
    const vector<ScheduleEntry>& schedule,
    const vector<Patient>&       patients,
    const vector<Worker>&        workers,
    const CapacityReport&        capacity_report)
{
    (void)capacity_report;
    Objectives obj{0.0, 0, 0};

    int makespan = 0;
    for (const auto& e : schedule)
        if (e.finish_time > makespan) makespan = e.finish_time;
    obj.makespan_minutes = makespan;

    int max_ot = 0;
    for (const auto& w : workers) {
        const int ot = -min(0, w.shift_minutes_left);
        if (ot > max_ot) max_ot = ot;
    }
    obj.max_overtime_minutes = max_ot;

    // Mean last-finish across CRIT+HIGH; patients with no scheduled tasks
    // are excluded (else they'd record 0 and falsely improve the metric).
    unordered_set<string> urgent;
    for (const auto& p : patients)
        if (p.acuity == Acuity::CRITICAL || p.acuity == Acuity::HIGH)
            urgent.insert(p.id);
    if (urgent.empty()) return obj;

    long long sum = 0;
    int n = 0;
    for (const auto& pid : urgent) {
        int last = -1;
        for (const auto& e : schedule)
            if (e.patient_id == pid && e.finish_time > last) last = e.finish_time;
        if (last >= 0) { sum += last; ++n; }
    }
    obj.acuity_metric = (n > 0) ? static_cast<double>(sum) / n : 0.0;
    return obj;
}

// a dominates b iff a ≤ b on every axis and < on at least one.
static bool dominates(const Objectives& a, const Objectives& b) {
    const bool le_all =
        a.acuity_metric        <= b.acuity_metric &&
        a.makespan_minutes     <= b.makespan_minutes &&
        a.max_overtime_minutes <= b.max_overtime_minutes;
    if (!le_all) return false;
    return a.acuity_metric        < b.acuity_metric        ||
           a.makespan_minutes     < b.makespan_minutes     ||
           a.max_overtime_minutes < b.max_overtime_minutes;
}

vector<int> pareto_frontier(const vector<Objectives>& points) {
    vector<int> out;
    const int n = static_cast<int>(points.size());
    out.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        bool dominated = false;
        for (int j = 0; j < n && !dominated; ++j)
            if (i != j && dominates(points[j], points[i])) dominated = true;
        if (!dominated) out.push_back(i);
    }
    return out;
}

// NSGA-II crowding distance, 3D. Boundaries get +∞; interior points
// accumulate (next - prev) / axis_range. Top-k by descending distance,
// then re-sorted by frontier index so output order matches input.
vector<int> top_k_by_crowding(
    const vector<Objectives>& points,
    const vector<int>&        frontier_indices,
    int                       k)
{
    const int F = static_cast<int>(frontier_indices.size());
    if (F == 0) return {};
    if (F <= k) return frontier_indices;

    vector<double> dist(F, 0.0);
    const double INF = numeric_limits<double>::infinity();

    auto run_axis = [&](auto getter) {
        vector<int> idx(F);
        for (int i = 0; i < F; ++i) idx[i] = i;
        sort(idx.begin(), idx.end(), [&](int a, int b) {
            return getter(points[frontier_indices[a]]) <
                   getter(points[frontier_indices[b]]);
        });
        const double lo = getter(points[frontier_indices[idx.front()]]);
        const double hi = getter(points[frontier_indices[idx.back()]]);
        const double range = hi - lo;
        dist[idx.front()] = INF;
        dist[idx.back()]  = INF;
        if (range <= 0.0) return;
        for (int i = 1; i < F - 1; ++i) {
            const double prev = getter(points[frontier_indices[idx[i - 1]]]);
            const double next = getter(points[frontier_indices[idx[i + 1]]]);
            dist[idx[i]] += (next - prev) / range;
        }
    };
    run_axis([](const Objectives& o) { return o.acuity_metric; });
    run_axis([](const Objectives& o) { return static_cast<double>(o.makespan_minutes); });
    run_axis([](const Objectives& o) { return static_cast<double>(o.max_overtime_minutes); });

    vector<int> order(F);
    for (int i = 0; i < F; ++i) order[i] = i;
    sort(order.begin(), order.end(), [&](int a, int b) {
        if (dist[a] != dist[b]) return dist[a] > dist[b];
        return frontier_indices[a] < frontier_indices[b];
    });

    vector<int> picks;
    picks.reserve(static_cast<size_t>(k));
    for (int i = 0; i < k && i < F; ++i) picks.push_back(frontier_indices[order[i]]);
    sort(picks.begin(), picks.end());
    return picks;
}
